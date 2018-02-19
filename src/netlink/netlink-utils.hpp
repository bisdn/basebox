#pragma once

#include <cassert>
#include <cinttypes>

#include <netlink/addr.h>

#define LINK_CAST(obj) reinterpret_cast<struct rtnl_link *>(obj)
#define NEIGH_CAST(obj) reinterpret_cast<struct rtnl_neigh *>(obj)
#define ROUTE_CAST(obj) reinterpret_cast<struct rtnl_route *>(obj)
#define ADDR_CAST(obj) reinterpret_cast<struct rtnl_addr *>(obj)

uint64_t nlall2uint64(const nl_addr *a);
