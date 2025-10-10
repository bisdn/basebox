// SPDX-FileCopyrightText: © 2018 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <iostream>

namespace basebox {

std::ostream &operator<<(std::ostream &stream, const struct nl_addr *addr);

std::ostream &operator<<(std::ostream &stream, struct nl_object *n);

std::ostream &operator<<(std::ostream &stream, struct rtnl_nexthop *nh);

inline std::ostream &operator<<(std::ostream &stream, struct rtnl_addr *addr) {
  return operator<<(stream, OBJ_CAST(addr));
}

inline std::ostream &operator<<(std::ostream &stream, struct rtnl_link *link) {
  return operator<<(stream, OBJ_CAST(link));
}

inline std::ostream &operator<<(std::ostream &stream,
                                struct rtnl_neigh *neigh) {
  return operator<<(stream, OBJ_CAST(neigh));
}

inline std::ostream &operator<<(std::ostream &stream,
                                struct rtnl_route *route) {
  return operator<<(stream, OBJ_CAST(route));
}

inline std::ostream &operator<<(std::ostream &stream, struct rtnl_nh *route) {
  return operator<<(stream, OBJ_CAST(route));
}

inline std::string_view safe_string_view(const char *str) {
  return std::string_view(str ? str : "(null)");
}

} // namespace basebox
