/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2020                                                  *
 * Jakob Otto <jakob.otto (at) haw-hamburg.de>                                *
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
#include <functional>
#include <iostream>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/defaults.hpp"
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/io/network/scribe_impl.hpp"
#include "caf/io/scribe.hpp"
#include "caf/net/backend/tcp.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/socket_manager.hpp"
#include "caf/uri.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

using namespace std;
using namespace caf;
using namespace std::chrono;

namespace {

struct tick_state {
  tick_state() {
    t1.reserve(1'000'000);
    t2.reserve(1'000'000);
    t3.reserve(1'000'000);
  }

  void send_timestamp() {
    timestamp_impl(t1);
  }

  void remote_timestamp(microseconds ts) {
    t2.emplace_back(ts);
  }

  void receive_timestamp() {
    timestamp_impl(t3);
  }

  actor sink;

  vector<microseconds> t1;
  vector<microseconds> t2;
  vector<microseconds> t3;

private:
  void timestamp_impl(vector<microseconds>& vec) {
    vec.emplace_back(
      duration_cast<microseconds>(system_clock::now().time_since_epoch()));
  }
};

behavior ping_actor(stateful_actor<tick_state>* self, size_t max_pongs,
                    actor listener) {
  return {
    [=](hello_atom, const actor& sink) {
      self->state.sink = sink;
      self->send(self, start_atom_v);
    },
    [=](start_atom) {
      self->send(self->state.sink, ping_atom_v);
      self->state.send_timestamp();
    },
    [=](pong_atom, microseconds remote_ts) {
      self->state.receive_timestamp();
      self->state.remote_timestamp(remote_ts);
      if (self->state.t3.size() == max_pongs) {
        self->send(listener, done_atom_v, self->state.t1, self->state.t2,
                   self->state.t3);
        self->quit();
        return make_message(done_atom_v);
      } else {
        self->state.send_timestamp();
        return make_message(ping_atom_v);
      }
    },
  };
}

behavior pong_actor(event_based_actor* self, const actor& source) {
  return {
    [=](start_atom) { self->send(source, hello_atom_v, self); },
    [=](ping_atom) {
      auto ts
        = duration_cast<microseconds>(system_clock::now().time_since_epoch());
      return make_message(pong_atom_v, ts);
    },
    [=](done_atom) { self->quit(); },
  };
}

struct config : actor_system_config {
  config() {
    init_global_meta_objects<caf::id_block::caf_net_benchmark>();
    io::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'ioBench', or 'netBench'")
      .add(max_pongs, "pongs,p", "number of pongs to remote");
    source_id = *make_uri("tcp://source");
    put(content, "middleman.this-node", source_id);
    put(content, "scheduler.max-threads", 1);
    load<net::middleman, net::backend::tcp>();
    set("logger.file-name", "source.log");
  }

  size_t max_pongs = 10'000;
  std::string mode = "netBench";
  uri source_id;
};

void io_run_node(uint16_t port, int sock) {
  actor_system_config cfg;
  cfg.load<io::middleman>();
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  cfg.set("logger.file-name", "sink.log");
  put(cfg.content, "scheduler.max-threads", 1);
  actor_system sys{cfg};
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock);
  auto bb = mm.named_broker<io::basp_broker>("BASP");
  scoped_actor self{sys};
  self->request(bb, infinite, connect_atom_v, move(scribe), port)
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, set<string>&) {
        if (ptr == nullptr)
          exit("could not get a handle to remote source");
        auto sink = sys.spawn(pong_actor, actor_cast<actor>(ptr));
        anon_send(sink, start_atom_v);
      },
      [&](error& err) { exit("failed to resolve", err); });
}

void net_run_node(uri id, net::stream_socket sock) {
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::tcp>();
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  cfg.set("logger.file-name", "sink.log");
  put(cfg.content, "middleman.this-node", id);
  put(cfg.content, "scheduler.max-threads", 1);
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
  auto source_id = *make_uri("tcp://source/name/source");
  auto ret = backend.emplace(make_node_id(*source_id.authority_only()), sock);
  if (!ret)
    exit("emplace failed", ret.error());
  auto source = mm.remote_actor(source_id);
  if (!source)
    exit("remote_actor failed", source.error());
  auto sink = sys.spawn(pong_actor, *source);
  anon_send(sink, start_atom_v);
  scoped_actor self{sys};
  self->await_all_other_actors_done();
}

void caf_main(actor_system& sys, const config& cfg) {
  thread t;
  scoped_actor self{sys};
  auto src = sys.spawn(ping_actor, cfg.max_pongs, self);
  switch (convert(cfg.mode)) {
    case bench_mode::io: {
      std::cerr << "run in 'ioBench' mode" << endl;
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      auto bb = mm.named_broker<io::basp_broker>("BASP");
      auto p = *net::make_stream_socket_pair();
      io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, p.first.id);
      anon_send(bb, publish_atom_v, move(scribe), uint16_t(8080),
                actor_cast<strong_actor_ptr>(src), set<string>{});
      auto f = [=]() { io_run_node(0, p.second.id); };
      t = thread{f};
      break;
    }
    case bench_mode::net: {
      cerr << "run in 'netBench' mode " << endl;
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
      mm.publish(src, "source");
      auto p = *make_connected_tcp_socket_pair();
      auto sink_id = *make_uri("tcp://sink");
      backend.emplace(make_node_id(sink_id), p.first);
      auto f = [=]() { net_run_node(sink_id, p.second); };
      t = thread{f};
      break;
    }
    default:
      exit("invalid mode: \""s + cfg.mode + "\"");
  }
  self->receive([&](done_atom, vector<microseconds> t1, vector<microseconds> t2,
                    vector<microseconds> t3) {
    std::cout << "what, ";
    for (size_t i = 0; i < t3.size(); ++i)
      std::cout << "value"s + to_string(i) + ", ";
    std::cout << std::endl;
    std::cout << "request, ";
    for (size_t i = 0; i < t3.size(); ++i)
      std::cout << to_string(t2.at(i).count() - t1.at(i).count()) + ", ";
    std::cout << std::endl;
    std::cout << "response, ";
    for (size_t i = 0; i < t3.size(); ++i)
      std::cout << to_string(t3.at(i).count() - t2.at(i).count()) + ", ";
    std::cout << std::endl;
  });
  t.join();
  std::cerr << endl;
}

} // namespace

CAF_MAIN(io::middleman)
