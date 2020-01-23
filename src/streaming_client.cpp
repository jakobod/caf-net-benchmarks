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
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/net/backend/test.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/doorman.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"
#include "dummy_application.hpp"

using namespace std;
using namespace caf;

namespace {

using start_atom = atom_constant<atom("start")>;
using tick_atom = atom_constant<atom("tick")>;
using done_atom = atom_constant<atom("done")>;
using io_bench_atom = atom_constant<atom("ioBench")>;
using net_bench_atom = atom_constant<atom("netBench")>;
using local_bench_atom = atom_constant<atom("localBench")>;

struct sink_state {
  const char* name = "sink";
  size_t tick_count = 0;
  uint64_t sequence = 0;
  size_t count = 0;

  void tick() {
    cout << count << ", ";
    count = 0;
  }
};

behavior sink(stateful_actor<sink_state>* self, actor src, size_t iterations) {
  self->send(self * src, start_atom::value);
  self->send(self, tick_atom::value);
  return {
    [=](tick_atom) {
      self->delayed_send(self, chrono::seconds(1), tick_atom::value);
      self->state.tick();
      if (self->state.tick_count++ >= iterations) {
        self->send(src, done_atom::value);
        cout << endl;
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
      .add(host, "host,H", "host to connect to")
      .add(port, "port,p", "port to connect to")
      .add(iterations, "iterations,i",
           "number of iterations that should be run");

    add_message_type<uint64_t>("uint64_t");
    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    load<net::middleman, net::backend::test>();
    set("logger.file-verbosity", atom("trace"));
    set("logger.file-name", "source.log");
    put(content, "middleman.this-node", mars_id);
  }

  std::string host = "localhost";
  uint16_t port = 0;
  size_t iterations = 10;
  atom_value mode;
  uri earth_id;
  uri mars_id;
};

void run_net_client(actor_system& sys, const config& cfg) {
  using namespace caf::net;
  uri::authority_type dst;
  dst.port = cfg.port;
  dst.host = cfg.host;
  cout << "connecting to " << to_string(dst) << endl;
  auto conn = make_socket_guard(*make_connected_tcp_stream_socket(dst));
  cout << "connected" << endl;
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  backend.emplace(make_node_id(cfg.earth_id), {}, conn.release());
  auto locator = *make_uri("test://earth/name/source");
  scoped_actor self{sys};
  cerr << "resolve locator " << endl;
  mm.resolve(locator, self);
  actor sink_actor;
  self->receive([&](strong_actor_ptr& ptr, const set<string>&) {
    cerr << "got soure: " << to_string(ptr).c_str() << " -> run" << endl;
    sink_actor = sys.spawn(sink, actor_cast<actor>(ptr), cfg.iterations);
  });
}

void caf_main(actor_system& sys, const config& cfg) {
  switch (static_cast<uint64_t>(cfg.mode)) {
    case net_bench_atom::uint_value(): {
      cerr << "run in 'netBench' client mode " << endl;
      run_net_client(sys, cfg);
      break;
    }
    default:
      cerr << "No mode specified!" << endl;
      break;
  }
}

} // namespace

CAF_MAIN(io::middleman)
