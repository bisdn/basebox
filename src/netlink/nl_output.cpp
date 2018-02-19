#include <cassert>
#include <vector>

#include <glog/logging.h>
#include <netlink/addr.h>
#include <netlink/route/route.h>

#include "nl_output.hpp"

namespace basebox {

static const int buf_size = 2048;
static std::vector<char> buf(buf_size, 0);
static struct nl_dump_params p = {
    NL_DUMP_LINE,   0, 0, 0, nullptr, nullptr, nullptr, nullptr, buf.data(),
    buf.capacity(), 0, 0, 0};

std::ostream &operator<<(std::ostream &stream, const struct nl_addr *addr) {
  assert(addr);

  if (VLOG_IS_ON(3)) {
    stream << "(obj=" << static_cast<const void *>(addr) << ") ";
    p.dp_type = NL_DUMP_DETAILS;
  }

  nl_addr2str(addr, buf.data(), buf.capacity());
  stream << buf.data();
  return stream;
}

std::ostream &operator<<(std::ostream &stream, struct nl_object *n) {
  assert(n);

  if (VLOG_IS_ON(3)) {
    stream << "(obj=" << static_cast<const void *>(n) << ") ";
    p.dp_type = NL_DUMP_DETAILS;
  }

  nl_object_dump(n, &p);
  stream << buf.data();
  return stream;
}

std::ostream &operator<<(std::ostream &stream, struct rtnl_nexthop *nh) {
  assert(nh);

  if (VLOG_IS_ON(3)) {
    stream << "(obj=" << static_cast<const void *>(nh) << ") ";
    p.dp_type = NL_DUMP_DETAILS;
  }

  rtnl_route_nh_dump(nh, &p);
  stream << buf.data();
  return stream;
}

} // namespace basebox
