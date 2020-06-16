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
#include "caf/ip_endpoint.hpp"
#include "caf/net/backend/udp.hpp"
#include "caf/net/basp/ec.hpp"
#include "caf/net/datagram_socket.hpp"
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
                    size_t iterations, size_t num_pings) {
  return {
    [=](hello_atom, const actor& sink) {
      self->state.sinks.push_back(sink);
      if (self->state.sinks.size() >= num_remote_nodes) {
        self->send(self, start_atom_v);
      }
    },
    [=](start_atom) {
      self->state.for_each([=](const auto& sink) {
        for (size_t i = 0; i < num_pings; ++i)
          self->send(sink, ping_atom_v);
      });
      self->delayed_send(self, seconds(1), tick_atom_v);
    },
    [=](tick_atom) {
      self->delayed_send(self, seconds(1), tick_atom_v);
      self->state.tick();
      if (++self->state.iterations >= iterations) {
        cout << endl;
        self->state.for_each(
          [=](const auto& sink) { self->send(sink, done_atom_v); });
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
      .add(iterations, "iterations,i",
           "number of iterations that should be run")
      .add(num_remote_nodes, "num_nodes,n", "number of remote nodes")
      .add(num_pings, "pings,p", "number of pings to send");
    source_id = *make_uri("udp://source");
    put(content, "middleman.this-node", source_id);
    load<net::middleman, net::backend::udp>();
    set("logger.file-name", "source.log");
  }

  size_t iterations = 10;
  size_t num_remote_nodes = 1;
  size_t num_pings = 1;
  uri source_id;
};

void net_run_source_node(uri this_node, const std::string& remote_str,
                         net::udp_datagram_socket sock, uint16_t port) {
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::udp>();
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  cfg.set("logger.file-name", "source.log");
  put(cfg.content, "middleman.this-node", this_node);
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
    exit(to_string(err));
  auto ret = net::make_udp_datagram_socket(ep);
  if (!ret)
    exit(ret.error());
  auto sock = ret->first;
  auto port = net::local_port(sock);
  auto this_node_str = "udp://127.0.0.1:" + to_string(port);
  auto this_node = make_uri(this_node_str);
  cerr << "ping_node = " << to_string(this_node) << endl;
  if (!this_node)
    exit(this_node.error());
  vector<thread> threads;
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::udp>();
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  cfg.set("logger.file-name", "ping.log");
  put(cfg.content, "middleman.this-node", *this_node);
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  actor_system sys{cfg};
  auto ping = sys.spawn(ping_actor, args.num_remote_nodes, args.iterations,
                        args.num_pings);
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::udp*>(mm.backend("udp"));
  // Overwrite the default initialized socket and port.
  backend.emplace(sock, *port);
  mm.publish(ping, "ping");
  for (size_t i = 0; i < args.num_remote_nodes; ++i) {
    if (auto err = parse("0.0.0.0:0", ep))
      exit(to_string(err));
    auto ret = net::make_udp_datagram_socket(ep);
    if (!ret)
      exit(ret.error());
    auto sock = ret->first;
    auto port = net::local_port(sock);
    auto pong_id = *make_uri("udp://127.0.0.1:"s + to_string(*port));
    auto f = [=]() {
      net_run_source_node(pong_id, this_node_str, ret->first, *port);
    };
    threads.emplace_back(f);
  }
  for (auto& t : threads)
    t.join();
}

} // namespace

CAF_MAIN(io::middleman)
