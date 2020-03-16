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
#include <utility.hpp>
#include <vector>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "caf/io/network/default_multiplexer.hpp"
#include "caf/io/network/scribe_impl.hpp"
#include "caf/net/backend/test.hpp"
#include "caf/net/defaults.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"
#include "type_ids.hpp"

using namespace std;
using namespace caf;

namespace {

struct sink_state {
  const char* name = "sink";
  size_t tick_count = 0;
  size_t count = 0;

  void tick() {
    cout << count << ", ";
    count = 0;
  }
};

behavior sink(stateful_actor<sink_state>* self, actor src, size_t iterations) {
  self->send(self * src, start_atom_v);
  self->send(self, tick_atom_v);
  return {
    [=](tick_atom) {
      self->delayed_send(self, chrono::seconds(1), tick_atom_v);
      self->state.tick();
      if (self->state.tick_count++ >= iterations) {
        self->send(src, done_atom_v);
        cout << endl;
        self->quit();
      }
    },
    [=](const stream<std::vector<uint64_t>>& in) {
      return self->make_sink(
        // input stream
        in,
        // initialize state
        [](unit_t&) {
          // nop
        },
        // processing step
        [=](unit_t&, std::vector<uint64_t>) { ++self->state.count; },
        // cleanup
        [](unit_t&) {
          // nop
        });
    },
  };
}

struct config : actor_system_config {
  config() {
    init_global_meta_objects<caf_net_benchmark_type_ids>();
    caf::io::middleman::init_global_meta_objects();
    opt_group{custom_options_, "global"}
      .add(mode, "mode,m", "one of 'local', 'io', or 'net'")
      .add(host, "host,H", "host to connect to")
      .add(port, "port,p", "port to connect to")
      .add(iterations, "iterations,i",
           "number of iterations that should be run")
      .add(data_size, "data-size,d", "size of messages to send");
    earth_id = *make_uri("test://earth");
    mars_id = *make_uri("test://mars");
    load<net::middleman, net::backend::test>();
    set("logger.file-name", "source.log");
    put(content, "middleman.this-node", mars_id);
  }

  std::string mode;
  std::string host = "localhost";
  uint16_t port = 8080;
  size_t iterations = 10;
  size_t data_size = 1;
  uri earth_id;
  uri mars_id;
};

net::socket_guard<net::tcp_stream_socket>
connect(const std::string& host, uint16_t port, actor_system& sys) {
  using namespace caf::net;
  uri::authority_type dst;
  dst.port = port;
  dst.host = host;
  cerr << "connecting to " << to_string(dst) << endl;
  if (auto socket = make_connected_tcp_stream_socket(dst); !socket) {
    cerr << sys.render(socket.error());
    return make_socket_guard(tcp_stream_socket{invalid_socket_id});
  } else {
    cerr << "connected socket " << socket->id << endl;
    return make_socket_guard(*socket);
  }
}

void run_io_client(actor_system& sys, const config& cfg) {
  using io::network::scribe_impl;
  auto sock = connect(cfg.host, cfg.port, sys);
  if (sock.socket() == net::invalid_socket)
    return;
  auto& mm = sys.middleman();
  auto& mpx = dynamic_cast<io::network::default_multiplexer&>(mm.backend());
  io::scribe_ptr scribe = make_counted<scribe_impl>(mpx, sock.release().id);
  auto bb = mm.named_broker<io::basp_broker>("BASP");
  scoped_actor self{sys};
  self->request(bb, infinite, connect_atom_v, move(scribe), uint16_t{8080})
    .receive(
      [&](node_id&, strong_actor_ptr& ptr, set<string>&) {
        if (ptr == nullptr) {
          cerr << "ERROR: could not get a handle to remote source\n";
          return;
        }
        sys.spawn(sink, actor_cast<actor>(ptr), cfg.iterations);
      },
      [&](error& err) { cerr << "ERROR: " << sys.render(err) << endl; });
}

void run_net_client(actor_system& sys, const config& cfg) {
  using namespace caf::net;
  auto sock = connect(cfg.host, cfg.port, sys);
  if (sock.socket() == invalid_socket)
    return;
  auto& mm = sys.network_manager();
  auto& backend = *dynamic_cast<net::backend::test*>(mm.backend("test"));
  backend.emplace(make_node_id(cfg.earth_id), {}, sock.release());
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
  switch (convert(cfg.mode)) {
    case bench_mode::io: {
      cerr << "run in 'ioBench' client mode " << endl;
      run_io_client(sys, cfg);
      break;
    }
    case bench_mode::net: {
      cerr << "run in 'netBench' client mode " << endl;
      run_net_client(sys, cfg);
      break;
    }
    default:
      cerr << "mode is invalid: " << cfg.mode << endl;
      break;
  }
}

} // namespace

CAF_MAIN(io::middleman)
