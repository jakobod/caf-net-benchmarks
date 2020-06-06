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
#include <vector>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/defaults.hpp"
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/io/network/scribe_impl.hpp"
#include "caf/io/scribe.hpp"
#include "caf/net/backend/tcp.hpp"
#include "caf/net/basp/ec.hpp"
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
    // cout << count << ", ";
    count = 0;
  }

  void timestamp() {
    using namespace std::chrono;
    auto ts = duration_cast<microseconds>(
      system_clock::now().time_since_epoch());
    timestamps.push_back(ts);
  }

  vector<actor> sinks;
  size_t count = 0;
  size_t iterations = 0;
  size_t arrived = 0;
  timestamp_vec timestamps;

  template <class Func>
  void for_each(Func f) {
    for (const auto& sink : sinks)
      f(sink);
  }
};

behavior ping_actor(stateful_actor<tick_state>* self, size_t num_remote_nodes,
                    size_t iterations, size_t num_pings, actor listener) {
  return {
    [=](hello_atom, const actor& sink) {
      self->state.sinks.push_back(sink);
      if (self->state.sinks.size() >= num_remote_nodes) {
        self->send(self, start_atom_v);
      }
    },
    [=](start_atom) {
      self->state.for_each([=](const auto& sink) {
        for (size_t i = 0; i < num_pings; ++i) {
          self->send(sink, ping_atom_v);
          self->state.timestamp();
        }
      });
      self->delayed_send(self, seconds(1), tick_atom_v);
    } // namespace
    ,
    [=](tick_atom) {
      self->delayed_send(self, seconds(1), tick_atom_v);
      self->state.tick();
      if (++self->state.iterations >= iterations) {
        cout << endl;
        self->state.for_each([=](const auto& sink) {
          self->state.timestamp();
          self->send(sink, done_atom_v);
        });
        self->send(listener, self->state.timestamps);
        self->quit();
      }
    },
    [=](pong_atom) {
      self->state.timestamp();
      self->state.count++;
      return ping_atom_v;
    },
  };
}

behavior pong_actor(event_based_actor* self, const actor& source) {
  return {
    [=](start_atom) { self->send(source, hello_atom_v, self); },
    [=](ping_atom) { return pong_atom_v; },
    [=](done_atom) { self->quit(); },
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
      .add(num_remote_nodes, "num_nodes,n", "number of remote nodes")
      .add(num_pings, "pings,p", "number of pings to send");
    source_id = *make_uri("tcp://source");
    put(content, "middleman.this-node", source_id);
    load<net::middleman, net::backend::tcp>();
    set("logger.file-name", "source.log");
  }

  size_t iterations = 10;
  size_t num_remote_nodes = 1;
  size_t num_pings = 1;
  std::string mode = "netBench";
  uri source_id;
};

void io_run_node(uint16_t port, int sock) {
  actor_system_config cfg;
  cfg.load<io::middleman>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-name", "sink.log");
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
        if (ptr == nullptr) {
          cerr << "ERROR: could not get a handle to remote source" << endl;
          return;
        }
        auto sink = sys.spawn(pong_actor, actor_cast<actor>(ptr));
        anon_send(sink, start_atom_v);
      },
      [&](error& err) {
        cerr << "ERROR: " << to_string(err) << endl;
        abort();
      });
}

