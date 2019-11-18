/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2017                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
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
#include <cstring>
#include <iostream>

#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/io/network/scribe_impl.hpp"
#include "caf/io/scribe.hpp"
#include "caf/net/backend/test.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/stream_socket.hpp"

using namespace std;
using namespace caf;

namespace {

using start_atom = atom_constant<atom("start")>;
using ping_atom = atom_constant<atom("ping")>;
using pong_atom = atom_constant<atom("pong")>;
using hello_atom = atom_constant<atom("hello")>;
using done_atom = atom_constant<atom("done")>;
using io_bench_atom = atom_constant<atom("ioBench")>;
using net_bench_atom = atom_constant<atom("netBench")>;
using local_bench_atom = atom_constant<atom("localBench")>;

struct source_state {
  size_t messages = 0;
};

behavior source(stateful_actor<source_state>* self, actor other, actor,
                size_t num_messages) {
  return {
    [=](start_atom) {
      printf("START from: %s\n", to_string(self->current_sender()).c_str());
      self->send(other, hello_atom::value, self);
    },
    [=](hello_atom) { self->send(other, ping_atom::value); },
    [=](pong_atom) {
      if ((++self->state.messages % 1000) == 0)
        std::cout << "got " << self->state.messages << " messages" << std::endl;
      if (self->state.messages >= num_messages) {
        // self->send(listener, done_atom::value);
        self->quit();
        return;
      }
      self->send(other, ping_atom::value);
    },
  };
}

struct sink_state {
  actor other;
};

behavior sink(stateful_actor<sink_state>* self) {
  self->set_down_handler([=](const down_msg&) {
    std::cout << "sink down_handler" << std::endl;
    self->quit();
  });
  return {
    [=](hello_atom, actor other) {
      self->state.other = other;
      self->monitor(other);
      self->send(other, hello_atom::value);
    },
    [=](ping_atom) { self->send(self->state.other, pong_atom::value); },
  };
}

struct config : actor_system_config {
  config() {
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'local', 'ioBench', or 'netBench'")
      .add(num_stages, "num-stages,n",
           "number of stages after source / before sink")
      .add(num_messages, "num-messages,M", "number of messages to send");
    add_message_type<string>("string");
    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    put(content, "middleman.this-node", earth_id);
    load<net::middleman, net::backend::test>();
    set("logger.file-verbosity", atom("trace"));
    set("logger.file-name", "source.log");
  }

  int num_messages = 10000;
  int num_stages = 0;
  atom_value mode;
  uri earth_id;
  uri mars_id;
};

void io_run_source(net::stream_socket first, net::stream_socket second,
                   size_t num_messages) {
  using namespace std::chrono;
  actor_system_config cfg;
  cfg.add_message_type<string>("string");
  cfg.load<io::middleman>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-verbosity", atom("trace"));
  cfg.set("logger.file-name", "sink.log");
  actor_system sys{cfg};
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, second.id);
  auto bb = mm.named_broker<io::basp_broker>(atom("BASP"));
  scoped_actor self{sys};
  milliseconds start;
  actor sink;
  self
    ->request(bb, infinite, connect_atom::value, std::move(scribe),
              uint16_t{8080})
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, std::set<std::string>& xs) {
        if (ptr == nullptr) {
          std::cerr << "ERROR: could not get a handle to remote source\n";
          return;
        }
        auto src = sys.spawn(source, actor_cast<actor>(ptr), self,
                             num_messages);
        start = duration_cast<milliseconds>(
          system_clock::now().time_since_epoch());
        self->send(src, start_atom::value);
      },
      [&](error& err) {
        std::cerr << "ERROR: " << sys.render(err) << std::endl;
      });
  self->await_all_other_actors_done();
  auto end = duration_cast<milliseconds>(
    system_clock::now().time_since_epoch());
  std::cout << (end - start).count() << "ms" << std::endl;
}

void net_run_source(net::stream_socket first, net::stream_socket second,
                    size_t num_messages) {
  using namespace std::chrono;
  auto mars_id = *make_uri("test://mars");
  auto earth_id = *make_uri("test://earth");
  actor_system_config cfg;
  cfg.add_message_type<string>("string");
  cfg.load<net::middleman, net::backend::test>();
  cfg.parse(0, nullptr);
  cfg.set("logger.file-verbosity", atom("trace"));
  cfg.set("logger.file-name", "sink.log");
  put(cfg.content, "middleman.this-node", mars_id);
  cfg.parse(0, nullptr);
  actor_system sys{cfg};
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  backend.emplace(make_node_id(earth_id), second, first);
  auto locator = *make_uri("test://earth/name/source");
  scoped_actor self{sys};
  puts("resolve locator");
  mm.resolve(locator, self);
  milliseconds start;
  actor sink;
  self->receive([&](strong_actor_ptr& ptr, const std::set<std::string>&) {
    printf("got soure: %s -> run\n", to_string(ptr).c_str());
    auto src = sys.spawn(source, actor_cast<actor>(ptr), self, num_messages);
    start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    self->send(src, start_atom::value);
  });
  self->await_all_other_actors_done();
  auto end = duration_cast<milliseconds>(
    system_clock::now().time_since_epoch());
  std::cout << (end - start).count() << "ms" << std::endl;
  abort();
}

void caf_main(actor_system& sys, const config& cfg) {
  switch (static_cast<uint64_t>(cfg.mode)) {
    case io_bench_atom::uint_value(): {
      puts("run in 'ioBench' mode");
      auto sockets = *net::make_stream_socket_pair();
      printf("sockets: %d, %d\n", sockets.first.id, sockets.second.id);
      auto src = sys.spawn(sink);
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sockets.first.id);
      auto bb = mm.named_broker<io::basp_broker>(atom("BASP"));
      anon_send(bb, publish_atom::value, std::move(scribe), uint16_t{8080},
                actor_cast<strong_actor_ptr>(src), std::set<std::string>{});
      io_run_source(sockets.first, sockets.second, cfg.num_messages);
      break;
    }
    case net_bench_atom::uint_value(): {
      puts("run in 'netBench' mode");
      auto num_messages = cfg.num_messages;
      auto sockets = *net::make_stream_socket_pair();
      printf("sockets: %d, %d\n", sockets.first.id, sockets.second.id);
      auto src = sys.spawn(sink);
      sys.registry().put(atom("source"), src);
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
      backend.emplace(make_node_id(cfg.mars_id), sockets.first, sockets.second);
      puts("spin up second actor sytem");
      net_run_source(sockets.first, sockets.second, num_messages);
      break;
    }
  }
}

} // namespace

CAF_MAIN(io::middleman)
