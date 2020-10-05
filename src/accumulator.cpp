#include "accumulator.hpp"

#include "caf/behavior.hpp"
#include "caf/event_based_actor.hpp"
#include "caf/stateful_actor.hpp"
#include "type_ids.hpp"
#include "utility.hpp"

caf::behavior accumulator_actor(caf::stateful_actor<accumulator_state>* self,
                                size_t num_nodes) {
  using std::chrono::microseconds;
  self->state.begins.reserve(num_nodes);
  self->state.ends.reserve(num_nodes);
  return {
    [=](init_atom) { self->state.begins.emplace_back(now<microseconds>()); },
    [=](done_atom) {
      auto mean = [=](const auto& x) {
        auto tmp = std::accumulate(std::next(x.begin()), x.end(), x[0],
                                   [](const microseconds v1,
                                      const microseconds v2) -> microseconds {
                                     return v1 + v2;
                                   });
        return tmp / x.size();
      };
      self->state.ends.emplace_back(now<microseconds>());
      if (self->state.ends.size() == self->state.ends.capacity()) {
        auto& begins = self->state.begins;
        auto& ends = self->state.ends;
        auto begin = mean(begins);
        auto end = mean(ends);
        auto duration = end - begin;
        std::cout << duration.count() << ", ";
        self->quit();
      }
    },
  };
}