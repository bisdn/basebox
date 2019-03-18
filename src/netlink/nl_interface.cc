/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <netlink/route/link.h>

#include "nl_interface.h"
#include "tap_manager.h"

namespace basebox {

nl_interface::nl_interface(cnetlink *nl) : nl(nl) {}

int nl_interface::changed(rtnl_link *old_link, rtnl_link *new_link) noexcept {

  if (rtnl_link_get_mtu(old_link) != rtnl_link_get_mtu(new_link)) {
    tm->update_mtu(new_link);
  }

  return 0;
}

} // namespace basebox
