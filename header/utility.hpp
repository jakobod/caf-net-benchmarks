/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2020 Jakob Otto                                             *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#pragma once

#include <chrono>

#include "caf/expected.hpp"
#include "caf/fwd.hpp"
#include "caf/net/fwd.hpp"

// -- type defines -------------------------------------------------------------

using socket_pair = std::pair<caf::net::stream_socket, caf::net::stream_socket>;

using timestamp_vec = std::vector<std::chrono::microseconds>;

enum class bench_mode { net, io, local, invalid };

bench_mode convert(const std::string& str);

caf::expected<std::pair<caf::net::stream_socket, caf::net::stream_socket>>
make_connected_tcp_socket_pair();

void print_vec(int num, timestamp_vec& v, size_t offset = 0);

void print_len(timestamp_vec& v);

void init_file(size_t len);

timestamp_vec strip_vec(timestamp_vec& vec, size_t begin_offset,
                        size_t end_offset);