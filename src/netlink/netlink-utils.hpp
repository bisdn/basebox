#pragma once

#include <cinttypes>

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
  LT_UNKNOWN = 0,
  LT_UNSUPPORTED,
  LT_BRIDGE,
  LT_TUN,
  LT_MAX /* must be last */
};

enum link_type kind_to_link_type(const char *type) noexcept;

} // namespace basebox
