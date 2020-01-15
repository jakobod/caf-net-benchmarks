/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2017                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENCE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include <chrono>
#include <iostream>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/defaults.hpp"
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/io/network/scribe_impl.hpp"
#include "caf/io/scribe.hpp"
#include "caf/net/backend/test.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_accept_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"

using namespace std;
using namespace caf;
using namespace std::chrono;

namespace {

using start_atom = atom_constant<atom("start")>;
using tick_atom = atom_constant<atom("tick")>;
using ping_atom = atom_constant<atom("ping")>;
using pong_atom = atom_constant<atom("pong")>;
using done_atom = atom_constant<atom("done")>;

using io_bench_atom = atom_constant<atom("ioBench")>;
using net_bench_atom = atom_constant<atom("netBench")>;
using local_bench_atom = atom_constant<atom("localBench")>;

using socket_pair = pair<net::stream_socket, net::stream_socket>;

struct tick_state {
  tick_state() {
    send_ts_.reserve(10000);
  }

  size_t count = 0;
  size_t tick_count = 0;
  std::vector<microseconds> send_ts_;

  void tick() {
    cerr << count << ", ";
    count = 0;
  }

  void timestamp() {
    send_ts_.emplace_back(
      duration_cast<microseconds>(system_clock::now().time_since_epoch()));
  }
};

/// This actor will start a pingpong, answer any pong and print counts.
behavior ping_actor(stateful_actor<tick_state>* self, actor sink,
                    actor listener, size_t iterations) {
  return {
    [=](tick_atom) {
      self->delayed_send(self, std::chrono::seconds(1), tick_atom::value);
      self->state.tick();
      if (self->state.tick_count++ >= iterations) {
        self->quit();
        self->send(listener, done_atom::value, std::move(self->state.send_ts_));
      }
    },
    [=](pong_atom) {
      self->state.timestamp();
      self->state.count++;
      self->send(sink, ping_atom::value);
    },
  };
}

/// Object is what should be sent.
behavior pong_actor(event_based_actor*) {
  return {[](ping_atom) { return pong_atom::value; }};
}

struct config : actor_system_config {
  config() {
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'ioBench', or 'netBench'")
      .add(num_stages, "num-stages,n",
           "number of stages after source / before sink")
      .add(iterations, "iterations,i",
           "number of iterations that should be run")
      .add(num_pairs, "num-pairs,p",
           "number of src/sink pairs that should be run");

    add_message_type<uint64_t>("uint64_t");
    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    put(content, "middleman.this-node", earth_id);
    load<net::middleman, net::backend::test>();
    set("logger.file-verbosity", atom("trace"));
    set("logger.file-name", "source.log");
    actor_atoms = std::vector<atom_value>{
      atom("actor0"), atom("actor1"), atom("actor2"),  atom("actor3"),
      atom("actor4"), atom("actor5"), atom("actor6"),  atom("actor7"),
      atom("actor8"), atom("actor9"), atom("actor10"),
    };
  }

