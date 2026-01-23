// SPDX-FileCopyrightText: © 2016 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <cinttypes>
#include <deque>
#include <set>

#include <rofl/common/caddress.h>

#include "utils/utils.h"

namespace basebox {
class switch_interface {
public:
  virtual ~switch_interface() = default;
  enum swi_flags {
    SWIF_ARP = 1 << 0,
  };

  typedef enum {
    SAI_PORT_STAT_RX_PACKETS,
    SAI_PORT_STAT_TX_PACKETS,
    SAI_PORT_STAT_RX_BYTES,
    SAI_PORT_STAT_TX_BYTES,
    SAI_PORT_STAT_RX_DROPPED,
    SAI_PORT_STAT_TX_DROPPED,
    SAI_PORT_STAT_RX_ERRORS,
    SAI_PORT_STAT_TX_ERRORS,
    SAI_PORT_STAT_RX_FRAME_ERR,
    SAI_PORT_STAT_RX_OVER_ERR,
    SAI_PORT_STAT_RX_CRC_ERR,
    SAI_PORT_STAT_COLLISIONS,
  } sai_port_stat_t;

  typedef enum {
    // Drop packets with unknown source MAC. Do not learn. Do not forward.
    SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DROP,
    // Do not learn unknown source MAC. Forward based on destination MAC.
    SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DISABLE,
    // Hardware learning. Learn source MAC. Forward based on destination MAC.
    SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW,
    // Trap packets with unknown source MAC to CPU. Do not learn. Do not
    // forward.
    SAI_BRIDGE_PORT_FDB_LEARNING_MODE_CPU_TRAP,
    // Trap packets with unknown source MAC to CPU. Do not learn. Forward based
    // on destination MAC.
    SAI_BRIDGE_PORT_FDB_LEARNING_MODE_CPU_LOG,
    // Do not learn in hardware. Do not forward. This mode will generate only
    // one notification per unknown source MAC to FDB callback.
    SAI_BRIDGE_PORT_FDB_LEARNING_MODE_FDB_NOTIFICATION,
    // Do not learn in hardware, but forward. This mode will generate only one
    // notification per unknown source MAC to FDB callback.
    // Not an official SAI mode, but there is no equivalent to what we currently
    // do by default in OF-DPA.
    SAI_BRIDGE_PORT_FDB_LEARNING_MODE_FDB_LOG_NOTIFICATION,
  } sai_bridge_port_fdb_learning_t;

  virtual int
  port_set_learn(uint32_t port_id,
                 sai_bridge_port_fdb_learning_t l2_learn) noexcept = 0;
  virtual int
  port_set_move_learn(uint32_t port_id,
                      sai_bridge_port_fdb_learning_t l2_learn) noexcept = 0;

  /* @ LAG { */
  virtual int lag_create(uint32_t *lag_id, std::string name,
                         uint8_t mode) noexcept = 0;
  virtual int lag_remove(uint32_t lag_id) noexcept = 0;
  virtual int lag_add_member(uint32_t lag_id, uint32_t port_id) noexcept = 0;
  virtual int lag_remove_member(uint32_t lag_id, uint32_t port_id) noexcept = 0;
  virtual int lag_set_member_active(uint32_t lag_id, uint32_t port_id,
                                    uint8_t active) noexcept = 0;
  virtual int lag_set_mode(uint32_t lag_id, uint8_t mode) noexcept = 0;
  /* @} */

  /* @ tunnel overlay { */
  virtual int overlay_tunnel_add(uint32_t tunnel_id) noexcept = 0;
  virtual int overlay_tunnel_remove(uint32_t tunnel_id) noexcept = 0;
  /* @} */

  /* @ Layer 2 bridging { */
  virtual int l2_set_idle_timeout(uint16_t idle_timeout) noexcept = 0;

  virtual int l2_addr_remove_all_in_vlan(uint32_t port,
                                         uint16_t vid) noexcept = 0;
  virtual int l2_addr_add(uint32_t port, uint16_t vid,
                          const rofl::caddress_ll &mac, bool filtered,
                          bool permanent, bool update = false) noexcept = 0;
  virtual int l2_addr_remove(uint32_t port, uint16_t vid,
                             const rofl::caddress_ll &mac) noexcept = 0;

