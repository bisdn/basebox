#pragma once

#include <memory>

extern "C" {
struct rtnl_link;
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

  int add_bridge_port(rtnl_link *);

private:
  void create_access_port(struct rtnl_link *link, uint16_t vid, bool untagged);
  void create_access_ports(struct rtnl_link *access_port,
                           struct rtnl_link_bridge_vlan *br_vlan);

  switch_interface *sw;
  std::shared_ptr<tap_manager> tap_man;
  cnetlink *nl;
};

} // namespace basebox
