/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <vector>

#include <glog/logging.h>
#include <netlink/addr.h>
#include <netlink/route/route.h>
#include <netlink/route/nexthop.h>

#include "nl_output.hpp"

namespace basebox {

static const int buf_size = 2048;
static std::vector<char> buf(buf_size, 0);
static struct nl_dump_params p = {
    NL_DUMP_LINE,   0, 0, 0, nullptr, nullptr, nullptr, nullptr, buf.data(),
    buf.capacity(), 0, 0, 0};

std::ostream &operator<<(std::ostream &stream, const struct nl_addr *addr) {
  if (addr == nullptr) {
    stream << "<nullptr>";
    return stream;
  }

  if (VLOG_IS_ON(3)) {
    stream << "(addr_obj=" << static_cast<const void *>(addr) << ") ";
    p.dp_type = NL_DUMP_DETAILS;
  }

  nl_addr2str(addr, buf.data(), buf.capacity());
  stream << buf.data();
  return stream;
}

std::ostream &operator<<(std::ostream &stream, struct nl_object *n) {
  if (n == nullptr) {
    stream << "<nullptr>";
    return stream;
  }

  if (VLOG_IS_ON(3)) {
    stream << "(nl_obj=" << static_cast<const void *>(n) << ") ";
    p.dp_type = NL_DUMP_DETAILS;
  }

  nl_object_dump(n, &p);
  stream << buf.data();
  return stream;
}

std::ostream &operator<<(std::ostream &stream, struct rtnl_nexthop *nh) {
  if (nh == nullptr) {
    stream << "<nullptr>";
    return stream;
  }

  if (VLOG_IS_ON(3)) {
    stream << "(nh_obj=" << static_cast<const void *>(nh) << ") ";
    p.dp_type = NL_DUMP_DETAILS;
    p.dp_ivar = NH_DUMP_FROM_DETAILS;
  }

  rtnl_route_nh_dump(nh, &p);
  stream << buf.data();
  return stream;
}

} // namespace basebox
