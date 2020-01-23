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

#include <iostream>
#include <string>
#include <vector>

#include "caf/byte.hpp"
#include "caf/error.hpp"
#include "caf/net/fwd.hpp"

using socket_pair = std::pair<caf::net::stream_socket, caf::net::stream_socket>;

caf::expected<std::pair<caf::net::stream_socket, caf::net::stream_socket>>
make_connected_tcp_socket_pair();

template <class T>
void print_vector(const std::string& name, const std::vector<T>& vec) {
  using namespace std;
  cout << name << ": ";
  for (const auto& v : vec) {
    auto val = v.count();
    cout << val << ", ";
  }
  cout << endl;
}

template <class T>
void erase(std::vector<T>& vec, size_t begin, size_t end) {
  vec.erase(vec.begin() + begin, vec.begin() + end);
}
