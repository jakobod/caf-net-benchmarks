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
#include "utility.hpp"

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

struct tick_state {
  tick_state() {
    send_ts_.reserve(10000);
  }

  void tick() {
    cerr << count << ", ";
    count = 0;
  }

  void timestamp() {
    send_ts_.emplace_back(
      duration_cast<microseconds>(system_clock::now().time_since_epoch()));
  }

  size_t count = 0;
  size_t tick_count = 0;
  vector<microseconds> send_ts_;
};

behavior ping_actor(stateful_actor<tick_state>* self, actor sink,
                    actor listener, size_t iterations) {
  return {
    [=](tick_atom) {
      self->delayed_send(self, seconds(1), tick_atom::value);
      self->state.tick();
      if (self->state.tick_count++ >= iterations) {
        self->send_exit(sink, exit_reason::user_shutdown);
        self->quit();
        self->send(listener, done_atom::value, move(self->state.send_ts_));
      }
    },
    [=](start_atom) {
      self->state.timestamp();
      self->delayed_send(self, seconds(1), tick_atom::value);
      self->send(sink, ping_atom::value);
    },
    [=](pong_atom) {
      self->state.timestamp();
      self->state.count++;
      self->send(sink, ping_atom::value);
    },
  };
}

behavior pong_actor(event_based_actor*) {
  return {
    [](ping_atom) { return pong_atom::value; },
  };
}

struct config : actor_system_config {
  config() {
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'ioBench', or 'netBench'")
      .add(num_stages, "num-stages,n",
           "number of stages after source / before sink")
      .add(iterations, "iterations,i",
           "number of iterations that should be run");

    add_message_type<uint64_t>("uint64_t");
    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    put(content, "middleman.this-node", earth_id);
    load<net::middleman, net::backend::test>();
    set("logger.file-verbosity", atom("trace"));
    set("logger.file-name", "source.log");
  }

  int num_stages = 0;
  size_t iterations = 10;
  atom_value mode;
  uri earth_id;
  uri mars_id;
};

void io_run_ping_actor(socket_pair sockets, size_t iterations) {
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
  self->request(bb, infinite, connect_atom::value, move(scribe), uint16_t{8080})
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, set<string>&) {
        if (ptr == nullptr) {
          cerr << "ERROR: could not get a handle to remote source\n";
          return;
        }
        auto act = sys.spawn(ping_actor, actor_cast<actor>(ptr), self,
                             iterations);
        anon_send(act, start_atom::value);
      },
      [&](error& err) { cerr << "ERROR: " << sys.render(err) << endl; });
  self->receive([](done_atom, const vector<microseconds>&) {});
}

void net_run_ping_actor(socket_pair sockets, size_t iterations) {
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
  auto locator = *make_uri("test://earth/name/source");
  mm.resolve(locator, self);
  actor ping;
  self->receive([&](strong_actor_ptr& ptr, const set<string>&) {
    cerr << "got source: " << to_string(ptr).c_str() << " -> run" << endl;
    ping = sys.spawn(ping_actor, actor_cast<actor>(ptr), self, iterations);
  });
  // Clear all timestamps due to initialization
  this_thread::sleep_for(seconds(1));
  timestamps.enqueue_ts.clear();
  timestamps.write_event_ts.clear();
  timestamps.write_packet_ts.clear();
  timestamps.write_some_ts.clear();
  // Start ping_pong
  anon_send(ping, start_atom::value);
  vector<microseconds> send_ts;
  self->receive(
    [&send_ts](done_atom, vector<microseconds> ts) { send_ts = move(ts); });
  cerr << endl;
  // strip `down_message` from timestamps
  timestamps.write_event_ts.resize(timestamps.write_event_ts.size() - 1);
  timestamps.write_packet_ts.resize(timestamps.write_packet_ts.size() - 1);
  timestamps.write_some_ts.resize(timestamps.write_some_ts.size() - 1);
  // Print results
  cout << "write:" << endl;
  print_vector("send_ts        ", send_ts);
  print_vector("enqueue_ts     ", timestamps.enqueue_ts);
  print_vector("write_event_ts ", timestamps.write_event_ts);
  print_vector("write_packet_ts", timestamps.write_packet_ts);
  print_vector("write_some_ts  ", timestamps.write_some_ts);
}

void caf_main(actor_system& sys, const config& cfg) {
  switch (static_cast<uint64_t>(cfg.mode)) {
    case io_bench_atom::uint_value(): {
      cerr << "run in 'ioBench' mode" << endl;
      auto sockets = *make_connected_tcp_socket_pair();
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << endl;
      auto src = sys.spawn(pong_actor);
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sockets.first.id);
      auto bb = mm.named_broker<io::basp_broker>(atom("BASP"));
      anon_send(bb, publish_atom::value, move(scribe), uint16_t{8080},
                actor_cast<strong_actor_ptr>(src), set<string>{});
      auto f = [sockets, &cfg] { io_run_ping_actor(sockets, cfg.iterations); };
      thread t{f};
      t.join();
      break;
    }
    case net_bench_atom::uint_value(): {
      cerr << "run in 'netBench' mode " << endl;
      auto sockets = *make_connected_tcp_socket_pair();
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << endl;
      auto src = sys.spawn(pong_actor);
      sys.registry().put(atom("source"), src);
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
      auto ep_pair = backend.emplace(make_node_id(cfg.mars_id), sockets.first,
                                     sockets.second);
      cerr << "spin up second actor sytem" << endl;
      auto f = [sockets, &cfg] { net_run_ping_actor(sockets, cfg.iterations); };
      thread t{f};
      t.join();
      break;
    }
  }
}

} // namespace

CAF_MAIN(io::middleman)
