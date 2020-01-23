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
#include "dummy_application.hpp"

class dummy_application_factory {
public:
  using application_type = dummy_application;

  static caf::error serialize(caf::actor_system& sys,
                              const caf::type_erased_tuple& x,
                              std::vector<caf::byte>& buf) {
    return dummy_application::serialize(sys, x, buf);
  }

  template <class Parent>
  caf::error init(Parent&) {
    return caf::none;
  }

  application_type make() const {
    return dummy_application{};
  }
};
