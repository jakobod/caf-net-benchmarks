/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2019 Jakob Otto                                             *
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

#include "caf/byte.hpp"
#include "caf/error.hpp"

class dummy_application {
public:
  static caf::error serialize(caf::actor_system& sys,
                              const caf::type_erased_tuple& x,
                              std::vector<caf::byte>& buf) {
    caf::binary_serializer sink{sys, buf};
    return caf::message::save(sink, x);
  }

  template <class Parent>
  caf::error init(Parent&) {
    return caf::none;
  }

  template <class Parent>
  void write_message(
    Parent& parent,
    std::unique_ptr<caf::net::endpoint_manager_queue::message> msg) {
    parent.write_packet(msg->payload);
  }

  template <class Parent>
  caf::error handle_data(Parent&, caf::span<const caf::byte>) {
    return caf::none;
  }

  template <class Parent>
  void resolve(Parent&, caf::string_view path, const caf::actor& listener) {
    anon_send(listener, caf::resolve_atom::value,
              "the resolved path is still "
                + std::string(path.begin(), path.end()));
  }

  template <class Parent>
  void timeout(Parent&, caf::atom_value, uint64_t) {
    // nop
  }

  template <class Parent>
  void new_proxy(Parent&, caf::actor_id) {
    // nop
  }

  template <class Parent>
  void local_actor_down(Parent&, caf::actor_id, caf::error) {
    // nop
  }

  void handle_error(caf::sec) {
    // nop
  }
};
