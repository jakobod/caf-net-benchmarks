/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2019 Dominik Charousset                                     *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "utility.hpp"

#include <cstdlib>
#include <string>
#include <utility>

#include "caf/error.hpp"
#include "caf/expected.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_accept_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"

bench_mode convert(const std::string& str) {
  if (str == "netBench")
    return bench_mode::net;
  else if (str == "ioBench")
    return bench_mode::io;
  else if (str == "localBench")
    return bench_mode::local;
  else
    return bench_mode::invalid;
}

caf::expected<std::pair<caf::net::stream_socket, caf::net::stream_socket>>
make_connected_tcp_socket_pair() {
  using namespace std;
  using namespace caf;
  using namespace caf::net;
  tcp_accept_socket acceptor;
  uri::authority_type auth;
  auth.port = 0;
  auth.host = "0.0.0.0"s;
  if (auto res = net::make_tcp_accept_socket(auth, false))
    acceptor = *res;
  else
    return res.error();
  auto guard = make_socket_guard(acceptor);
  uri::authority_type dst;
  dst.host = "localhost"s;
  if (auto port = local_port(socket_cast<network_socket>(guard.socket())))
    dst.port = *port;
  else
    return port.error();
  tcp_stream_socket sock1;
  if (auto res = make_connected_tcp_stream_socket(dst))
    sock1 = *res;
  else
    return res.error();
  if (auto res = accept(guard.socket()))
    return make_pair(sock1, *res);
  else
    return res.error();
}

void exit(const std::string& msg) {
  if (msg != "")
    std::cerr << "ERROR: " << msg << std::endl;
  std::abort();
}

void exit(const caf::error& err) {
  exit(caf::to_string(err));
}
