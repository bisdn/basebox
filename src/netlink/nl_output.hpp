#pragma once

#include <iostream>

namespace basebox {

std::ostream &operator<<(std::ostream &stream, const struct nl_addr *addr);

std::ostream &operator<<(std::ostream &stream, struct nl_object *n);

std::ostream &operator<<(std::ostream &stream, struct rtnl_nexthop *nh);

} // namespace basebox
