/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>
#include <memory>

struct rtnl_addr;
struct rtnl_neigh;
struct nl_addr;

namespace basebox {

class cnetlink;
class nl_vlan;
class switch_interface;
class tap_manager;

class nl_l3 {
public:
  nl_l3(std::shared_ptr<nl_vlan> vlan, cnetlink *nl);
  ~nl_l3() {}

  int add_l3_termination(struct rtnl_addr *a);
  int del_l3_termination(struct rtnl_addr *a);

  int add_l3_neigh(struct rtnl_neigh *n);
  int update_l3_neigh(struct rtnl_neigh *n_old, struct rtnl_neigh *n_new);
  int del_l3_neigh(struct rtnl_neigh *n);

  int del_l3_egress(int ifindex, const struct nl_addr *s_mac,
                    const struct nl_addr *d_mac);

  void register_switch_interface(switch_interface *sw);
  void set_tapmanager(std::shared_ptr<tap_manager> tm) { tap_man = tm; }

  struct nh_lookup_params {
    std::deque<struct rtnl_neigh *> *neighs;
    rtnl_route *rt;
    cnetlink *nl;
  };

  void get_neighbours_of_route(rtnl_route *r, nh_lookup_params *p);

private:
  switch_interface *sw;
  std::shared_ptr<tap_manager> tap_man;
  std::shared_ptr<nl_vlan> vlan;
  cnetlink *nl;
};

} // namespace basebox
