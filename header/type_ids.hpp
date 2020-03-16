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

#include <cstdint>
#include <vector>

#include "caf/fwd.hpp"
#include "caf/type_id.hpp"

CAF_BEGIN_TYPE_ID_BLOCK(caf_net_benchmark, caf::first_custom_type_id)

CAF_ADD_TYPE_ID(caf_net_benchmark, (std::vector<uint64_t>) )
CAF_ADD_TYPE_ID(caf_net_benchmark, (caf::stream<uint64_t>) )
CAF_ADD_TYPE_ID(caf_net_benchmark, (caf::stream<std::vector<uint64_t>>) )
CAF_ADD_TYPE_ID(caf_net_benchmark, (std::vector<std::vector<uint64_t>>) )

CAF_ADD_ATOM(caf_net_benchmark, start_atom)
CAF_ADD_ATOM(caf_net_benchmark, stop_atom)
CAF_ADD_ATOM(caf_net_benchmark, done_atom)
CAF_ADD_ATOM(caf_net_benchmark, io_bench_atom)
CAF_ADD_ATOM(caf_net_benchmark, net_bench_atom)
CAF_ADD_ATOM(caf_net_benchmark, local_ench_atom)

CAF_END_TYPE_ID_BLOCK(caf_net_benchmark)
