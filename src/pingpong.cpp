/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2019                                                  *
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
#include "caf/uri.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

using namespace std;
using namespace caf;
using namespace std::chrono;

namespace {

struct tick_state {
  void tick() {
    cerr << count << ", ";
    count = 0;
  }

  size_t count = 0;
  size_t tick_count = 0;
};

behavior ping_actor(stateful_actor<tick_state>* self, actor sink,
                    size_t iterations) {
  return {
    [=](tick_atom) {
      self->delayed_send(self, seconds(1), tick_atom_v);
      self->state.tick();
      if (self->state.tick_count++ >= iterations) {
        self->send_exit(sink, exit_reason::user_shutdown);
        self->quit();
      }
    },
    [=](start_atom) {
      self->delayed_send(self, seconds(1), tick_atom_v);
      self->send(sink, ping_atom_v);
    },
    [=](pong_atom) {
      self->state.count++;
      return ping_atom_v;
    },
  };
}

behavior pong_actor(event_based_actor*) {
  return {
    [](ping_atom) { return pong_atom_v; },
  };
}

struct config : actor_system_config {
  config() {
    init_global_meta_objects<caf::id_block::caf_net_benchmark>();
    io::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'ioBench', or 'netBench'")
      .add(num_stages, "num-stages,n",
           "number of stages after source / before sink")
      .add(iterations, "iterations,i",
           "number of iterations that should be run");

    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    put(content, "middleman.this-node", earth_id);
    load<net::middleman, net::backend::test>();
    set("logger.file-name", "source.log");
  }

  int num_stages = 0;
  size_t iterations = 10;
  std::string mode;
  uri earth_id;
  uri mars_id;
};

void io_run_ping_actor(socket_pair sockets, size_t iterations) {
  actor_system_config cfg;
  cfg.load<io::middleman>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-name", "sink.log");
  actor_system sys{cfg};
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sockets.second.id);
  auto bb = mm.named_broker<io::basp_broker>("BASP");
  scoped_actor self{sys};
  self->request(bb, infinite, connect_atom_v, move(scribe), uint16_t{8080})
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, set<string>&) {
        if (ptr == nullptr) {
          cerr << "ERROR: could not get a handle to remote source\n";
          return;
        }
        auto act = sys.spawn(ping_actor, actor_cast<actor>(ptr), iterations);
        anon_send(act, start_atom_v);
      },
      [&](error& err) { cerr << "ERROR: " << to_string(err) << endl; });
}

void net_run_ping_actor(socket_pair sockets, size_t iterations) {
  auto mars_id = *make_uri("test://mars");
  auto earth_id = *make_uri("test://earth");
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::test>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-name", "sink.log");
  put(cfg.content, "middleman.this-node", mars_id);
  cfg.parse(0, nullptr);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  backend.emplace(make_node_id(earth_id), sockets.second, sockets.first);
  scoped_actor self{sys};
  auto locator = *make_uri("test://earth/name/source");
  mm.resolve(locator, self);
  actor ping;
  self->receive([&](strong_actor_ptr& ptr, const set<string>&) {
    cerr << "got source: " << to_string(ptr).c_str() << " -> run" << endl;
    ping = sys.spawn(ping_actor, actor_cast<actor>(ptr), iterations);
  });
  // Start ping_pong
  anon_send(ping, start_atom_v);
}

void caf_main(actor_system& sys, const config& cfg) {
  switch (convert(cfg.mode)) {
    case bench_mode::io: {
      cerr << "run in 'ioBench' mode" << endl;
      auto sockets = *make_connected_tcp_socket_pair();
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << endl;
      auto src = sys.spawn(pong_actor);
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sockets.first.id);
      auto bb = mm.named_broker<io::basp_broker>("BASP");
      anon_send(bb, publish_atom_v, move(scribe), uint16_t{8080},
                actor_cast<strong_actor_ptr>(src), set<string>{});
      auto f = [sockets, &cfg] { io_run_ping_actor(sockets, cfg.iterations); };
      thread t{f};
      t.join();
      break;
    }
    case bench_mode::net: {
      cerr << "run in 'netBench' mode " << endl;
      auto sockets = *make_connected_tcp_socket_pair();
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << endl;
      auto src = sys.spawn(pong_actor);
      sys.registry().put("source", src);
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
    default:
      cerr << "mode is invalid: " << cfg.mode << endl;
  }
}

} // namespace

CAF_MAIN(io::middleman)
