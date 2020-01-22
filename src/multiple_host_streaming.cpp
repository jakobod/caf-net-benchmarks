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
#include "caf/net/doorman.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/multiplexer.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_accept_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"
#include "dummy_application.hpp"
#include "dummy_application_factory.hpp"
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
      .add(is_server, "server,s", "toggle server mode")
      .add(host, "host,H", "host to connect to")
      .add(port, "port,p", "port to connect to")
      .add(num_stages, "num-stages,n",
           "number of stages after source / before sink")
      .add(iterations, "iterations,i",
           "number of iterations that should be run");

    add_message_type<uint64_t>("uint64_t");
    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    load<net::middleman, net::backend::test>();
    set("logger.file-verbosity", atom("trace"));
    set("logger.file-name", "source.log");
    if (is_server) {
      put(content, "middleman.this-node", earth_id);
      cout << "SERVER" << endl;
    } else {
      cout << "CLIENT" << endl;
      put(content, "middleman.this-node", mars_id);
    }
  }

  bool is_server = false;
  std::string host = "localhost";
  uint16_t port = 0;
  int num_stages = 0;
  size_t iterations = 10;
  atom_value mode;
  uri earth_id;
  uri mars_id;
};

void run_net_server(actor_system& sys, const config& cfg) {
  using namespace caf::net;
  uri::authority_type auth;
  auth.port = 0;
  auth.host = "0.0.0.0"s;
  auto src = sys.spawn(source, false, cfg.iterations);
  sys.registry().put(atom("source"), src);
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  auto acceptor = *make_tcp_accept_socket(auth, false);
  auto port = *local_port(socket_cast<network_socket>(acceptor));
  auto acceptor_guard = make_socket_guard(acceptor);
  cout << "opened acceptor on port " << port << endl;
  auto& mpx = mm.mpx();
  auto mgr = make_endpoint_manager(
    mpx, sys,
    doorman<dummy_application_factory>{acceptor_guard.release(),
                                       dummy_application_factory{}});
  if (auto err = mgr->init())
    cerr << "mgr init failed: " << sys.render(err);
  scoped_actor self{sys};
  self->await_all_other_actors_done();
}

void run_net_client(actor_system& sys, const config& cfg) {
  using namespace caf::net;
  uri::authority_type dst;
  dst.port = cfg.port;
  dst.host = cfg.host;
  std::cout << "connecting to " << cfg.host << ":" << cfg.port << std::endl;
  auto conn = make_socket_guard(*make_connected_tcp_stream_socket(dst));
  cout << "connected" << endl;
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  backend.emplace(make_node_id(cfg.earth_id), stream_socket{}, conn.release());
  auto locator = *make_uri("test://earth/name/source");
  scoped_actor self{sys};
  cerr << "resolve locator " << endl;
  mm.resolve(locator, self);
  actor sink_actor;
  self->receive([&](strong_actor_ptr& ptr, const set<string>&) {
    cerr << "got soure: " << to_string(ptr).c_str() << " -> run" << endl;
    sink_actor = sys.spawn(sink, actor_cast<actor>(ptr), self, cfg.iterations);
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
    case net_bench_atom::uint_value(): {
      cerr << "run in 'netBench' mode " << endl;
      if (cfg.is_server)
        run_net_server(sys, cfg);
      else
        run_net_client(sys, cfg);
      break;
    }
    default:
      cerr << "No mode specified!" << endl;
  }
  cout << endl;
}

} // namespace

CAF_MAIN(io::middleman)
