#include "switch_behavior.hpp"
#include "roflibs/of-dpa/switch_behavior_ofdpa.hpp"

namespace basebox {

switch_behavior::switch_behavior(const rofl::cdptid &dptid) : dptid(dptid) {}

switch_behavior::~switch_behavior() {}

switch_behavior *switch_behavior_fabric::get_behavior(const int type,
                                                      rofl::crofdpt &dpt) {
  switch (type) {
  case 1:
    return new switch_behavior_ofdpa(dpt);
    break;
  default:
    return new switch_behavior(dpt.get_dptid());
    break;
  }
}

} /* namespace basebox */