  int num_stages = 0;
  size_t iterations = 10;
  atom_value mode;
  uri earth_id;
  uri mars_id;
  size_t num_pairs = 1;
  std::vector<atom_value> actor_atoms;
};

expected<std::pair<net::stream_socket, net::stream_socket>>
make_connected_tcp_socket_pair() {
  using namespace net;
  net::tcp_accept_socket acceptor;
  uri::authority_type auth;
  auth.port = 0;
  auth.host = "0.0.0.0"s;
  if (auto res = net::make_tcp_accept_socket(auth, false))
    acceptor = *res;
  else
    return res.error();
  uri::authority_type dst;
  dst.host = "localhost"s;
  if (auto port = local_port(socket_cast<network_socket>(acceptor)))
    dst.port = *port;
  else
    return port.error();
  tcp_stream_socket sock1;
  if (auto res = make_connected_tcp_stream_socket(dst))
    sock1 = *res;
  else
    return res.error();
  if (auto res = accept(acceptor))
    return make_pair(sock1, *res);
  else
    return res.error();
}

template <class T>
void print_vector(string name, const std::vector<T>& vec) {
  cout << name << ": ";
  for (const auto& v : vec) {
    auto val = v.count();
    // auto val = v.count() - vec.begin()->count();
    cout << val << ", ";
  }
  cout << endl;
}

template <class T>
void erase(std::vector<T>& vec, size_t begin, size_t end) {
  vec.erase(vec.begin() + begin, vec.begin() + end);
}

void io_run_ping_actor(socket_pair sockets, size_t, size_t iterations) {
  actor_system_config cfg;
  cfg.add_message_type<uint64_t>("uint64_t");
  cfg.load<io::middleman>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-verbosity", atom("trace"));
  cfg.set("logger.file-name", "sink.log");
  actor_system sys{cfg};
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sockets.second.id);
  auto bb = mm.named_broker<io::basp_broker>(atom("BASP"));
  scoped_actor self{sys};
  self
    ->request(bb, infinite, connect_atom::value, std::move(scribe),
              uint16_t{8080})
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, std::set<std::string>&) {
        if (ptr == nullptr) {
          std::cerr << "ERROR: could not get a handle to remote source\n";
          return;
        }
        auto act = sys.spawn(ping_actor, actor_cast<actor>(ptr), self,
                             iterations);
        anon_send(act, pong_atom::value);
        delayed_anon_send(act, seconds(1), tick_atom::value);
      },
      [&](error& err) {
        std::cerr << "ERROR: " << sys.render(err) << std::endl;
      });
  self->receive([](done_atom, std::vector<microseconds>) {});
}

void net_run_ping_actor(socket_pair sockets, size_t num_pairs,
                        size_t iterations) {
  auto mars_id = *make_uri("test://mars");
  auto earth_id = *make_uri("test://earth");
  actor_system_config cfg;
  cfg.add_message_type<uint64_t>("uint64_t");
  cfg.load<net::middleman, net::backend::test>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-verbosity", atom("trace"));
  cfg.set("logger.file-name", "sink.log");
  put(cfg.content, "middleman.this-node", mars_id);
  cfg.parse(0, nullptr);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  auto& ep_pair = backend.emplace(make_node_id(earth_id), sockets.second,
                                  sockets.first);
  auto& ep_manager = ep_pair.second;
  auto timestamps = ep_manager->get_timestamps();
  scoped_actor self{sys};
  for (size_t i = 0; i < num_pairs; ++i) {
    auto locator = *make_uri("test://earth/name/actor" + to_string(i));
    cerr << "resolve locator: " << to_string(locator) << endl;
    mm.resolve(locator, self);
  }
  std::vector<actor> ping_actors;
  for (size_t i = 0; i < num_pairs; ++i) {
    self->receive([&](strong_actor_ptr& ptr, const std::set<std::string>&) {
      std::cerr << "got source: " << to_string(ptr).c_str() << " -> run"
                << std::endl;
      ping_actors.emplace_back(
        sys.spawn(ping_actor, actor_cast<actor>(ptr), self, iterations));
    });
  }
  this_thread::sleep_for(seconds(1));
  timestamps.enqueue_ts.clear();
  timestamps.write_event_ts.clear();
  timestamps.write_packet_ts.clear();
  timestamps.write_some_ts.clear();
  anon_send(*ping_actors.begin(), pong_atom::value);
  delayed_anon_send(*ping_actors.begin(), seconds(1), tick_atom::value);
  std::vector<microseconds> send_ts;
  self->receive([&send_ts](done_atom, std::vector<microseconds> ts) {
    send_ts = std::move(ts);
  });
  cerr << endl;

  print_vector("send_ts        ", send_ts);
  print_vector("enqueue_ts     ", timestamps.enqueue_ts);
  print_vector("write_event_ts ", timestamps.write_event_ts);
  print_vector("write_packet_ts", timestamps.write_packet_ts);
  print_vector("write_some_ts  ", timestamps.write_some_ts);
  /*
    erase(timestamps.write_event_ts, 0, 3);
    erase(timestamps.write_packet_ts, 0, 3);
    erase(timestamps.write_some_ts, 0, 3);

    auto min_len = min({send_ts.size(), timestamps.enqueue_ts.size(),
                        timestamps.write_event_ts.size(),
                        timestamps.write_packet_ts.size(),
                        timestamps.write_some_ts.size()});
    auto max_len = max({send_ts.size(), timestamps.enqueue_ts.size(),
                        timestamps.write_event_ts.size(),
                        timestamps.write_packet_ts.size(),
                        timestamps.write_some_ts.size()});
    cout << "min_len: " << min_len << endl;
    cout << "max_len: " << max_len << endl;

    send_ts.resize(min_len);
    timestamps.enqueue_ts.resize(min_len);
    timestamps.write_event_ts.resize(min_len);
    timestamps.write_packet_ts.resize(min_len);
    timestamps.write_some_ts.resize(min_len);
  */
}

