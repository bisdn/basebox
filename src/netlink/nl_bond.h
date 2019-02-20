/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>
#include <map>
#include <set>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class cnetlink;
class switch_interface;

class nl_bond {
public:
  nl_bond(cnetlink *nl);
  ~nl_bond() {}

  void register_switch_interface(switch_interface *swi) { this->swi = swi; }

  /**
   * returns the id of the lag
   *
   * @returns id > 0 if found, 0 if not found
   */
  uint32_t get_lag_id(rtnl_link *bond);

  /**
   * create a new lag
   *
   * @returns <0 on error, lag_id on success
   */
  int add_lag(rtnl_link *bond);

  int remove_lag(rtnl_link *bond);
  int add_lag_member(rtnl_link *bond, rtnl_link *link);
  int remove_lag_member(rtnl_link *bond, rtnl_link *link);

private:
  switch_interface *swi;
  cnetlink *nl;

  std::map<int, uint32_t> ifi2lag;
  std::map<int, uint32_t> lag_members;
};

} // namespace basebox
