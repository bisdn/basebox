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

#include "netlink-utils.hpp"
#include "nl_output.hpp"

#define lt_names                                                               \
  "unsupported", "team", "bond", "bond_slave", "bridge", "bridge_slave",       \
      "tun", "vlan"

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

  } else {
    type = rtnl_link_get_type(link);
  }

  VLOG(2) << __FUNCTION__ << ": type=" << std::string_view(type)
          << ", af=" << rtnl_link_get_family(link) << ", slave=" << slave
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

} // namespace basebox
