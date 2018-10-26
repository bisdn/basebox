/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cinttypes>
#include <deque>

extern "C" {
struct nl_addr;
struct rtnl_addr;
struct rtnl_link;
struct rtnl_neigh;
struct rtnl_route;
}

#define LINK_CAST(obj) reinterpret_cast<struct rtnl_link *>(obj)
#define NEIGH_CAST(obj) reinterpret_cast<struct rtnl_neigh *>(obj)
#define ROUTE_CAST(obj) reinterpret_cast<struct rtnl_route *>(obj)
#define ADDR_CAST(obj) reinterpret_cast<struct rtnl_addr *>(obj)

namespace basebox {

enum link_type {
  LT_UNSUPPORTED,
  LT_BOND,
  LT_BOND_SLAVE,
  LT_BRIDGE,
  LT_BRIDGE_SLAVE,
  LT_TUN,
  LT_VLAN,
  LT_MAX /* must be last */
};

enum link_type get_link_type(rtnl_link *type) noexcept;

} // namespace basebox
