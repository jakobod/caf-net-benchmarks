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
#include <iostream>
#include <numeric>
#include <queue>

#include "accumulator.hpp"
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

using namespace caf;
using namespace std::chrono;

namespace {

using payload = std::vector<caf::byte>;

// -- source actor -------------------------------------------------------------

struct source_state {
  std::vector<payload> payloads;

  void fill_payloads(size_t byte_amount, size_t message_size) {
    while (byte_amount > 0) {
      auto size = std::min(byte_amount, message_size);
      payloads.emplace_back(payload(size));
      byte_amount -= size;
    }
  }
};

behavior source_actor(stateful_actor<source_state>* self, actor sink,
                      size_t message_size, size_t streaming_amount) {
  self->set_exit_handler([=](const exit_msg&) { self->quit(); });
  self->link_to(sink);
  return {
    [=](init_atom init) {
      self->state.fill_payloads(streaming_amount, message_size);
      self->send(sink, init, streaming_amount);
    },
    [=](send_atom) {
      auto& payloads = self->state.payloads;
      for (size_t i = 0; i < payloads.size(); ++i)
        self->send(sink, std::move(payloads[i]));
    },
  };
} // namespace

// -- sink actor ---------------------------------------------------------------

struct sink_state {
  actor source;
  size_t streaming_amount = 0;
  size_t received_bytes = 0;
  size_t ticks = 0;
};

behavior sink_actor(stateful_actor<sink_state>* self, actor accumulator) {
  self->set_exit_handler([=](const exit_msg&) { self->quit(); });
  self->link_to(accumulator);
  return {
    [=](init_atom, size_t streaming_amount) {
      self->state.streaming_amount = streaming_amount;
      self->send(accumulator, init_atom_v);
      return send_atom_v;
    },
    [=](const payload& p) {
      self->state.received_bytes += p.size();
      if (self->state.received_bytes >= self->state.streaming_amount)
        self->send(accumulator, done_atom_v);
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
      .add(streaming_amount, "amount,a",
           "amount of bytes that should be transmitted")
      .add(message_size, "size,s", "size of the payload in byte");

    earth_id = *make_uri("tcp://earth");
    put(content, "caf.middleman.this-node", earth_id);
    put(content, "caf.scheduler.max-threads", 1);
    load<net::middleman, net::backend::tcp>();
  }

  size_t message_size = 1;
  size_t num_remote_nodes = 1;
  size_t streaming_amount = 1024;
  std::string mode = "netBench";
  uri earth_id;
};

void io_run_source(net::stream_socket sock, uint16_t port,
                   size_t streaming_amount, size_t message_size) {
  actor_system_config cfg;
  cfg.load<io::middleman>();
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  put(cfg.content, "caf.scheduler.max-threads", 1);
  actor_system sys{cfg};
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock.id);
  auto bb = mm.named_broker<io::basp_broker>("BASP");
  scoped_actor self{sys};
  self->request(bb, infinite, connect_atom_v, std::move(scribe), port)
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, std::set<std::string>&) {
        if (ptr == nullptr)
          exit("ERROR: could not get a handle to remote source");
        auto source = sys.spawn(source_actor, actor_cast<actor>(ptr),
                                streaming_amount, message_size);
        anon_send(source, init_atom_v);
      },
      [&](error& err) { exit(err); });
}

void net_run_source(net::stream_socket sock, size_t id, size_t streaming_amount,
                    size_t message_size) {
  auto source_id = *make_uri(std::string("tcp://source") + std::to_string(id));
  auto sink_locator
    = *make_uri(std::string("tcp://earth/name/sink") + std::to_string(id));
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::tcp>();
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  put(cfg.content, "caf.middleman.this-node", source_id);
  put(cfg.content, "caf.scheduler.max-threads", 1);
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
  auto ret = backend.emplace(make_node_id(*sink_locator.authority_only()),
                             sock);
  if (!ret)
    exit("emplace failed with err: ", ret.error());
  std::cerr << "resolve locator " << to_string(sink_locator) << std::endl;
  auto sink = mm.remote_actor(sink_locator, 2s);
  if (!sink)
    exit(sink.error());
  scoped_actor self{sys};
  auto source = sys.spawn(source_actor, *sink, streaming_amount, message_size);
  anon_send(source, init_atom_v);
}

void caf_main(actor_system& sys, const config& cfg) {
  std::vector<std::thread> threads;
  auto accumulator = sys.spawn(accumulator_actor, cfg.num_remote_nodes);
  switch (convert(cfg.mode)) {
    case bench_mode::io: {
      std::cerr << "run in 'ioBench' mode" << std::endl;
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      auto bb = mm.named_broker<io::basp_broker>("BASP");
      for (size_t port = 0; port < cfg.num_remote_nodes; ++port) {
        auto p = *net::make_stream_socket_pair();
        io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, p.first.id);
        auto sink = sys.spawn(sink_actor, accumulator);
        anon_send(bb, publish_atom_v, std::move(scribe), uint16_t(8080 + port),
                  actor_cast<strong_actor_ptr>(sink), std::set<std::string>{});
        auto f = [=, &cfg]() {
          io_run_source(p.second, port, cfg.streaming_amount, cfg.message_size);
        };
        threads.emplace_back(f);
      }
      break;
    }
    case bench_mode::net: {
      std::cerr << "run in 'netBench' mode " << std::endl;
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
      for (size_t node = 0; node < cfg.num_remote_nodes; ++node) {
        auto source_id
          = *make_uri(std::string("tcp://source") + std::to_string(node));
        auto sink = sys.spawn(sink_actor, accumulator);
        sys.registry().put(std::string("sink") + std::to_string(node), sink);
        auto sockets = *make_connected_tcp_socket_pair();
        backend.emplace(make_node_id(source_id), sockets.first);
        auto f = [=, &cfg]() {
          net_run_source(sockets.second, node, cfg.streaming_amount,
                         cfg.message_size);
        };
        threads.emplace_back(f);
      }
      break;
    }
    default:
      std::cerr << "mode is invalid: " << cfg.mode << std::endl;
  }

  for (auto& t : threads)
    t.join();
}

} // namespace

CAF_MAIN(io::middleman)
