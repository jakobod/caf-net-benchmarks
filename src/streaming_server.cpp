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
#include "caf/io/network/scribe_impl.hpp"
#include "caf/io/scribe.hpp"
#include "caf/net/backend/test.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/multiplexer.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_accept_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"

using namespace std;
using namespace caf;

namespace {

using start_atom = atom_constant<atom("start")>;
using done_atom = atom_constant<atom("done")>;
using io_bench_atom = atom_constant<atom("ioBench")>;
using net_bench_atom = atom_constant<atom("netBench")>;
using local_bench_atom = atom_constant<atom("localBench")>;

struct source_state {
  uint64_t current = 0;
};

behavior source(stateful_actor<source_state>* self) {
  using namespace std::chrono;
  return {
    [=](done_atom) { self->quit(); },
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
        },
        // check whether we reached the end
        [=](const unit_t&) { return false; });
    },
  };
}

struct config : actor_system_config {
  config() {
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'local', 'ioBench', or 'netBench'")
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
    put(content, "middleman.this-node", earth_id);
  }

  std::string host = "localhost";
  uint16_t port = 8080;
  int num_stages = 0;
  size_t iterations = 10;
  atom_value mode;
  uri earth_id;
  uri mars_id;
};

net::socket_guard<net::tcp_stream_socket> accept(uint16_t port) {
  uri::authority_type auth;
  auth.port = port;
  auth.host = "0.0.0.0"s;
  auto acceptor = *net::make_tcp_accept_socket(auth, false);
  port = *local_port(net::socket_cast<net::network_socket>(acceptor));
  auto acceptor_guard = net::make_socket_guard(acceptor);
  cout << "opened acceptor on port " << port << endl;
  return net::make_socket_guard(*net::accept(acceptor));
}

void run_io_server(actor_system& sys, const config& cfg) {
  auto sock = accept(cfg.port);
  auto src = sys.spawn(source);
  using io::network::scribe_impl;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock.release().id);
  auto bb = mm.named_broker<io::basp_broker>(atom("BASP"));
  anon_send(bb, publish_atom::value, move(scribe), uint16_t{8080},
            actor_cast<strong_actor_ptr>(src), set<string>{});
}

void run_net_server(actor_system& sys, const config& cfg) {
  using namespace caf::net;
  auto sock = accept(cfg.port);
  scoped_actor self{sys};
  auto src = sys.spawn(source);
  sys.registry().put(atom("source"), src);
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  backend.emplace(make_node_id(cfg.mars_id), {}, sock.release());
}

void caf_main(actor_system& sys, const config& cfg) {
  auto workers = get_or(sys.config(), "middleman.serializing_workers",
                        defaults::middleman::serializing_workers);
  cerr << "using " << workers << " workers for serializing";
  workers = get_or(sys.config(), "middleman.workers",
                   defaults::middleman::workers);
  cerr << "using " << workers << " workers for deserializing" << endl;
  switch (static_cast<uint64_t>(cfg.mode)) {
    case io_bench_atom::uint_value(): {
      cerr << "run in 'ioBench' server mode " << endl;
      run_io_server(sys, cfg);
      break;
    }
    case net_bench_atom::uint_value(): {
      cerr << "run in 'netBench' server mode " << endl;
      run_net_server(sys, cfg);
      break;
    }
    default:
      cerr << "No mode specified!" << endl;
      break;
  }
}

} // namespace

CAF_MAIN(io::middleman)
