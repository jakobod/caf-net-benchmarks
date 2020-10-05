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
#include <numeric>
#include <utility>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/defaults.hpp"
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/io/network/scribe_impl.hpp"
#include "caf/io/scribe.hpp"
#include "caf/net/backend/tcp.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/ip.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

using namespace caf;
using namespace std::chrono;

namespace {

using payload = std::vector<caf::byte>;

// -- source actor -------------------------------------------------------------

struct source_state {
  payload p;
  std::chrono::system_clock::time_point begin;
  size_t streaming_amount = 0;
};

behavior source_actor(stateful_actor<source_state>* self, actor sink,
                      size_t payload_size, size_t streaming_amount) {
  return {
    [=](init_atom init) {
      self->state.streaming_amount = streaming_amount;
      self->state.p.resize(payload_size);
      self->state.begin = std::chrono::system_clock::now();
      self->send(sink, init, self, streaming_amount);
      self->send(self, send_atom_v);
    },
    [=](send_atom) {
      while (self->state.streaming_amount > 0) {
        self->send(sink, self->state.p);
        self->state.streaming_amount -= payload_size;
      }
    },
    [=](done_atom) {
      auto end = std::chrono::system_clock::now();
      auto begin = self->state.begin;
      auto duration = end - begin;
      std::cerr << duration_cast<microseconds>(duration).count() << "us "
                << std::endl;
      self->quit();
    },
    [](unit_t) {
      // nop
    },
  };
}

// -- sink actor ---------------------------------------------------------------

struct sink_state {
  actor source;
  size_t streaming_amount = 0;
  size_t received_bytes = 0;
  size_t ticks = 0;
};

behavior sink_actor(stateful_actor<sink_state>* self) {
  self->set_exit_handler([=](const exit_msg&) { self->quit(); });
  return {
    [=](init_atom, const actor& source, size_t streaming_amount) {
      self->link_to(source);
      self->state.source = source;
      self->state.streaming_amount = streaming_amount;
    },
    [=](const payload& p) {
      self->state.received_bytes += p.size();
      if (self->state.received_bytes >= self->state.streaming_amount)
        self->send(self->state.source, done_atom_v);
    },
  };
}

// -- config -------------------------------------------------------------------

struct config : actor_system_config {
  config() {
    init_global_meta_objects<caf::id_block::caf_net_benchmark>();
    io::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'local', 'ioBench', or 'netBench'")
      .add(payload_size, "size,s", "size of the payload in byte")
      .add(streaming_amount, "amount,a", "amount of bytes to transmit")
      .add(is_server, "server,S", "toggle server mode")
      .add(host, "host,H", "host to connect to")
      .add(port, "port,p", "port to connect to");

    earth_id = *make_uri("tcp://earth");
    put(content, "caf.middleman.this-node", earth_id);
    put(content, "caf.scheduler.max-threads", 1);
    load<net::middleman, net::backend::tcp>();
  }

  bool is_server = false;
  std::string host = "";
  uint16_t port = 0;

  size_t streaming_amount = 1024;
  size_t payload_size = 1;
  std::string mode = "netBench";
  uri earth_id;
};

// -- IO client and server
// -----------------------------------------------------

void run_io_server(actor_system& sys, const config& cfg) {
  using namespace caf::io;
  using io::network::scribe_impl;
  printf("%s\n", __FUNCTION__);
  auto sock = accept();
  if (sock.socket() == net::invalid_socket)
    exit("could not accept connection..", sec::runtime_error);
  std::cout << "*** Accepted connection. socket.id = "
            << std::to_string(sock.socket().id) << std::endl;
  auto sink = sys.spawn(sink_actor);
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<network::default_multiplexer&>(mm.backend());
  auto bb = mm.named_broker<basp_broker>("BASP");
  scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock.release().id);
  anon_send(bb, publish_atom_v, std::move(scribe), uint16_t(8080),
            actor_cast<strong_actor_ptr>(sink), std::set<std::string>{});
}