void net_run_node(uri id, net::stream_socket sock) {
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
  auto ret = backend.emplace(make_node_id(*source_id.authority_only()), sock);
  if (!ret)
    cerr << " emplace failed with err: " << to_string(ret.error()) << endl;
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
  scoped_actor self{sys};
  vector<thread> threads;
  auto src = sys.spawn(ping_actor, cfg.num_remote_nodes, cfg.iterations,
                       cfg.num_pings, self);
  switch (convert(cfg.mode)) {
    case bench_mode::io: {
      auto offset = system_clock::now().time_since_epoch().count();
      cerr << "run in 'ioBench' mode" << endl;
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      auto bb = mm.named_broker<io::basp_broker>("BASP");
      for (size_t port = 0; port < cfg.num_remote_nodes; ++port) {
        auto p = *net::make_stream_socket_pair();
        io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, p.first.id);
        anon_send(bb, publish_atom_v, move(scribe), uint16_t(8080 + port),
                  actor_cast<strong_actor_ptr>(src), set<string>{});
        auto f = [=]() { io_run_node(port, p.second.id); };
        threads.emplace_back(f);
        for (auto& t : threads)
          t.join();

        timestamp_vec actor_ts;
        timestamp_vec dequeue_ts;
        timestamp_vec enqueue_ts;
        self->receive([&](timestamp_vec& ts) { actor_ts = move(ts); });
        self->send(bb, get_timestamps_atom_v);
        self->receive([&](timestamp_vec dequeue, timestamp_vec enqueue) {
          dequeue_ts = dequeue;
          enqueue_ts = enqueue;
        });

        actor_ts = strip_vec(actor_ts, 0, 1);
        dequeue_ts = strip_vec(dequeue_ts, 1, 1);
        enqueue_ts = strip_vec(enqueue_ts, 3, 0);
        timestamp_vec t1; // actor -> dequeue broker
        timestamp_vec t2; // dequeue broker -> enqueue stream
        for (size_t i = 0; i < actor_ts.size(); ++i) {
          t1.push_back(dequeue_ts.at(i) - actor_ts.at(i));
          t2.push_back(enqueue_ts.at(i) - dequeue_ts.at(i));
        }
        init_file(t1.size());
        print_vec(1, t1);
        print_vec(2, t2);
      }
      break;
    }
    case bench_mode::net: {
      cerr << "run in 'netBench' mode " << endl;
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
      mm.publish(src, "source");
      for (size_t i = 0; i < cfg.num_remote_nodes; ++i) {
        auto p = *net::make_stream_socket_pair();
        auto sink_id = *make_uri("tcp://sink"s + to_string(i));
        backend.emplace(make_node_id(sink_id), p.first);
        auto f = [=]() { net_run_node(sink_id, p.second); };
        threads.emplace_back(f);
      }
      for (auto& t : threads)
        t.join();
      timestamp_vec ts_actor;
      self->receive([&](timestamp_vec& ts) { ts_actor = move(ts); });
      auto ts = mm.get_timestamps();
      auto offset = ts.trans_enqueue_.at(0).count();
      cout << endl;
      // First 5 calls are basp handshake.
      ts_actor = strip_vec(ts_actor, 1, 1);
      ts.ep_enqueue_ = strip_vec(ts.ep_enqueue_, 1, 1);
      ts.ep_dequeue_ = strip_vec(ts.ep_dequeue_, 1, 1);
      // only release mode!!
      ts.trans_enqueue_ = strip_vec(ts.trans_enqueue_, 5, 0);
      // debug
      // ts.trans_enqueue_ = strip_vec(ts.trans_enqueue_, 5, 0);

      timestamp_vec t1; // actor -> ep_manager
      timestamp_vec t2; // enqueue ep_manager -> dequeue manager
      timestamp_vec t3; // dequeue ep_manager -> enqueue trans
      for (size_t i = 0; i < ts.ep_enqueue_.size(); ++i) {
        t1.push_back(ts.ep_enqueue_.at(i) - ts_actor.at(i));
        t2.push_back(ts.ep_dequeue_.at(i) - ts.ep_enqueue_.at(i));
        t3.push_back(ts.trans_enqueue_.at(i) - ts.ep_dequeue_.at(i));
      }
      init_file(t1.size());
      print_vec(1, t1);
      print_vec(2, t2);
      print_vec(3, t3);
      break;
    }
    default:
      cerr << "mode is invalid: " << cfg.mode << endl;
  }

  cerr << endl;
}

} // namespace

CAF_MAIN(io::middleman)
