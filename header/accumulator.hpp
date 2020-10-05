// accumulator.hpp
// @author Jakob Otto

#include <chrono>
#include <numeric>
#include <vector>

#include "caf/fwd.hpp"

struct accumulator_state {
  std::vector<std::chrono::microseconds> begins;
  std::vector<std::chrono::microseconds> ends;
};

caf::behavior accumulator_actor(caf::stateful_actor<accumulator_state>* self,
                                size_t num_nodes);
