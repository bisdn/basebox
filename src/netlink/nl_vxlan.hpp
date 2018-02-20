#pragma once

#include <memory>

#include "nl_route_query.hpp"

extern "C" {
struct rtnl_link;
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

  int add_bridge_port(rtnl_link *);

  int create_endpoint_port(struct rtnl_link *link);

private:
  void create_access_port(struct rtnl_link *link, uint16_t vid, bool untagged);
  void create_access_ports(struct rtnl_link *access_port,
                           struct rtnl_link_bridge_vlan *br_vlan);

  uint32_t port_id;     // XXX TODO see below
  uint32_t next_hop_id; // XXX TODO handle these better and prevent id overflow
  switch_interface *sw;
  std::shared_ptr<tap_manager> tap_man;
  cnetlink *nl;
  nl_l3 *l3;
  nl_route_query rq;
};

} // namespace basebox
