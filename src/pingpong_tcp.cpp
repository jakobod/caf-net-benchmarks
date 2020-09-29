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
#include "caf/byte.hpp"
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

using namespace caf;
using namespace std::chrono;

namespace {

using payload = std::vector<caf::byte>;

struct tick_state {
  void tick() {
    std::cout << count << ", ";
    count = 0;
  }

  std::vector<actor> sinks;
  size_t count = 0;
  size_t iterations = 0;

  template <class Func>
  void for_each(Func f) {
    for (const auto& sink : sinks)
      f(sink);
  }
};

behavior ping_actor(stateful_actor<tick_state>* self, size_t num_remote_nodes,
                    size_t iterations, size_t num_pings, size_t payload_size) {
  return {
    [=](hello_atom, const actor& sink) {
      self->state.sinks.push_back(sink);
      if (self->state.sinks.size() >= num_remote_nodes) {
        std::cerr << "STARTING BENCHMARK NOW" << std::endl;
        payload p(payload_size);
        self->state.for_each([&](const auto& sink) {
          for (size_t i = 0; i < num_pings; ++i)
            self->send(sink, p);
        });
        self->delayed_send(self, seconds(1), tick_atom_v);
      }
    },
    [=](tick_atom) {
      self->delayed_send(self, seconds(1), tick_atom_v);
      self->state.tick();
      if (++self->state.iterations >= iterations) {
        std::cout << std::endl;
        self->quit();
      }
    },
    [=](payload& p) {
      self->state.count++;
      return p;
    },
  };
}

behavior pong_actor(event_based_actor* self, const actor& source) {
  self->set_exit_handler([=](const exit_msg&) { self->quit(); });
  return {
    [=](start_atom) {
      self->link_to(source);
      self->send(source, hello_atom_v, self);
    },
    [=](payload& p) { return p; },
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
      .add(num_pings, "pings,p", "number of pings to send")
      .add(payload_size, "size,s", "size of the payload");
    source_id = *make_uri("tcp://source");
    put(content, "caf.middleman.this-node", source_id);
    put(content, "caf.scheduler.max-threads", 1);
    load<net::middleman, net::backend::tcp>();
  }

  size_t payload_size = 1;
  size_t iterations = 10;
  size_t num_remote_nodes = 1;
  size_t num_pings = 1;
  std::string mode = "netBench";
  uri source_id;
};

void io_run_node(uint16_t port, int sock) {
  actor_system_config cfg;
  cfg.load<io::middleman>();
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  put(cfg.content, "caf.scheduler.max-threads", 1);
  actor_system sys{cfg};
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock);
  auto bb = mm.named_broker<io::basp_broker>("BASP");
  scoped_actor self{sys};
  self->request(bb, infinite, connect_atom_v, std::move(scribe), port)
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, std::set<std::string>&) {
        if (ptr == nullptr)
          exit("could not get a handle to remote source");
        auto sink = sys.spawn(pong_actor, actor_cast<actor>(ptr));
        anon_send(sink, start_atom_v);
      },
      [&](error& err) { exit("failed to resolve", err); });
}

void net_run_node(uri id, net::stream_socket sock) {
  actor_system_config cfg;
  cfg.load<net::middleman, net::backend::tcp>();
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  put(cfg.content, "caf.middleman.this-node", id);
  put(cfg.content, "caf.scheduler.max-threads", 1);
  if (auto err = cfg.parse(0, nullptr))
    exit("could not parse config", err);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
  auto source_id = *make_uri("tcp://source/name/source");
  auto ret = backend.emplace(make_node_id(*source_id.authority_only()), sock);
  if (!ret)
    exit("emplace failed", ret.error());
  auto source = mm.remote_actor(source_id);
  if (!source)
    exit("remote_actor failed", source.error());
  auto sink = sys.spawn(pong_actor, *source);
  anon_send(sink, start_atom_v);
  scoped_actor self{sys};
  self->await_all_other_actors_done();
}

void caf_main(actor_system& sys, const config& cfg) {
  std::vector<std::thread> threads;
  auto src = sys.spawn(ping_actor, cfg.num_remote_nodes, cfg.iterations,
                       cfg.num_pings, cfg.payload_size);
  std::cout << cfg.num_remote_nodes << ", ";
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
        anon_send(bb, publish_atom_v, std::move(scribe), uint16_t(8080 + port),
                  actor_cast<strong_actor_ptr>(src), std::set<std::string>{});
        auto f = [=]() { io_run_node(port, p.second.id); };
        threads.emplace_back(f);
      }
      break;
    }
    case bench_mode::net: {
      std::cerr << "run in 'netBench' mode " << std::endl;
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::tcp*>(mm.backend("tcp"));
      mm.publish(src, "source");
      for (size_t i = 0; i < cfg.num_remote_nodes; ++i) {
        auto p = *make_connected_tcp_socket_pair();
        auto sink_id = *make_uri(std::string("tcp://sink") + std::to_string(i));
        backend.emplace(make_node_id(sink_id), p.first);
        auto f = [=]() { net_run_node(sink_id, p.second); };
        threads.emplace_back(f);
      }
      break;
    }
    default:
      exit(std::string("invalid mode: \"") + cfg.mode + "\"");
  }
  for (auto& t : threads)
    t.join();
  std::cerr << std::endl;
}

} // namespace

CAF_MAIN(io::middleman)
