/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstddef>
#include <cstdint>
#include <linux/if_ether.h>

namespace basebox {

struct packet {
  std::size_t len; ///< actual lenght written into data
  char data[0];    ///< total allocated buffer
};

struct vlan_hdr {
  struct ethhdr eth; // ethertype/tpid
  uint16_t vlan;     // vid + cfi + pcp
  uint16_t h_proto;  // ethertype or data
} __attribute__((packed));

} // namespace basebox