void run_io_client(actor_system&, const config& args) {
  using namespace caf::io;
  printf("%s\n", __FUNCTION__);
  auto sock = connect(args.host, args.port);
  if (sock.socket() == net::invalid_socket)
    exit("failed to create connected socket", sec::runtime_error);
  std::cout << "*** Established connection. socket.id = "
            << std::to_string(sock.socket().id) << std::endl;
  actor_system_config cfg;
  cfg.load<middleman>();
  if (auto err = cfg.parse(0, nullptr))
    exit("parsing config failed", err);
  put(cfg.content, "caf.scheduler.max-threads", 1);
  actor_system sys{cfg};
  using network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<network::default_multiplexer&>(mm.backend());
  scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock.release().id);
  auto bb = mm.named_broker<basp_broker>("BASP");
  scoped_actor self{sys};
  self->request(bb, infinite, connect_atom_v, std::move(scribe), uint16_t(8080))
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, std::set<std::string>&) {
        if (ptr == nullptr)
          exit("ERROR: could not get a handle to remote source");
        auto source = sys.spawn(source_actor, actor_cast<actor>(ptr),
                                args.payload_size, args.streaming_amount);
        anon_send(source, init_atom_v);
      },
      [&](error& err) { exit(err); });
}

// -- NET client and server ----------------------------------------------------

void run_net_server(actor_system& sys, const config& cfg) {
  using namespace caf::net;
  printf("%s\n", __FUNCTION__);
  auto sock = accept();
  if (sock.socket() == invalid_socket)
    exit("could not accept connection", sec::runtime_error);
  std::cout << "*** Accepted connection. socket.id = "
            << std::to_string(sock.socket().id) << std::endl;
  auto sink = sys.spawn(sink_actor);
  auto source_id = *make_uri(std::string("tcp://source-node"));
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<backend::tcp*>(mm.backend("tcp"));
  sys.registry().put(std::string("sink"), sink);
  backend.emplace(make_node_id(source_id), sock.release());
}

void run_net_client(actor_system&, const config& args) {
  using namespace caf::net;
  printf("%s\n", __FUNCTION__);
  auto sock = connect(args.host, args.port);
  if (sock.socket() == invalid_socket)
    exit("failed to create connected socket", sec::runtime_error);
  std::cout << "*** Established connection. socket.id = "
            << std::to_string(sock.socket().id) << std::endl;
  auto source_id = *make_uri(std::string("tcp://source-node"));
  auto sink_locator = *make_uri(std::string("tcp://earth/name/sink"));
  actor_system_config cfg;
  cfg.load<middleman, backend::tcp>();
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  put(cfg.content, "caf.middleman.this-node", source_id);
  put(cfg.content, "caf.scheduler.max-threads", 1);
  if (auto err = cfg.parse(0, nullptr))
    exit(err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<backend::tcp*>(mm.backend("tcp"));
  auto ret = backend.emplace(make_node_id(*sink_locator.authority_only()),
                             sock.release());
  if (!ret)
    exit("emplace failed with err: ", ret.error());
  auto sink = mm.remote_actor(sink_locator);
  if (!sink)
    exit("remote actor failed: ", sink.error());
  scoped_actor self{sys};
  auto source = sys.spawn(source_actor, *sink, args.payload_size,
                          args.streaming_amount);
  anon_send(source, init_atom_v);
}

void caf_main(actor_system& sys, const config& cfg) {
  using key_type = std::pair<bench_mode, bool>;
  using function_type = std::function<void(actor_system&, const config&)>;
  std::map<key_type, function_type> functions{
    {std::make_pair(bench_mode::io, true), run_io_server},
    {std::make_pair(bench_mode::io, false), run_io_client},
    {std::make_pair(bench_mode::net, true), run_net_server},
    {std::make_pair(bench_mode::net, false), run_net_client}};
  if (cfg.mode != "netBench" && cfg.mode != "ioBench")
    exit("benchmark mode was not set", sec::runtime_error);
  auto f = functions.at(std::make_pair(convert(cfg.mode), cfg.is_server));
  f(sys, cfg);
}

} // namespace

CAF_MAIN(io::middleman)
