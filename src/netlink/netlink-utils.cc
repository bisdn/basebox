/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <map>
#include <memory>
#include <string_view>
#include <vector>
#include <cstring>

#include <glog/logging.h>
#include <netlink/route/link.h>

#include "netlink-utils.h"
#include "nl_output.h"

#define lt_names                                                               \
  "unsupported", "team", "bond", "bond_slave", "bridge", "bridge_slave",       \
      "tun", "vlan", "vxlan"

namespace basebox {

std::map<std::string, enum link_type> kind2lt;
std::vector<std::string> lt2names = {lt_names};

static void init_kind2lt_map() {
  enum link_type lt = LT_UNSUPPORTED;
  for (const auto &n : lt2names) {
    assert(lt != LT_MAX);
    kind2lt.emplace(n, lt);
    lt = static_cast<enum link_type>(lt + 1);
  }
}

enum link_type get_link_type(rtnl_link *link) noexcept {
  const char *type;
  bool slave = false;

  assert(link);
  assert(lt2names.size() == LT_MAX);

  if (kind2lt.size() == 0) {
    init_kind2lt_map();
  }

  if (rtnl_link_get_master(link)) {
    // slave device
    type = rtnl_link_get_slave_type(link);
    slave = true;

    if (type == nullptr && rtnl_link_get_family(link) == AF_BRIDGE) {
      // libnl bug or kernel adds an additional link?
      return LT_BRIDGE_SLAVE;
    }

    // the bridge device (not the bridge_slave)
    if (type && 0 == lt2names[LT_BRIDGE].compare(type) &&
        rtnl_link_get_type(link)) {
      type = rtnl_link_get_type(link);
      slave = false;
    }

  } else {
    type = rtnl_link_get_type(link);
  }

  VLOG(2) << __FUNCTION__ << ": type=" << std::string_view(type)
          << ", slave=" << slave << ", af=" << rtnl_link_get_family(link)
          << " of link " << OBJ_CAST(link);

  // lo has no type
  if (!type) {
    return LT_UNSUPPORTED;
  }

  auto it = kind2lt.find(std::string(type));

  if (it != kind2lt.end()) {
    enum link_type lt = it->second;

    // treat team as bond
    if (lt == LT_TEAM)
      lt = LT_BOND;

    if (slave)
      lt = static_cast<enum link_type>(lt + 1);
    return lt;
  }

  VLOG(1) << __FUNCTION__ << ": type=" << std::string_view(type)
          << " not supported";
  return LT_UNSUPPORTED;
}

uint64_t nlall2uint64(const nl_addr *a) noexcept {
  uint64_t b = 0;
  char *b_, *a_;

  assert(nl_addr_get_len(a) == 6);

  a_ = static_cast<char *>(nl_addr_get_binary_addr(a));
  b_ = reinterpret_cast<char *>(&b);
  b_[5] = a_[0];
  b_[4] = a_[1];
  b_[3] = a_[2];
  b_[2] = a_[3];
  b_[1] = a_[4];
  b_[0] = a_[5];
  return b;
}

} // namespace basebox
