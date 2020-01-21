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
#include "caf/net/backend/test.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/uri.hpp"
#include "utility.hpp"

using namespace std;
using namespace caf;

namespace {

using start_atom = atom_constant<atom("start")>;
using tick_atom = atom_constant<atom("tick")>;
using done_atom = atom_constant<atom("done")>;
using io_bench_atom = atom_constant<atom("ioBench")>;
using net_bench_atom = atom_constant<atom("netBench")>;
using local_bench_atom = atom_constant<atom("localBench")>;

struct tick_state {
  size_t count = 0;
  size_t tick_count = 0;

  void tick() {
    cout << count << ", ";
    count = 0;
  }
};

struct source_state : tick_state {
  const char* name = "source";
  uint64_t current = 0;
};

behavior source(stateful_actor<source_state>* self, bool print_rate,
                size_t iterations) {
  using namespace std::chrono;
  if (print_rate)
    self->delayed_send(self, seconds(1), tick_atom::value);
  return {
    [=](tick_atom) {
      self->delayed_send(self, seconds(1), tick_atom::value);
      self->state.tick();
      if (self->state.tick_count++ >= iterations) {
        self->quit();
      }
    },
    [=](start_atom) {
      cerr << "START from: " << to_string(self->current_sender()).c_str()
           << endl;
      return self->make_source(
        // initialize state
        [&](unit_t&) {
          // nop
        },
        // get next element
        [=](unit_t&, downstream<uint64_t>& out, size_t num) {
          for (size_t i = 0; i < num; ++i)
            out.push(self->state.current++);
          self->state.count += num;
        },
        // check whether we reached the end
        [=](const unit_t&) { return false; });
    },
  };
}

struct stage_state {
  const char* name = "stage";
};

behavior stage(stateful_actor<stage_state>* self) {
  return {
    [=](const stream<uint64_t>& in) {
      return self->make_stage(
        // input stream
        in,
        // initialize state
        [](unit_t&) {
          // nop
        },
        // processing step
        [=](unit_t&, downstream<uint64_t>& xs, uint64_t x) { xs.push(x); },
        // cleanup
        [](unit_t&) {
          // nop
        });
    },
  };
}

struct sink_state : tick_state {
  const char* name = "sink";
  uint64_t sequence = 0;
};

behavior sink(stateful_actor<sink_state>* self, actor src, actor listener,
              size_t iterations) {
  self->send(self * src, start_atom::value);
  self->delayed_send(self, chrono::seconds(1), tick_atom::value);
  return {
    [=](tick_atom) {
      self->delayed_send(self, chrono::seconds(1), tick_atom::value);
      self->state.tick();
      if (self->state.tick_count++ >= iterations) {
        self->send(listener, done_atom::value);
        self->quit();
      }
    },
    [=](const stream<uint64_t>& in) {
      return self->make_sink(
        // input stream
        in,
        // initialize state
        [](unit_t&) {
          // nop
        },
        // processing step
        [=](unit_t&, uint64_t x) {
          CAF_LOG_ERROR_IF(x != self->state.sequence,
                           "got x = " << x << " | expected: "
                                      << CAF_ARG2("x", self->state.sequence));
          self->state.sequence++;
          ++self->state.count;
        },
        // cleanup
        [](unit_t&) {
          // nop
        });
    },
  };
}

struct config : actor_system_config {
  config() {
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'local', 'ioBench', or 'netBench'")
      .add(num_stages, "num-stages,n",
           "number of stages after source / before sink")
      .add(iterations, "iterations,i",
           "number of iterations that should be run");

    add_message_type<uint64_t>("uint64_t");
    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    put(content, "middleman.this-node", earth_id);
    load<net::middleman, net::backend::test>();
    set("logger.file-verbosity", atom("trace"));
    set("logger.file-name", "source.log");
  }

