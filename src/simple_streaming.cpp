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

#include <iostream>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/defaults.hpp"
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/io/network/scribe_impl.hpp"
#include "caf/io/scribe.hpp"
#include "caf/net/backend/tcp.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/uri.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

using namespace std;
using namespace caf;

namespace {

struct accumulator_state {
  map<actor, vector<size_t>> counts;
  size_t max = 0;
  size_t num_dones = 0;
};

behavior accumulator_actor(stateful_actor<accumulator_state>* self,
                           size_t num_nodes) {
  return {
    [=](size_t amount, const actor& whom) {
      auto& s = self->state;
      s.counts[whom].push_back(amount);
      if (s.max < s.counts.at(whom).size())
        s.max = s.counts.at(whom).size();
    },
    [=](done_atom) {
      cerr << "got done" << endl;
      if (++self->state.num_dones >= num_nodes) {
        auto& s = self->state;
        vector<size_t> acc(s.max);

        for (const auto& p : s.counts) {
          for (int i = 0; i < p.second.size(); ++i) {
            acc.at(i) += p.second.at(i);
          }
        }
        for (auto& v : acc)
          cout << v << ", ";
        cout << endl;
        self->quit();
      }
    },
  };
}

struct tick_state {
  size_t count = 0;
  size_t tick_count = 0;
};

struct source_state : tick_state {
  uint64_t current = 0;
};

behavior source_actor(stateful_actor<source_state>* self) {
  self->set_down_handler([=](const down_msg& msg) {
    cerr << "sink quit!" << endl;
    self->quit();
  });
  return {
    [=](start_atom, const actor& sink) {
      cerr << "START from: " << to_string(self->current_sender()).c_str()
           << endl;
      self->monitor(sink);
      return attach_stream_source(
        self, sink,
        // initialize state
        [&](unit_t&) {
          // nop
        },
        // get next element
        [=](unit_t&, downstream<uint64_t>& out, size_t num) {
          for (size_t i = 0; i < num; ++i)
            out.push(self->state.current++);
        },
        // check whether we reached the end
        [=](const unit_t&) { return false; });
    },
  };
}

struct sink_state : tick_state {
  const char* name = "sink";
};

behavior sink_actor(stateful_actor<sink_state>* self, size_t iterations,
                    actor accumulator) {
  return {
    [=](tick_atom) {
      self->delayed_send(self, chrono::seconds(1), tick_atom_v);
      self->send(accumulator, self->state.count, self);
      self->state.count = 0;
      if (++self->state.tick_count >= iterations) {
        self->send(accumulator, done_atom_v);
        self->quit();
      }
    },
    [=](const stream<uint64_t>& in) {
      self->delayed_send(self, chrono::seconds(1), tick_atom_v);
      return attach_stream_sink(
        self,
        // input stream
        in,
        // initialize state
        [](unit_t&) {
          // nop
        },
        // processing step
        [=](unit_t&, uint64_t x) { ++self->state.count; },
        // cleanup
        [](unit_t&) {
          // nop
        });
    },
  };
}

struct config : actor_system_config {
  config() {
    init_global_meta_objects<caf::id_block::caf_net_benchmark>();
    io::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'local', 'ioBench', or 'netBench'")
      .add(num_remote_nodes, "num-nodes,n", "number of remote nodes")
      .add(iterations, "iterations,i",
           "number of iterations that should be run");

    earth_id = *make_uri("tcp://earth");
    put(content, "middleman.this-node", earth_id);
    load<net::middleman, net::backend::tcp>();
    set("logger.file-name", "source.log");
  }

  int num_remote_nodes = 1;
  size_t iterations = 10;
  std::string mode = "netBench";
  uri earth_id;
};

void io_run_source(net::stream_socket sock, uint16_t port) {
  actor_system_config cfg;
  cfg.load<io::middleman>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-name", "sink.log");
  actor_system sys{cfg};
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock.id);
  auto bb = mm.named_broker<io::basp_broker>("BASP");
  scoped_actor self{sys};
  self->request(bb, infinite, connect_atom_v, move(scribe), port)
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, set<string>&) {
        if (ptr == nullptr) {
          cerr << "ERROR: could not get a handle to remote source\n";
          return;
        }
        auto source = sys.spawn(source_actor);
        self->send(source, start_atom_v, actor_cast<actor>(ptr));
      },
      [&](error& err) { cerr << "ERROR: " << to_string(err) << endl; });
}

void net_run_source(net::stream_socket sock, size_t id) {
  cerr << "id = " << id << endl;
  auto source_id = *make_uri("tcp://source"s + to_string(id));
  auto sink_locator = *make_uri("tcp://earth/name/sink"s + to_string(id));
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::tcp>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-name", "sink.log");
  put(cfg.content, "middleman.this-node", source_id);
  cfg.parse(0, nullptr);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
  auto ret = backend.emplace(make_node_id(*sink_locator.authority_only()),
                             sock);
  if (!ret)
    cerr << " emplace failed with err: " << to_string(ret.error()) << endl;
  cerr << "resolve locator " << to_string(sink_locator) << endl;
  auto sink = mm.remote_actor(sink_locator);
  if (!sink) {
    cerr << "ERROR: " << to_string(sink.error()) << endl;
    abort();
  }
  scoped_actor self{sys};
  auto source = sys.spawn(source_actor);
  self->send(source, start_atom_v, *sink);
}

void caf_main(actor_system& sys, const config& cfg) {
  cout << cfg.num_remote_nodes << ", ";
  vector<thread> threads;
  auto accumulator = sys.spawn(accumulator_actor, cfg.num_remote_nodes);
  switch (convert(cfg.mode)) {
    case bench_mode::io: {
      cerr << "run in 'ioBench' mode" << endl;
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      auto bb = mm.named_broker<io::basp_broker>("BASP");
      for (size_t port = 0; port < cfg.num_remote_nodes; ++port) {
        auto p = *net::make_stream_socket_pair();
        io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, p.first.id);
        auto sink = sys.spawn(sink_actor, cfg.iterations, accumulator);
        anon_send(bb, publish_atom_v, move(scribe), uint16_t(8080 + port),
                  actor_cast<strong_actor_ptr>(sink), set<string>{});
        auto f = [=]() { io_run_source(p.second, port); };
        threads.emplace_back(f);
      }
      break;
    }
    case bench_mode::net: {
      cerr << "run in 'netBench' mode " << endl;
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
      for (size_t node = 0; node < cfg.num_remote_nodes; ++node) {
        cerr << "node = " << node << endl;
        auto source_id = *make_uri("tcp://source"s + to_string(node));
        cerr << "source_id: " << to_string(source_id) << endl;
        auto sink = sys.spawn(sink_actor, cfg.iterations, accumulator);
        sys.registry().put("sink"s + to_string(node), sink);
        auto sockets = *make_connected_tcp_socket_pair();
        backend.emplace(make_node_id(source_id), sockets.first);
        auto f = [=]() { net_run_source(sockets.second, node); };
        threads.emplace_back(f);
      }
      break;
    }
    default:
      cerr << "mode is invalid: " << cfg.mode << endl;
  }
  for (auto& thread : threads)
    thread.join();
}

} // namespace

CAF_MAIN(io::middleman)
