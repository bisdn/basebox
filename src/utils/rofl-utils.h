/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <arpa/inet.h>
#include <rofl/common/caddress.h>

namespace rofl {

inline caddress_in4 build_mask_in4(unsigned prefix_len) {
  caddress_in4 mask;
  uint32_t m = ~((UINT32_C(1) << (32 - prefix_len)) - 1);
  mask.set_addr_hbo(m);
  return mask;
}

inline caddress_in6 build_mask_in6(unsigned prefix_len) {
  struct sockaddr_in6 sa;
  int div = prefix_len / 8;

  sa.sin6_family = AF_INET6;
  memset(sa.sin6_addr.s6_addr, 0, sizeof(sa.sin6_addr.s6_addr));
  memset(sa.sin6_addr.s6_addr, 0xff, div);
  sa.sin6_addr.s6_addr[div] = (0xff << (8 - prefix_len % 8)) & 0xff;

  return rofl::caddress_in6(&sa, sizeof(sa));
}
} // namespace rofl
