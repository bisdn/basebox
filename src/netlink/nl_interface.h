// SPDX-FileCopyrightText: © 2019 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <memory>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class cnetlink;
class port_manager;

class nl_interface {
public:
  nl_interface(cnetlink *nl);

  void set_tapmanager(std::shared_ptr<port_manager> pm) { this->pm = pm; }
  int changed(rtnl_link *old_link, rtnl_link *new_link) noexcept;

private:
  std::shared_ptr<port_manager> pm;
  cnetlink *nl;
};

} // namespace basebox
