#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "nl_route_query.hpp"

extern "C" {
struct rtnl_link;
struct rtnl_neigh;
}

namespace basebox {

class cnetlink;
class nl_l3;
class switch_interface;
class tap_manager;

class nl_vxlan {
public:
  nl_vxlan(std::shared_ptr<tap_manager> tap_man, nl_l3 *l3, cnetlink *nl);
  ~nl_vxlan() {}

  void register_switch_interface(switch_interface *sw);

  int add_bridge_port(rtnl_link *vxlan_link, rtnl_link *br_port);

  int create_endpoint_port(struct rtnl_link *link);

  int add_l2_neigh(rtnl_neigh *neigh, rtnl_link *l);

  struct tunnel_port {
    uint32_t port_id;
    int tunnel_id;

    tunnel_port(int port_id, int tunnel_id)
        : port_id(port_id), tunnel_id(tunnel_id) {}
  };

private:
  void create_access_port(uint32_t tunnel_id, struct rtnl_link *link,
                          uint16_t vid, bool untagged);
  void create_access_ports(uint32_t tunnel_id, struct rtnl_link *access_port,
                           struct rtnl_link_bridge_vlan *br_vlan);

  // XXX TODO handle these better and prevent id overflow
  uint32_t next_hop_id = 1;
  uint32_t port_id = 1 << 16 | 1;
  uint32_t tunnel_id = 1;

  std::map<uint32_t, tunnel_port> vni2tunnel;

  switch_interface *sw;
  std::shared_ptr<tap_manager> tap_man;
  cnetlink *nl;
  nl_l3 *l3;
  nl_route_query rq;
};

} // namespace basebox