  virtual int l2_overlay_addr_add(uint32_t lport, uint32_t tunnel_id,
                                  const rofl::cmacaddr &mac,
                                  bool permanent) noexcept = 0;
  virtual int l2_overlay_addr_remove(uint32_t tunnel_id, uint32_t lport_id,
                                     const rofl::cmacaddr &mac) noexcept = 0;
  /* @} */

  /* @ Layer 2 Multicast { */
  virtual int
  l2_multicast_group_join(uint32_t port, uint16_t vid,
                          const rofl::caddress_ll &mc_group) noexcept = 0;
  virtual int l2_multicast_group_leave(uint32_t port, uint16_t vid,
                                       const rofl::caddress_ll &mc_group,
                                       bool disable_only = false) noexcept = 0;
  virtual int l2_multicast_group_rejoin_all_in_vlan(uint32_t port,
                                                    uint16_t vid) noexcept = 0;
  virtual int l2_multicast_group_leave_all_in_vlan(uint32_t port,
                                                   uint16_t vid) noexcept = 0;
  /* @} */

  /* @ termination MAC { */
  virtual int l3_termination_add(uint32_t sport, uint16_t vid,
                                 const rofl::caddress_ll &dmac) noexcept = 0;
  virtual int l3_termination_add_v6(uint32_t sport, uint16_t vid,
                                    const rofl::caddress_ll &dmac) noexcept = 0;
  virtual int l3_termination_remove(uint32_t sport, uint16_t vid,
                                    const rofl::caddress_ll &dmac) noexcept = 0;
  virtual int
  l3_termination_remove_v6(uint32_t sport, uint16_t vid,
                           const rofl::caddress_ll &dmac) noexcept = 0;
  /* @} */

  /* @ Layer 3 egress group { */
  virtual int l3_egress_create(uint32_t port, uint16_t vid,
                               const rofl::caddress_ll &src_mac,
                               const rofl::caddress_ll &dst_mac,
                               uint32_t *l3_interface) noexcept = 0;
  virtual int l3_egress_update(uint32_t port, uint16_t vid,
                               const rofl::caddress_ll &src_mac,
                               const rofl::caddress_ll &dst_mac,
                               uint32_t *l3_interface_id) noexcept = 0;
  virtual int l3_egress_remove(uint32_t l3_interface) noexcept = 0;
  /* @} */

  /* @ Layer3 Unicast { */
  virtual int l3_unicast_host_add(const rofl::caddress_in4 &ipv4_dst,
                                  uint32_t l3_interface, bool is_ecmp,
                                  bool update_route,
                                  uint16_t vrf_id = 0) noexcept = 0;
  virtual int l3_unicast_host_remove(const rofl::caddress_in4 &ipv4_dst,
                                     uint16_t vrf_id = 0) noexcept = 0;

  virtual int l3_unicast_host_add(const rofl::caddress_in6 &ipv6_dst,
                                  uint32_t l3_interface, bool is_ecmp,
                                  bool update_route,
                                  uint16_t vrf_id = 0) noexcept = 0;
  virtual int l3_unicast_host_remove(const rofl::caddress_in6 &ipv6_dst,
                                     uint16_t vrf_id = 0) noexcept = 0;

  virtual int l3_unicast_route_add(const rofl::caddress_in4 &ipv4_dst,
                                   const rofl::caddress_in4 &mask,
                                   uint32_t l3_interface, bool is_ecmp,
                                   bool update_route,
                                   uint16_t vrf_id = 0) noexcept = 0;

  virtual int l3_unicast_route_remove(const rofl::caddress_in4 &ipv4_dst,
                                      const rofl::caddress_in4 &mask,
                                      uint16_t vrf_id = 0) noexcept = 0;

  virtual int l3_unicast_route_add(const rofl::caddress_in6 &ipv6_dst,
                                   const rofl::caddress_in6 &mask,
                                   uint32_t l3_interface, bool is_ecmp,
                                   bool update_route,
                                   uint16_t vrf_id = 0) noexcept = 0;
  virtual int l3_unicast_route_remove(const rofl::caddress_in6 &ipv6_dst,
                                      const rofl::caddress_in6 &mask,
                                      uint16_t vrf_id = 0) noexcept = 0;
  /* @} */

