// SPDX-FileCopyrightText: © 2018 BISDN GmbH
//
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#include <cstdint>
#include <map>

extern "C" {
struct rtnl_link;
}

namespace basebox {

class cnetlink;
class switch_interface;

class nl_vlan {
public:
  nl_vlan(cnetlink *nl);
  ~nl_vlan() {}

  void register_switch_interface(switch_interface *swi) { this->swi = swi; }

  int add_vlan(rtnl_link *link, uint16_t vid, bool tagged);
  int remove_vlan(rtnl_link *link, uint16_t vid, bool tagged);

  int add_ingress_vlan(rtnl_link *link, uint16_t vid, bool pvid);
  int add_ingress_vlan(uint32_t port_id, uint16_t vid, bool pvid,
                       uint16_t vrf_id = 0);

  int enable_vlans(rtnl_link *link);
  int disable_vlans(rtnl_link *link);

  int add_ingress_pvid(rtnl_link *link, uint16_t pvid);
  int remove_ingress_pvid(rtnl_link *link, uint16_t vid);

  int remove_ingress_vlan(rtnl_link *link, uint16_t vid, bool pvid);
  int remove_ingress_vlan(uint32_t port_id, uint16_t vid, bool pvid,
                          uint16_t vrf_id = 0);

  int add_bridge_vlan(rtnl_link *link, uint16_t vid, bool pvid, bool tagged);
  int update_egress_bridge_vlan(rtnl_link *link, uint16_t vid, bool tagged);
  void remove_bridge_vlan(rtnl_link *link, uint16_t vid, bool pvid,
                          bool tagged);

  uint16_t get_vid(rtnl_link *link);

  bool is_vid_valid(uint16_t vid) const {
    if (vid < vid_low || vid > vid_high)
      return false;
    return true;
  }

  void vrf_attach(rtnl_link *old_link, rtnl_link *new_link);
  void vrf_detach(rtnl_link *old_link, rtnl_link *new_link);
  uint16_t get_vrf_id(uint16_t vid, rtnl_link *link);

  int bond_member_attached(rtnl_link *bond, rtnl_link *member);
  int bond_member_detached(rtnl_link *bond, rtnl_link *member);

private:
  static const uint16_t vid_low = 1;
  static const uint16_t vid_high = 0xfff;

  int enable_vlan(uint32_t port_id, uint16_t vid, bool tagged,
                  uint16_t vrf_id = 0);
  int disable_vlan(uint32_t port_id, uint16_t vid, bool tagged,
                   uint16_t vrf_id = 0);

  // ifindex - vlan - refcount
  std::map<std::pair<uint32_t, uint16_t>, uint32_t> port_vlan;
  // vlan - vrf
  std::map<uint16_t, uint16_t> vlan_vrf;

  // ifindex - vlan - ovid, untagged
  std::map<uint32_t, std::map<uint16_t, std::pair<uint16_t, bool>>> bridge_vlan;
  // ifindex - pvid
  std::map<uint32_t, uint16_t> port_pvid;

  switch_interface *swi;
  cnetlink *nl;
};

} // namespace basebox
