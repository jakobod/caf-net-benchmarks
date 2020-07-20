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
#include "caf/net/middleman.hpp"
#include "caf/net/udp_datagram_socket.hpp"
#include "caf/uri.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

using namespace std;
using namespace caf;
using namespace std::chrono;
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
      .add(num_remote_nodes, "num_nodes,n", "number of remote nodes")
      .add(num_pings, "pings,p", "number of pings to send");
    put(content, "middleman.this-node", source_id);
    put(content, "scheduler.max-threads", 1);
    load<net::middleman, net::backend::udp>();
    set("logger.file-name", "source.log");
  }

  size_t iterations = 1;
  size_t num_remote_nodes = 1;
  size_t num_pings = 1;
  uri source_id;
};

void net_run_source_node(uri this_node, const std::string& remote_str,
                         net::udp_datagram_socket sock, uint16_t port) {
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::udp>();
  net::middleman::init_global_meta_objects();
  init_global_meta_objects<caf::id_block::caf_net_benchmark>();
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  cfg.set("logger.file-name", "source.log");
  put(cfg.content, "middleman.this-node", this_node);
  put(cfg.content, "scheduler.max-threads", 1);
  cerr << "pong_node " << to_string(this_node) << endl;
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::udp*>(mm.backend("udp"));
  // reset the created endpoint manager with a new one using the passed socket
  auto ret = backend.emplace(sock, port);
  if (!ret)
    exit(ret.error());
  auto remote_locator = make_uri(remote_str + "/name/ping");
  if (!remote_locator)
    exit(remote_locator.error());
  auto source = mm.remote_actor(*remote_locator);
  if (!source)
    exit(to_string(source.error()));
  auto sink = sys.spawn(pong_actor, *source);
  anon_send(sink, start_atom_v);
}

void caf_main(actor_system&, const config& args) {
  cout << args.num_remote_nodes << ", ";
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
  auto ping = sys.spawn(sink_actor, args.num_remote_nodes, args.iterations,
                        args.num_pings);
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::udp*>(mm.backend("udp"));
  // Overwrite the default initialized socket and port.
  backend.emplace(sock, port);
  mm.publish(ping, "ping");
  vector<thread> threads;
  for (size_t i = 0; i < args.num_remote_nodes; ++i) {
    auto ret = net::make_udp_datagram_socket(ep);
    if (!ret)
      exit(ret.error());
    auto [sock, port] = *ret;
    auto pong_id = make_uri("udp://127.0.0.1:"s + to_string(port));
    if (!pong_id)
      exit("make_uri failed", pong_id.error());
    cerr << "ping_id = " << to_string(*this_node)
         << " pong_id = " << to_string(*pong_id) << endl;
    auto f = [pong_id = *pong_id, this_node_str, sock = sock, port = port]() {
      net_run_source_node(pong_id, this_node_str, sock, port);
    };
    threads.emplace_back(f);
  }
  for (auto& t : threads)
    t.join();
}

} // namespace

CAF_MAIN(net::middleman)
