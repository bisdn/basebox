/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_
#define SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_

#include <cstdint>
#include <list>

#include "roflibs/netlink/crtlink.hpp"

namespace rofcore {

class switch_interface;

class ofdpa_bridge {
public:
  ofdpa_bridge(switch_interface *sw);

  virtual ~ofdpa_bridge();

  void set_bridge_interface(const rofcore::crtlink &rtl);

  const rofcore::crtlink &get_bridge_interface() const { return bridge; }

  bool has_bridge_interface() const { return 0 != bridge.get_ifindex(); }

  void add_interface(uint32_t port, const rofcore::crtlink &rtl);

  void update_interface(uint32_t port, const rofcore::crtlink &oldlink,
                        const rofcore::crtlink &newlink);

  void delete_interface(uint32_t port, const rofcore::crtlink &rtl);

  void add_mac_to_fdb(const uint32_t port, const uint16_t vid,
                      const rofl::cmacaddr &mac);

  void remove_mac_from_fdb(const uint32_t port, uint16_t vid,
                           const rofl::cmacaddr &mac);

private:
  void update_vlans(uint32_t port, const std::string &devname,
                    const rtnl_link_bridge_vlan *old_br_vlan,
                    const rtnl_link_bridge_vlan *new_br_vlan);

  switch_interface *sw;
  bool ingress_vlan_filtered;
  bool egress_vlan_filtered;
  rofcore::crtlink bridge;
  std::map<uint16_t, std::list<uint32_t>> l2_domain;
};

} /* namespace basebox */

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_BRIDGE_HPP_ */
