/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_
#define SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_

#include <cstdint>
#include <list>
#include <string>
#include <memory>

struct rtnl_link_bridge_vlan;
struct rtnl_link;
struct rtnl_neigh;
struct nl_addr;

namespace basebox {

class switch_interface;
class tap_manager;

class ofdpa_bridge {
public:
  ofdpa_bridge(switch_interface *sw, std::shared_ptr<tap_manager> tap_man);

  virtual ~ofdpa_bridge();

  void set_bridge_interface(rtnl_link *);

  bool has_bridge_interface() const { return bridge != nullptr; }

  void add_interface(rtnl_link *);

  void update_interface(rtnl_link *old_link, rtnl_link *new_link);

  void delete_interface(rtnl_link *);

  void add_neigh_to_fdb(rtnl_neigh *);
  void add_mac_to_fdb(rtnl_neigh *, rtnl_link *);

  void remove_mac_from_fdb(rtnl_neigh *);

private:
  void update_vlans(uint32_t port, const std::string &devname,
                    const rtnl_link_bridge_vlan *old_br_vlan,
                    const rtnl_link_bridge_vlan *new_br_vlan);

  rtnl_link *bridge;
  switch_interface *sw;
  bool ingress_vlan_filtered;
  bool egress_vlan_filtered;

  std::shared_ptr<tap_manager> tap_man;
};

} /* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_ */
