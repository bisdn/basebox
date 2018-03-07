#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <tuple>

extern "C" {
struct nl_addr;
struct rtnl_link;
struct rtnl_neigh;
}

namespace basebox {

class cnetlink;
class switch_interface;
class tap_manager;

class nl_vxlan {
public:
  nl_vxlan(std::shared_ptr<tap_manager> tap_man, cnetlink *nl);
  ~nl_vxlan() {}

  void register_switch_interface(switch_interface *sw);

  int create_endpoint_port(struct rtnl_link *link);

  int add_l2_neigh(rtnl_neigh *neigh, rtnl_link *l);

  struct tunnel_port {
    uint32_t port_id;
    int tunnel_id;

    tunnel_port(int port_id, int tunnel_id)
        : port_id(port_id), tunnel_id(tunnel_id) {}
  };

  int get_tunnel_id(rtnl_link *vxlan_link, uint32_t *tunnel_id) noexcept;
  int get_tunnel_id(uint32_t vni, uint32_t *tunnel_id) noexcept;

  void create_access_port(uint32_t tunnel_id,
                          const std::string &access_port_name,
                          uint32_t pport_no, uint16_t vid, bool untagged);

private:
  int create_endpoint(rtnl_link *link,
                      std::unique_ptr<nl_addr, void (*)(nl_addr *)> local_,
                      std::unique_ptr<nl_addr, void (*)(nl_addr *)> group_,
                      uint32_t _next_hop_id, uint32_t *_port_id);

  int create_next_hop(rtnl_neigh *neigh, uint32_t *_next_hop_id);

  // XXX TODO handle these better and prevent id overflow
  uint32_t next_hop_id = 1;
  uint32_t port_id = 1 << 16 | 1;
  uint32_t tunnel_id = 1;

  std::map<uint32_t, tunnel_port> vni2tunnel;

  switch_interface *sw;
  std::shared_ptr<tap_manager> tap_man;
  cnetlink *nl;
};

} // namespace basebox
