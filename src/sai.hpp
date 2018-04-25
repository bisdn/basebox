#pragma once

#include <cinttypes>
#include <deque>

#include <rofl/common/caddress.h>

#include "utils.hpp"

namespace basebox {
class switch_interface {
public:
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

  virtual int l2_addr_remove_all_in_vlan(uint32_t port,
                                         uint16_t vid) noexcept = 0;
  virtual int l2_addr_add(uint32_t port, uint16_t vid,
                          const rofl::caddress_ll &mac, bool filtered,
                          bool permanent) noexcept = 0;
  virtual int l2_addr_remove(uint32_t port, uint16_t vid,
                             const rofl::caddress_ll &mac) noexcept = 0;

  virtual int l3_termination_add(uint32_t sport, uint16_t vid,
                                 const rofl::caddress_ll &dmac) noexcept = 0;
  virtual int l3_termination_remove(uint32_t sport, uint16_t vid,
                                    const rofl::caddress_ll &dmac) noexcept = 0;

  virtual int l3_egress_create(uint32_t port, uint16_t vid,
                               const rofl::caddress_ll &src_mac,
                               const rofl::caddress_ll &dst_mac,
                               uint32_t *l3_interface) noexcept = 0;
  virtual int l3_egress_remove(uint32_t l3_interface) noexcept = 0;

  virtual int l3_unicast_host_add(const rofl::caddress_in4 &ipv4_dst,
                                  uint32_t l3_interface) noexcept = 0;
  virtual int
  l3_unicast_host_remove(const rofl::caddress_in4 &ipv4_dst) noexcept = 0;

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
  virtual int egress_bridge_port_vlan_add(uint32_t port, uint16_t vid,
                                          bool untagged) noexcept = 0;
  virtual int egress_bridge_port_vlan_remove(uint32_t port,
                                             uint16_t vid) noexcept = 0;

  virtual int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept = 0;
  virtual int subscribe_to(enum swi_flags flags) noexcept = 0;

  virtual int get_statistics(uint64_t port_no, uint32_t number_of_counters,
                             const sai_port_stat_t *counter_ids,
                             uint64_t *counters) noexcept = 0;
};

class nbi {
public:
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
  virtual void register_switch(switch_interface *) noexcept = 0;
  virtual void resend_state() noexcept = 0;
  virtual void
  port_notification(std::deque<port_notification_data> &) noexcept = 0;
  virtual void port_status_changed(uint32_t port,
                                   enum port_status) noexcept = 0;
  virtual int enqueue(uint32_t port_id, basebox::packet *pkt) noexcept = 0;
  virtual int fdb_timeout(uint32_t port_id, uint16_t vid,
                          const rofl::caddress_ll &mac) noexcept = 0;
};
} // namespace basebox
