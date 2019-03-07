/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <memory>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class cnetlink;
class tap_manager;

class nl_interface {
public:
  nl_interface(cnetlink *nl);

  void set_tapmanager(std::shared_ptr<tap_manager> tm) { this->tm = tm; }
  int changed(rtnl_link *old_link, rtnl_link *new_link) noexcept;

private:
  std::shared_ptr<tap_manager> tm;
  cnetlink *nl;
};

} // namespace basebox
