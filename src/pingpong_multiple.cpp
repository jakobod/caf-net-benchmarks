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
#include "caf/net/basp/ec.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/endpoint_manager.hpp"
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
    cout << count << ", ";
    count = 0;
  }

  vector<actor> sinks;
  size_t count = 0;
  size_t iterations = 0;
  size_t arrived = 0;

  template <class Func>
  void for_each(Func f) {
    for (const auto& sink : sinks)
      f(sink);
  }
};

behavior ping_actor(stateful_actor<tick_state>* self, size_t num_remote_nodes,
                    size_t iterations) {
  aout(self) << "source spawned" << endl;
  return {
    [=](hello_atom, actor sink) {
      aout(self) << "got hello!" << endl;
      self->state.sinks.push_back(sink);
      if (self->state.sinks.size() >= num_remote_nodes) {
        self->send(self, start_atom_v);
        aout(self) << "got all hellos" << endl;
      }
    },
    [=](start_atom) {
      self->state.for_each(
        [=](const auto& sink) { self->send(sink, ping_atom_v); });
      self->delayed_send(self, seconds(1), tick_atom_v);
    },
    [=](tick_atom) {
      self->delayed_send(self, seconds(1), tick_atom_v);
      self->state.tick();
      if (++self->state.iterations >= iterations) {
        cout << endl;
        self->state.for_each([=](const auto& sink) {
          self->send_exit(sink, exit_reason::normal);
        });
        self->quit();
      }
    },
    [=](pong_atom) {
      self->state.count++;
      return ping_atom_v;
    },
  };
}

behavior pong_actor(event_based_actor* self, const actor& source) {
  aout(self) << "sink spawned" << endl;
  return {
    [=](start_atom) { self->send(source, hello_atom_v, self); },
    [=](ping_atom) { return pong_atom_v; },
  };
}

struct config : actor_system_config {
  config() {
    init_global_meta_objects<caf::id_block::caf_net_benchmark>();
    io::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'ioBench', or 'netBench'")
      .add(iterations, "iterations,i",
           "number of iterations that should be run")
      .add(num_pings, "pings,p", "number of pings that should be sent")
      .add(num_sources, "sources,s", "number of sources to spawn")
      .add(num_sinks, "sinks,S", "number of sinks to spawn")
      .add(num_remote_nodes, "remote_nodes,r", "number of remote nodes");
    source_id = *make_uri("tcp://source");
    put(content, "middleman.this-node", source_id);
    load<net::middleman, net::backend::tcp>();
    set("logger.file-name", "source.log");
  }

  int num_stages = 0;
  size_t num_sources = 1;
  size_t num_sinks = 1;
  size_t iterations = 10;
  size_t num_pings = 1;
  std::string mode = "netBench";
  size_t num_remote_nodes = 1;
  uri source_id;
};

void net_run_node(uri id, net::stream_socket sock) {
  cerr << __FUNCTION__ << endl;
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::tcp>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-name", "sink.log");
  put(cfg.content, "middleman.this-node", id);
  cfg.parse(0, nullptr);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
  auto source_id = *make_uri("tcp://source/name/source");
  backend.emplace(make_node_id(*source_id.authority_only()), sock);
  auto source = mm.remote_actor(source_id);
  if (!source) {
    cerr << "got error while resolving: " << to_string(source.error()) << endl;
    abort(); // kill this thread and everything else!
  }
  auto sink = sys.spawn(pong_actor, *source);
  anon_send(sink, start_atom_v);
  scoped_actor self{sys};
  self->await_all_other_actors_done();
}

void caf_main(actor_system& sys, const config& cfg) {
  cout << cfg.num_pings << ", " << cfg.num_pings << ", ";
  switch (convert(cfg.mode)) {
    case bench_mode::io: {
      // nop
    }
    case bench_mode::net: {
      cerr << "run in 'netBench' mode " << endl;
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
      auto source = sys.spawn(ping_actor, cfg.num_remote_nodes, cfg.iterations);
      mm.publish(source, "source");
      vector<thread> threads;
      for (size_t i = 0; i < cfg.num_remote_nodes; ++i) {
        auto p = *net::make_stream_socket_pair();
        auto sink_id = *make_uri("tcp://sink"s + to_string(i));
        backend.emplace(make_node_id(sink_id), p.first);
        auto f = [=]() { net_run_node(sink_id, p.second); };
        threads.emplace_back(f);
      }
      for (auto& t : threads) {
        t.join();
      }
      break;
    }
    default:
      cerr << "mode is invalid: " << cfg.mode << endl;
  }
  cerr << endl;
}

} // namespace

CAF_MAIN(io::middleman)