  /* @ Layer3 ECMP { */
  virtual int l3_ecmp_add(uint32_t *l3_ecmp_id,
                          const std::set<uint32_t> &l3_interfaces) noexcept = 0;
  virtual int
  l3_ecmp_update(uint32_t l3_ecmp_id,
                 const std::set<uint32_t> &l3_interfaces) noexcept = 0;
  virtual int l3_ecmp_remove(uint32_t l3_ecmp_id) noexcept = 0;
  /* @} */

  /* @ Ingress port vlan  { */
  virtual int ingress_port_vlan_accept_all(uint32_t port) noexcept = 0;
  virtual int ingress_port_vlan_drop_accept_all(uint32_t port) noexcept = 0;
  virtual int ingress_port_vlan_add(uint32_t port, uint16_t vid, bool pvid,
                                    uint16_t vrf_id = 0) noexcept = 0;
  virtual int ingress_port_vlan_remove(uint32_t port, uint16_t vid, bool pvid,
                                       uint16_t vrf_id = 0) noexcept = 0;
  virtual int ingress_port_pvid_add(uint32_t port, uint16_t pvid) noexcept = 0;
  virtual int ingress_port_pvid_remove(uint32_t port,
                                       uint16_t pvid) noexcept = 0;
  /* @} */

  /* @ Egress port vlan  { */
  virtual int egress_port_vlan_accept_all(uint32_t port) noexcept = 0;
  virtual int egress_port_vlan_drop_accept_all(uint32_t port) noexcept = 0;

  virtual int egress_port_vlan_add(uint32_t port, uint16_t vid, bool untagged,
                                   bool update = false) noexcept = 0;
  virtual int egress_port_vlan_remove(uint32_t port, uint16_t vid) noexcept = 0;

  virtual int egress_bridge_port_vlan_add(uint32_t port, uint16_t vid,
                                          bool untagged) noexcept = 0;
  virtual int egress_bridge_port_vlan_remove(uint32_t port,
                                             uint16_t vid) noexcept = 0;
  /* @} */

  /* @ Layer 2 overlay  { */
  virtual int add_l2_overlay_flood(uint32_t tunnel_id,
                                   uint32_t lport_id) noexcept = 0;
  virtual int del_l2_overlay_flood(uint32_t tunnel_id,
                                   uint32_t lport_id) noexcept = 0;
  /* @} */

  /* @ QnQ  { */

  /* translate double tag to single tag on ingress */
  virtual int ingress_port_stacked_vlan_enable(uint32_t port,
                                               uint16_t vid) noexcept = 0;
  virtual int ingress_port_stacked_vlan_disable(uint32_t port,
                                                uint16_t vid) noexcept = 0;
  virtual int ingress_port_pop_vlan_add(uint32_t port, uint16_t outer_vid,
                                        uint16_t inner_vid,
                                        uint16_t vrf_id = 0) noexcept = 0;
  virtual int ingress_port_pop_vlan_remove(uint32_t port, uint16_t outer_vid,
                                           uint16_t inner_vid,
                                           uint16_t vrf_id = 0) noexcept = 0;

  /* translate single tag to double tag on egress */
  virtual int egress_port_push_vlan_add(uint32_t port, uint16_t vid,
                                        uint16_t push_vid) noexcept = 0;
  virtual int egress_port_push_vlan_remove(uint32_t port, uint16_t vid,
                                           uint16_t push_vid) noexcept = 0;

  virtual int set_egress_tpid(uint32_t port) noexcept = 0;
  virtual int delete_egress_tpid(uint32_t port) noexcept = 0;
  /* @} */

  /* @ port  { */
  virtual int port_set_config(uint32_t port_id, const rofl::caddress_ll &mac,
                              bool up) noexcept = 0;

  virtual int port_knet_create(uint32_t port_id) noexcept = 0;
  virtual int port_knet_delete(uint32_t port_id) noexcept = 0;
  /* @} */

  /* @ control  { */
  virtual int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept = 0;
  virtual int subscribe_to(enum swi_flags flags) noexcept = 0;

  virtual bool is_connected() noexcept = 0;

  virtual int get_statistics(uint64_t port_no, uint32_t number_of_counters,
                             const sai_port_stat_t *counter_ids,
                             uint64_t *counters) noexcept = 0;
  /* @} */

