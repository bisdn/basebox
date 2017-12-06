/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>

struct rtnl_addr;
struct rtnl_neigh;
struct nl_addr;

namespace basebox {

class switch_interface;

class nl_l3 {
public:
  nl_l3(switch_interface *sw);
  ~nl_l3() {}

  int add_l3_termination(struct rtnl_addr *a);
  int del_l3_termination(struct rtnl_addr *a);

  int add_l3_neigh(struct rtnl_neigh *n);
  int update_l3_neigh(struct rtnl_neigh *n_old, struct rtnl_neigh *n_new);
  int del_l3_neigh(struct rtnl_neigh *n);

  int del_l3_egress(int ifindex, const struct nl_addr *s_mac,
                    const struct nl_addr *d_mac);

  void register_switch_interface(switch_interface *sw);

private:
  switch_interface *sw;
};

} // namespace basebox
