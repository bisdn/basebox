/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <iostream>

namespace basebox {

std::ostream &operator<<(std::ostream &stream, const struct nl_addr *addr);

std::ostream &operator<<(std::ostream &stream, struct nl_object *n);

std::ostream &operator<<(std::ostream &stream, struct rtnl_nexthop *nh);

inline std::string_view safe_string_view(const char *str) {
  return std::string_view(str ? str : "(null)");
}

} // namespace basebox