  /* @ tunnels  { */
  virtual int tunnel_tenant_create(uint32_t tunnel_id,
                                   uint32_t vni) noexcept = 0;
  virtual int tunnel_tenant_delete(uint32_t tunnel_id) noexcept = 0;

  virtual int tunnel_next_hop_create(uint32_t next_hop_id, uint64_t src_mac,
                                     uint64_t dst_mac, uint32_t physical_port,
                                     uint16_t vlan_id) noexcept = 0;
  virtual int tunnel_next_hop_modify(uint32_t next_hop_id, uint64_t src_mac,
                                     uint64_t dst_mac, uint32_t physical_port,
                                     uint16_t vlan_id) noexcept = 0;
  virtual int tunnel_next_hop_delete(uint32_t next_hop_id) noexcept = 0;

  virtual int tunnel_access_port_create(uint32_t port_id,
                                        const std::string &port_name,
                                        uint32_t physical_port,
                                        uint16_t vlan_id,
                                        bool untagged) noexcept = 0;
  virtual int tunnel_enpoint_create(
      uint32_t port_id, const std::string &port_name, uint32_t remote_ipv4,
      uint32_t local_ipv4, uint32_t ttl, uint32_t next_hop_id,
      uint32_t terminator_udp_dst_port, uint32_t initiator_udp_dst_port,
      uint32_t udp_src_port_if_no_entropy, bool use_entropy) noexcept = 0;
  virtual int tunnel_port_delete(uint32_t port_id) noexcept = 0;

  virtual int tunnel_port_tenant_add(uint32_t port_id,
                                     uint32_t tunnel_id) noexcept = 0;
  virtual int tunnel_port_tenant_remove(uint32_t port_id,
                                        uint32_t tunnel_id) noexcept = 0;
  /* @} */

  /* @ STP  { */
  // The stg_create and stg_destroy functions implement creating
  // other Spanning Tree Groups/Instances
  // the stg_vlan_add and stg_vlan_remove function implements adding/removing
  // VLANS from a certain STG
  virtual int ofdpa_stg_create(uint16_t vlan_id) noexcept = 0;
  virtual int ofdpa_stg_destroy(uint16_t vlan_id) noexcept = 0;

  virtual int ofdpa_stg_state_port_set(uint32_t port_id, uint16_t vlan_id,
                                       uint8_t state) noexcept = 0;
  /* @} */
};

class nbi {
public:
  virtual ~nbi() = default;
  enum port_event {
    PORT_EVENT_ADD,
    PORT_EVENT_DEL,
    PORT_EVENT_MODIFY,
    PORT_EVENT_TABLE,
  };

  enum port_status {
    PORT_STATUS_LOWER_DOWN = 0x01,
    PORT_STATUS_ADMIN_DOWN = 0x02,
  };

  struct port_notification_data {
    enum port_event ev;
    uint32_t port_id;
    rofl::caddress_ll hwaddr;
    std::string name;
    bool status;
    uint32_t speed;
    uint8_t duplex;
  };

  enum port_type {
    port_type_physical = 0,
    port_type_vxlan = 1,
    port_type_lag = 2,
  };

  enum switch_state {
    SWITCH_STATE_UNKNOWN,
    SWITCH_STATE_UP,
    SWITCH_STATE_DOWN,
    SWITCH_STATE_FAILED,
  };

  static uint32_t combine_port_type(uint16_t port_num, enum port_type type) {
    return (uint32_t)port_num | type << 16;
  }

  static enum port_type get_port_type(uint32_t port_id) {
    return static_cast<enum port_type>(port_id >> 16);
  }

  static uint16_t get_port_num(uint32_t port_id) {
    return port_id & 0xffffffff;
  }

  virtual void register_switch(switch_interface *) noexcept = 0;
  virtual void switch_state_notification(enum switch_state) noexcept = 0;
  virtual void resend_state() noexcept = 0;
  virtual void
  port_notification(std::deque<port_notification_data> &) noexcept = 0;
  virtual int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept = 0;
  virtual int fdb_timeout(uint32_t port_id, uint16_t vid,
                          const rofl::caddress_ll &mac) noexcept = 0;
};

inline switch_interface::swi_flags operator|(switch_interface::swi_flags a,
                                             switch_interface::swi_flags b) {
  return static_cast<switch_interface::swi_flags>(static_cast<int>(a) |
                                                  static_cast<int>(b));
}

} // namespace basebox
