#pragma once

#include <glog/logging.h>
#include <rofl/common/caddress.h>

namespace rofl {

inline caddress_in4 build_mask_in4(unsigned prefix_len) {
  caddress_in4 mask;
  uint32_t m = ~((UINT32_C(1) << (32 - prefix_len)) - 1);
  LOG(INFO) << std::showbase << std::hex << m;
  mask.set_addr_hbo(m);
  return mask;
}

} // namespace rofl
