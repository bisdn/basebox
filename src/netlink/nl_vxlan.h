/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "nl_l3_interfaces.h"

extern "C" {
struct nl_addr;
struct rtnl_link;
struct rtnl_neigh;
}

namespace basebox {

class cnetlink;
class nl_l3;
class nl_bridge;
class switch_interface;
struct tunnel_nh;

class nl_vxlan : public net_reachable, nh_reachable {
public:
  nl_vxlan(std::shared_ptr<nl_l3> l3, cnetlink *nl);
  ~nl_vxlan() {}

  void net_reachable_notification(struct net_params) noexcept override;
  void nh_reachable_notification(struct rtnl_neigh *,
                                 struct nh_params) noexcept override;

  int create_vni(rtnl_link *link);
  int remove_vni(rtnl_link *link);

  void register_switch_interface(switch_interface *sw);
  void register_bridge(nl_bridge *bridge);

  int add_l2_neigh(rtnl_neigh *neigh, rtnl_link *vxlan_link,
                   rtnl_link *br_link);
  int delete_l2_neigh(rtnl_neigh *neigh, rtnl_link *vxlan_link,
                      rtnl_link *br_link);

  int get_tunnel_id(rtnl_link *vxlan_link, uint32_t *vni,
                    uint32_t *tunnel_id) noexcept;
  int get_tunnel_id(uint32_t vni, uint32_t *tunnel_id) noexcept;

  int create_access_port(rtnl_link *br_link, uint32_t tunnel_id,
                         const std::string &access_port_name, uint32_t pport_no,
                         uint16_t vid, bool untagged, uint32_t *lport);
  int delete_access_port(rtnl_link *br_link, uint32_t pport_no, uint16_t vid,
                         bool wipe_l2_addresses);

  int create_endpoint(rtnl_link *vxlan_link);
  int delete_endpoint(rtnl_link *vxlan_link);

private:
  int create_endpoint(rtnl_link *vxlan_link, rtnl_link *br_link,
                      nl_addr *group);
  int create_endpoint(rtnl_link *vxlan_link, nl_addr *local_, nl_addr *group_,
                      uint32_t _next_hop_id, uint32_t *_port_id);
  int delete_endpoint(rtnl_link *vxlan_link, nl_addr *local_, nl_addr *group_);

  int create_next_hop(rtnl_link *vxlan_link, nl_addr *remote,
                      uint32_t *next_hop_id);
  int create_next_hop(rtnl_neigh *neigh, uint32_t *_next_hop_id);
  int delete_next_hop(rtnl_neigh *neigh);
  int delete_next_hop(uint32_t nh_id);
  int delete_next_hop(const struct tunnel_nh &);

  int enable_flooding(uint32_t tunnel_id, uint32_t lport_id);
  int disable_flooding(uint32_t tunnel_id, uint32_t lport_id);

  int add_l2_neigh(rtnl_neigh *neigh, uint32_t lport, uint32_t tunnel_id);

  int delete_l2_neigh_access(rtnl_link *link, uint16_t vlan,
                             nl_addr *neigh_mac);
  int delete_l2_neigh_endpoint(rtnl_link *vxlan_link, nl_addr *local,
                               nl_addr *remote, nl_addr *neigh_mac);
  int delete_l2_neigh(uint32_t tunnel_id, nl_addr *neigh_mac);

  // XXX TODO handle these better and prevent id overflow
  uint32_t next_hop_id_cnt = 1;
  uint32_t port_id_cnt = 1 << 16 | 1;
  uint32_t tunnel_id_cnt = 10;

  std::map<uint32_t, int> vni2tunnel;

  switch_interface *sw;
  nl_bridge *bridge;
  std::shared_ptr<nl_l3> l3;
  cnetlink *nl;
};

} // namespace basebox
