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

#include "accumulator.hpp"
#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/byte.hpp"
#include "caf/defaults.hpp"
#include "caf/ip_endpoint.hpp"
#include "caf/net/all.hpp"
#include "caf/net/backend/udp.hpp"
#include "caf/net/basp/ec.hpp"
#include "caf/net/ip.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/udp_datagram_socket.hpp"
#include "caf/uri.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

using namespace caf;
using namespace std::chrono;

namespace {

using payload = std::vector<caf::byte>;

struct ping_state {
  size_t count = 0;
};

behavior ping_actor(stateful_actor<ping_state>* self, const actor& accumulator,
                    size_t num_pings, size_t payload_size) {
  self->set_exit_handler([=](const exit_msg&) { self->quit(); });
  self->link_to(accumulator);
  return {
    [=](init_atom) {
      self->send(accumulator, init_atom_v);
      payload p(payload_size);
      return p;
    },
    [=](const payload& p) {
      if (++self->state.count >= num_pings)
        self->send(accumulator, done_atom_v);
      return p;
    },
  };
}

behavior pong_actor(event_based_actor* self, const actor& source) {
  self->set_exit_handler([=](const exit_msg&) { self->quit(); });
  self->link_to(source);
  return {
    [=](start_atom) { self->send(source, init_atom_v); },
    [=](const payload& p) { return p; },
  };
}
struct config : actor_system_config {
  config() {
    init_global_meta_objects<caf::id_block::caf_net_benchmark>();
    net::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(num_remote_nodes, "num_nodes,n", "number of remote nodes")
      .add(num_pings, "pings,p", "number of pings to exchange")
      .add(payload_size, "size,s", "size of the exchanged payload");
    put(content, "caf.middleman.this-node", this_node);
    put(content, "caf.scheduler.max-threads", 1);
    load<net::middleman, net::backend::udp>();
  }

  size_t payload_size = 1;
  size_t num_remote_nodes = 1;
  size_t num_pings = 1024;
  uri this_node;
};

void net_run_source_node(uri this_node, const std::string& remote_str,
                         net::udp_datagram_socket sock, uint16_t port) {
  std::cerr << "net_run_source_node thread started! " << std::endl;
  std::cerr << "thread got socket " << sock.id << std::endl;
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::udp>();
  net::middleman::init_global_meta_objects();
  init_global_meta_objects<caf::id_block::caf_net_benchmark>();
  if (auto err = cfg.parse(0, nullptr))
    exit("thread could not parse config 1: ", err);
  put(cfg.content, "caf.middleman.this-node", this_node);
  put(cfg.content, "caf.scheduler.max-threads", 1);
  std::cerr << "pong_node " << to_string(this_node) << std::endl;
  if (auto err = cfg.parse(0, nullptr))
    exit("thread could not parse config 2: ", err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::udp*>(mm.backend("udp"));
  // reset the created endpoint manager with a new one using the passed socket
  std::cerr << "emplacing socket " << sock.id << std::endl;
  auto ret = backend.emplace(sock, port);
  if (!ret)
    exit("thread backend.emplace failed: ", ret.error());
  auto remote_locator = make_uri(remote_str + "/name/ping");
  if (!remote_locator)
    exit("thread make_uri failed: ", remote_locator.error());
  auto source = mm.remote_actor(*remote_locator, 2s);
  if (!source)
    exit("thread remote actor failed: ", source.error());
  auto sink = sys.spawn(pong_actor, *source);
  anon_send(sink, start_atom_v);
}

void caf_main(actor_system&, const config& args) {
  ip_endpoint ep;
  auto addrs = net::ip::local_addresses("localhost");
  if (addrs.empty())
    exit("main could not get local addresses.");
  ep.address(addrs.front());
  auto ret = net::make_udp_datagram_socket(ep);
  if (!ret)
    exit("main make_udp_socket failed 1: ", ret.error());
  auto [sock, port] = *ret;
  std::cerr << "main created socket " << sock.id << std::endl;

  auto this_node_str = "udp://" + to_string(addrs.front()) + ":"
                       + std::to_string(port);
  auto this_node = make_uri(this_node_str);
  if (!this_node)
    exit("main make_uri failed", this_node.error());
  std::cerr << "main ping_node = " << to_string(this_node) << std::endl;
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::udp>();
  if (auto err = cfg.parse(0, nullptr))
    exit("main, could not parse config 1", err);
  put(cfg.content, "caf.middleman.this-node", *this_node);
  if (auto err = cfg.parse(0, nullptr))
    exit("main, could not parse config 2", err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::udp*>(mm.backend("udp"));
  // Overwrite the default initialized socket and port.
  std::cerr << "main emplacing socket " << sock.id << std::endl;
  auto err = backend.emplace(sock, port);
  if (!err)
    exit("main backend.emplace() failed: ", err.error());
  auto accumulator = sys.spawn(accumulator_actor, args.num_remote_nodes);
  auto ping = sys.spawn(ping_actor, accumulator, args.num_pings,
                        args.payload_size);
  mm.publish(ping, "ping");
  std::this_thread::sleep_for(500ms);
  std::vector<std::thread> threads;
  std::cerr << "starting remote node now!" << std::endl;
  std::vector<net::socket_guard<net::udp_datagram_socket>> sockets;
  for (size_t i = 0; i < args.num_remote_nodes; ++i) {
    std::cerr << "making datagram_socket for ep = " << to_string(ep)
              << std::endl;
    auto ret = net::make_udp_datagram_socket(ep, true);
    if (!ret)
      exit("main make_datagram_socket failed 2", ret.error());
    auto [sock, port] = *ret;
    std::cerr << "main created socket " << ret->first.id << std::endl;
    auto pong_id = make_uri("udp://" + to_string(addrs.front()) + ":"
                            + std::to_string(port));
    if (!pong_id)
      exit("main make_uri failed", pong_id.error());
    std::cerr << "ping_id = " << to_string(*this_node)
              << " pong_id = " << to_string(*pong_id) << std::endl;
    std::cerr << "main passing to thread socket " << sock.id << std::endl;
    auto f = [pong_id = *pong_id, this_node_str, sock = sock, port = port]() {
      net_run_source_node(pong_id, this_node_str, sock, port);
    };
    threads.emplace_back(f);
  }
  for (auto& t : threads)
    t.join();
  std::cerr << std::endl;
}

} // namespace

CAF_MAIN(net::middleman)
