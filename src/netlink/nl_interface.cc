// SPDX-FileCopyrightText: © 2019 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#include <netlink/route/link.h>

#include "nl_interface.h"
#include "port_manager.h"

namespace basebox {

nl_interface::nl_interface(cnetlink *nl) : nl(nl) {}

int nl_interface::changed(rtnl_link *old_link, rtnl_link *new_link) noexcept {

  if (rtnl_link_get_mtu(old_link) != rtnl_link_get_mtu(new_link)) {
    pm->update_mtu(new_link);
  }

  return 0;
}

} // namespace basebox
