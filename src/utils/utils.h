// SPDX-FileCopyrightText: © 2018 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

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
