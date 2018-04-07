/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cinttypes>
#include <deque>

#include <rofl/common/caddress.h>

#include "utils/utils.hpp"

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

  virtual int lag_create(uint32_t *lag_id) noexcept = 0;
  virtual int lag_remove(uint32_t lag_id) noexcept = 0;
  virtual int lag_add_member(uint32_t lag_id, uint32_t port_id) noexcept = 0;
  virtual int lag_remove_member(uint32_t lag_id, uint32_t port_id) noexcept = 0;

  virtual int overlay_tunnel_add(uint32_t tunnel_id) noexcept = 0;
  virtual int overlay_tunnel_remove(uint32_t tunnel_id) noexcept = 0;

  virtual int l2_addr_remove_all_in_vlan(uint32_t port,
                                         uint16_t vid) noexcept = 0;
  virtual int l2_addr_add(uint32_t port, uint16_t vid,
                          const rofl::caddress_ll &mac, bool filtered,
                          bool permanent) noexcept = 0;
  virtual int l2_addr_remove(uint32_t port, uint16_t vid,
                             const rofl::caddress_ll &mac) noexcept = 0;

  virtual int l2_overlay_addr_add(uint32_t lport, uint32_t tunnel_id,
                                  const rofl::cmacaddr &mac,
                                  bool permanent) noexcept = 0;
  virtual int l2_overlay_addr_remove(uint32_t tunnel_id, uint32_t lport_id,
                                     const rofl::cmacaddr &mac) noexcept = 0;

  virtual int l3_termination_add(uint32_t sport, uint16_t vid,
                                 const rofl::caddress_ll &dmac) noexcept = 0;
  virtual int l3_termination_add_v6(uint32_t sport, uint16_t vid,
                                    const rofl::caddress_ll &dmac) noexcept = 0;
  virtual int l3_termination_remove(uint32_t sport, uint16_t vid,
                                    const rofl::caddress_ll &dmac) noexcept = 0;
  virtual int
  l3_termination_remove_v6(uint32_t sport, uint16_t vid,
                           const rofl::caddress_ll &dmac) noexcept = 0;

  virtual int l3_egress_create(uint32_t port, uint16_t vid,
                               const rofl::caddress_ll &src_mac,
                               const rofl::caddress_ll &dst_mac,
                               uint32_t *l3_interface) noexcept = 0;
  virtual int l3_egress_update(uint32_t port, uint16_t vid,
                               const rofl::caddress_ll &src_mac,
                               const rofl::caddress_ll &dst_mac,
                               uint32_t *l3_interface_id) noexcept = 0;
  virtual int l3_egress_remove(uint32_t l3_interface) noexcept = 0;

  virtual int l3_unicast_host_add(const rofl::caddress_in4 &ipv4_dst,
                                  uint32_t l3_interface) noexcept = 0;
  virtual int l3_unicast_host_add(const rofl::caddress_in6 &ipv6_dst,
                                  uint32_t l3_interface) noexcept = 0;

  virtual int
  l3_unicast_host_remove(const rofl::caddress_in4 &ipv4_dst) noexcept = 0;
  virtual int
  l3_unicast_host_remove(const rofl::caddress_in6 &ipv6_dst) noexcept = 0;

  virtual int l3_unicast_route_add(const rofl::caddress_in4 &ipv4_dst,
                                   const rofl::caddress_in4 &mask,
                                   uint32_t l3_interface) noexcept = 0;
  virtual int l3_unicast_route_add(const rofl::caddress_in6 &ipv6_dst,
                                   const rofl::caddress_in6 &mask,
                                   uint32_t l3_interface) noexcept = 0;

  virtual int
  l3_unicast_route_remove(const rofl::caddress_in4 &ipv4_dst,
                          const rofl::caddress_in4 &mask) noexcept = 0;

  virtual int
  l3_unicast_route_remove(const rofl::caddress_in6 &ipv6_dst,
                          const rofl::caddress_in6 &mask) noexcept = 0;

  virtual int ingress_port_vlan_accept_all(uint32_t port) noexcept = 0;
  virtual int ingress_port_vlan_drop_accept_all(uint32_t port) noexcept = 0;
  virtual int ingress_port_vlan_add(uint32_t port, uint16_t vid,
                                    bool pvid) noexcept = 0;
  virtual int ingress_port_vlan_remove(uint32_t port, uint16_t vid,
                                       bool pvid) noexcept = 0;

  virtual int egress_port_vlan_accept_all(uint32_t port) noexcept = 0;
  virtual int egress_port_vlan_drop_accept_all(uint32_t port) noexcept = 0;

  virtual int egress_port_vlan_add(uint32_t port, uint16_t vid,
                                   bool untagged) noexcept = 0;
  virtual int egress_port_vlan_remove(uint32_t port, uint16_t vid) noexcept = 0;

  virtual int add_l2_overlay_flood(uint32_t tunnel_id,
                                   uint32_t lport_id) noexcept = 0;
  virtual int del_l2_overlay_flood(uint32_t tunnel_id,
                                   uint32_t lport_id) noexcept = 0;

  virtual int egress_bridge_port_vlan_add(uint32_t port, uint16_t vid,
                                          bool untagged) noexcept = 0;
  virtual int egress_bridge_port_vlan_remove(uint32_t port,
                                             uint16_t vid) noexcept = 0;

  virtual int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept = 0;
  virtual int subscribe_to(enum swi_flags flags) noexcept = 0;

  virtual bool is_connected() noexcept = 0;

  virtual int get_statistics(uint64_t port_no, uint32_t number_of_counters,
                             const sai_port_stat_t *counter_ids,
                             uint64_t *counters) noexcept = 0;

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
};

class nbi {
public:
  virtual ~nbi() = default;
  enum port_event {
    PORT_EVENT_ADD,
    PORT_EVENT_DEL,
  };

  enum port_status {
    PORT_STATUS_LOWER_DOWN = 0x01,
    PORT_STATUS_ADMIN_DOWN = 0x02,
  };

  struct port_notification_data {
    enum port_event ev;
    uint32_t port_id;
    std::string name;
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
  virtual void port_status_changed(uint32_t port,
                                   enum port_status) noexcept = 0;
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