void caf_main(actor_system& sys, const config& cfg) {
  /*auto serializer = get_or(sys.config(), "middleman.serializing_workers",
                           defaults::middleman::serializing_workers);
  auto deserializer = get_or(sys.config(), "middleman.workers",
                             defaults::middleman::workers);
  cout << serializer << ", " << deserializer << ", ";*/
  switch (static_cast<uint64_t>(cfg.mode)) {
    case io_bench_atom::uint_value(): {
      cerr << "run in 'ioBench' mode" << std::endl;
      std::pair<net::stream_socket, net::stream_socket> sockets;
      if (auto res = make_connected_tcp_socket_pair()) {
        sockets = *res;
      } else {
        std::cerr << "ERROR: socket creation failed" << std::endl;
        return;
      }
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << endl;
      auto src = sys.spawn(pong_actor);
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sockets.first.id);
      auto bb = mm.named_broker<io::basp_broker>(atom("BASP"));
      anon_send(bb, publish_atom::value, std::move(scribe), uint16_t{8080},
                actor_cast<strong_actor_ptr>(src), std::set<std::string>{});
      auto f = [sockets, &cfg] {
        io_run_ping_actor(sockets, cfg.num_pairs, cfg.iterations);
      };
      std::thread t{f};
      t.join();
      anon_send_exit(src, exit_reason::user_shutdown);
      cout << std::endl;
      break;
    }
    case net_bench_atom::uint_value(): {
      if (cfg.num_pairs > cfg.actor_atoms.size()) {
        cerr << "ERROR: number of pairs should not be greater than "
             << cfg.actor_atoms.size() << endl;
        return;
      }
      cerr << "run in 'netBench' mode " << endl;
      auto sockets = *make_connected_tcp_socket_pair();
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << std::endl;
      cerr << "spawn " << cfg.num_pairs << " pong_actors" << endl;
      std::vector<actor> pong_actors;
      for (size_t i = 0; i < cfg.num_pairs; ++i) {
        auto src = sys.spawn(pong_actor);
        sys.registry().put(cfg.actor_atoms.at(i), src);
        pong_actors.emplace_back(std::move(src));
      }
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
      auto ep_pair = backend.emplace(make_node_id(cfg.mars_id), sockets.first,
                                     sockets.second);
      cerr << "spin up second actor sytem" << endl;
      auto f = [sockets, &cfg] {
        net_run_ping_actor(sockets, cfg.num_pairs, cfg.iterations);
      };
      thread t{f};
      t.join();
      cerr << "joined" << std::endl;
      for (auto& a : pong_actors)
        anon_send_exit(a, exit_reason::user_shutdown);
      break;
    }
  }
}

} // namespace

CAF_MAIN(io::middleman)
