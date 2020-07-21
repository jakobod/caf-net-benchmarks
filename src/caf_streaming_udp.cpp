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
#include <thread>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/defaults.hpp"
#include "caf/ip_endpoint.hpp"
#include "caf/net/all.hpp"
#include "caf/net/backend/udp.hpp"
#include "caf/net/basp/ec.hpp"
#include "caf/net/endpoint_manager.hpp"
#include "caf/net/backend/udp.hpp"
#include "caf/net/datagram_socket.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/udp_datagram_socket.hpp"
#include "caf/uri.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

using namespace std;
using namespace std::chrono;
using namespace caf;
using namespace caf::net;

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
    net::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(iterations, "iterations,i",
           "number of iterations that should be run")
      .add(num_remote_nodes, "num_nodes,n", "number of remote nodes");
    put(content, "scheduler.max-threads", 1);
    load<net::middleman, net::backend::udp>();
    set("logger.file-name", "source.log");
  }

  int num_remote_nodes = 1;
  size_t iterations = 10;
  std::string mode = "netBench";
  uri earth_id;
};

void net_run_source_node(net::udp_datagram_socket sock, uri sink_locator,
                         uri this_node, uint16_t port) {
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::udp>();
  net::middleman::init_global_meta_objects();
  init_global_meta_objects<caf::id_block::caf_net_benchmark>();
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  cfg.set("logger.file-name", "source.log");
  put(cfg.content, "middleman.this-node", this_node);
  put(cfg.content, "scheduler.max-threads", 1);
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::udp*>(mm.backend("udp"));
  // reset the created endpoint manager with a new one using the passed socket
  auto ret = backend.emplace(sock, port);
  if (!ret)
    exit(ret.error());
  auto sink = mm.remote_actor(sink_locator);
  if (!sink)
    exit(sink.error());
  auto source = sys.spawn(source_actor);
  anon_send(source, start_atom_v, *sink);
}

void caf_main(actor_system&, const config& args) {
  ip_endpoint ep;
  if (auto err = parse("0.0.0.0:0", ep))
    exit("could not parse endpoint", err);
  auto ret = net::make_udp_datagram_socket(ep);
  if (!ret)
    exit(ret.error());
  auto [sock, port] = *ret;
  auto this_node_str = "udp://127.0.0.1:" + to_string(port);
  auto this_node = make_uri(this_node_str);
  cerr << "ping_node = " << to_string(this_node) << endl;
  if (!this_node)
    exit("make_uri failed", this_node.error());
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::udp>();
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  cfg.set("logger.file-name", "ping.log");
  put(cfg.content, "middleman.this-node", *this_node);
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  actor_system sys{cfg};
  auto accumulator = sys.spawn(accumulator_actor, args.num_remote_nodes);
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::udp*>(mm.backend("udp"));
  // Overwrite the default initialized socket and port.
  backend.emplace(sock, port);
  vector<thread> threads;
  for (size_t i = 0; i < args.num_remote_nodes; ++i) {
    auto sink = sys.spawn(sink_actor, args.iterations, accumulator);
    auto sink_str = "sink"s + to_string(i);
    mm.publish(sink, sink_str);
    auto sink_locator = *make_uri(this_node_str + "/name/" + sink_str);
    // create remote socket and remote_node_id
    auto ret = net::make_udp_datagram_socket(ep);
    if (!ret)
      exit(ret.error());
    auto sock = ret->first;
    auto port = ret->second;
    auto remote_node_id = *make_uri("udp://127.0.0.1:"s + to_string(port));
    auto f = [sink_locator, sock, remote_node_id, port]() {
      net_run_source_node(sock, sink_locator, remote_node_id, port);
    };
    threads.emplace_back(f);
  }
  for (auto& t : threads)
    t.join();
}

} // namespace

CAF_MAIN(net::middleman)
