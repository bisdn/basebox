/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_
#define SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_

#include <cstdint>
#include <list>
#include <string>

struct rtnl_link_bridge_vlan;
struct rtnl_link;
struct rtnl_neigh;
struct nl_addr;

namespace basebox {

class switch_interface;

class ofdpa_bridge {
public:
  ofdpa_bridge(switch_interface *sw);

  virtual ~ofdpa_bridge();

  void set_bridge_interface(rtnl_link *);

  bool has_bridge_interface() const { return bridge != nullptr; }

  void add_interface(uint32_t port, rtnl_link *);

  void update_interface(uint32_t port, rtnl_link *old_link,
                        rtnl_link *new_link);

  void delete_interface(uint32_t port, rtnl_link *);

  void add_mac_to_fdb(const uint32_t port, uint16_t vlan, nl_addr *mac);
  void add_mac_to_fdb(const uint32_t port, rtnl_neigh *, rtnl_link *);

  void remove_mac_from_fdb(const uint32_t port, rtnl_neigh *);

private:
  void update_vlans(uint32_t port, const std::string &devname,
                    const rtnl_link_bridge_vlan *old_br_vlan,
                    const rtnl_link_bridge_vlan *new_br_vlan);

  rtnl_link *bridge;
  switch_interface *sw;
  bool ingress_vlan_filtered;
  bool egress_vlan_filtered;
};

} /* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_ */