  int num_stages = 0;
  size_t iterations = 10;
  atom_value mode;
  uri earth_id;
  uri mars_id;
};

void io_run_sink(net::stream_socket, net::stream_socket second,
                 size_t iterations) {
  actor_system_config cfg;
  cfg.add_message_type<uint64_t>("uint64_t");
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
  self->request(bb, infinite, connect_atom::value, move(scribe), uint16_t{8080})
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, set<string>&) {
        if (ptr == nullptr) {
          cerr << "ERROR: could not get a handle to remote source\n";
          return;
        }
        sys.spawn(sink, actor_cast<actor>(ptr), self, iterations);
      },
      [&](error& err) { cerr << "ERROR: " << sys.render(err) << endl; });
  self->receive([](done_atom) {});
}

void net_run_sink(net::stream_socket first, net::stream_socket second,
                  size_t iterations) {
  auto mars_id = *make_uri("test://mars");
  auto earth_id = *make_uri("test://earth");
  actor_system_config cfg;
  cfg.add_message_type<uint64_t>("uint64_t");
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
  cerr << "resolve locator " << endl;
  mm.resolve(locator, self);
  actor sink_actor;
  self->receive([&](strong_actor_ptr& ptr, const set<string>&) {
    cerr << "got soure: " << to_string(ptr).c_str() << " -> run" << endl;
    sink_actor = sys.spawn(sink, actor_cast<actor>(ptr), self, iterations);
  });
  self->receive([](done_atom) {
    cerr << "done" << endl;
    cout << endl;
  });
  anon_send_exit(sink_actor, exit_reason::user_shutdown);
}

void caf_main(actor_system& sys, const config& cfg) {
  auto add_stages = [&](actor hdl) {
    for (int i = 0; i < cfg.num_stages; ++i)
      hdl = sys.spawn(stage) * hdl;
    return hdl;
  };
  switch (static_cast<uint64_t>(cfg.mode)) {
    case local_bench_atom::uint_value(): {
      cerr << "run in 'localBench' mode" << endl;
      scoped_actor self{sys};
      sys.spawn(sink, add_stages(sys.spawn(source, false, cfg.iterations)),
                self, cfg.iterations);
      break;
    }
    case io_bench_atom::uint_value(): {
      cerr << "run in 'ioBench' mode" << endl;
      auto sockets = *make_connected_tcp_socket_pair();
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << endl;
      auto src = sys.spawn(source, false, cfg.iterations);
      using io::network::scribe_impl;
      auto& mm = sys.middleman();
      auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
      io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sockets.first.id);
      auto bb = mm.named_broker<io::basp_broker>(atom("BASP"));
      anon_send(bb, publish_atom::value, move(scribe), uint16_t{8080},
                actor_cast<strong_actor_ptr>(src), set<string>{});
      auto iterations = cfg.iterations;
      auto f = [sockets, iterations] {
        io_run_sink(sockets.first, sockets.second, iterations);
      };
      thread t{f};
      t.join();
      anon_send_exit(src, exit_reason::user_shutdown);
      break;
    }
    case net_bench_atom::uint_value(): {
      cerr << "run in 'netBench' mode " << endl;
      auto sockets = *make_connected_tcp_socket_pair();
      cerr << "sockets: " << sockets.first.id << ", " << sockets.second.id
           << endl;
      auto src = sys.spawn(source, false, cfg.iterations);
      sys.registry().put(atom("source"), src);
      auto& mm = sys.network_manager();
      auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
      backend.emplace(make_node_id(cfg.mars_id), sockets.first, sockets.second);
      cerr << "spin up second actor sytem" << endl;
      auto iterations = cfg.iterations;
      auto f = [sockets, iterations] {
        net_run_sink(sockets.first, sockets.second, iterations);
      };
      thread t{f};
      t.join();
      anon_send_exit(src, exit_reason::user_shutdown);
      break;
    }
  }
  cout << endl;
}

} // namespace

CAF_MAIN(io::middleman)
