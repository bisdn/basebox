/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <rofl/common/caddress.h>

namespace rofl {

inline caddress_in4 build_mask_in4(unsigned prefix_len) {
  caddress_in4 mask;
  uint32_t m = ~((UINT32_C(1) << (32 - prefix_len)) - 1);
  mask.set_addr_hbo(m);
  return mask;
}

} // namespace rofl
