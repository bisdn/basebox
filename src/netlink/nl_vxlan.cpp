#include <cassert>
#include <deque>
#include <string>
#include <tuple>

#include <netlink/route/link.h>
#include <netlink/route/link/bridge.h>

#include "cnetlink.hpp"
#include "netlink-utils.hpp"
#include "nl_output.hpp"
#include "nl_vxlan.hpp"
#include "tap_manager.hpp"

namespace basebox {

nl_vxlan::nl_vxlan(std::shared_ptr<tap_manager> tap_man, cnetlink *nl)
    : sw(nullptr), tap_man(tap_man), nl(nl) {}

void nl_vxlan::register_switch_interface(switch_interface *sw) {
  this->sw = sw;
}

struct rtnl_link_bridge_vlan
get_matching_vlans(const struct rtnl_link_bridge_vlan *a,
                   const struct rtnl_link_bridge_vlan *b) {
  struct rtnl_link_bridge_vlan r;

  assert(a);
  assert(b);

  for (int k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
    r.vlan_bitmap[k] = a->vlan_bitmap[k] & b->vlan_bitmap[k];
    r.untagged_bitmap[k] = a->untagged_bitmap[k] & b->untagged_bitmap[k];
  }

  return r;
}

// XXX code duplication
static int find_next_bit(int i, uint32_t x) {
  int j;

  if (i >= 32)
    return -1;

  /* find first bit */
  if (i < 0)
    return __builtin_ffs(x);

  /* mask off prior finds to get next */
  j = __builtin_ffs(x >> i);
  return j ? j + i : 0;
}

void nl_vxlan::create_access_port(struct rtnl_link *access_port, uint16_t vid,
                                  bool untagged) {
  assert(access_port);
  assert(sw);

  static uint32_t port_id = 1 << 16 | 1 << 8;
  // XXX FIXME mark access ports or find another way to del with fdb changes
  const uint32_t port =
      tap_man->get_port_id(rtnl_link_get_ifindex(access_port));
  std::string port_name(rtnl_link_get_name(access_port));
  port_name += "." + std::to_string(vid);
  sw->tunnel_access_port_create(port_id, port_name, port, vid);
  // XXX FIXME next
  // sw->tunnel_port_tenant_add(port_id, XXX);
  port_id++;
}

// XXX loop through vlans (again code duplication
// TODO shouldn't be void
void nl_vxlan::create_access_ports(struct rtnl_link *access_port,
                                   struct rtnl_link_bridge_vlan *br_vlan) {
  assert(access_port);
  assert(br_vlan);

  for (int k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
    int base_bit;
    uint32_t a = br_vlan->vlan_bitmap[k];

    base_bit = k * 32;
    int i = -1;
    int done = 0;
    while (!done) {
      int j = find_next_bit(i, a);
      if (j > 0) {
        int vid = j - 1 + base_bit;
        bool egress_untagged = false;

        // check if egress is untagged
        if (br_vlan->untagged_bitmap[k] & 1 << (j - 1)) {
          egress_untagged = true;
        }

        LOG(INFO) << __FUNCTION__ << ": same vlan " << vid;
#if 0
        if (egress_vlan_filtered) {
          sw->egress_bridge_port_vlan_add(port, vid, egress_untagged);
        }

        if (ingress_vlan_filtered) {
          sw->ingress_port_vlan_add(port, vid, br_vlan->pvid == vid);
        }
#endif // 0

        // create access ports for each overlapping vlan
        bool untagged = false;
        create_access_port(access_port, vid, untagged);

        i = j;
      } else {
        done = 1;
      }
    }
  }
}

int nl_vxlan::add_bridge_port(rtnl_link *link) {
  assert(link);

  std::unique_ptr<rtnl_link, void (*)(rtnl_link *)> filter(rtnl_link_alloc(),
                                                           &rtnl_link_put);
  assert(filter && "out of memory");
  rtnl_link_set_family(filter.get(), AF_BRIDGE);
  rtnl_link_set_master(filter.get(), rtnl_link_get_master(link));

  std::deque<rtnl_link *> bridge_ports;
  std::tuple<nl_vxlan *, std::deque<rtnl_link *> *> params = {this,
                                                              &bridge_ports};
  auto cache = nl->get_cache(cnetlink::NL_LINK_CACHE);
  assert(cache);

  nl_cache_foreach_filter(
      cache, OBJ_CAST(filter.get()),
      [](struct nl_object *obj, void *arg) {
        std::tuple<nl_vxlan *, std::deque<rtnl_link *> *> *params =
            static_cast<std::tuple<nl_vxlan *, std::deque<rtnl_link *> *> *>(
                arg);

        LOG(INFO) << __FUNCTION__ << ": got bridge link name="
                  << rtnl_link_get_name(LINK_CAST(obj));

        if (!rtnl_link_get_master(LINK_CAST(obj))) {
          return;
        }

        if (!std::get<0>(*params)->tap_man->get_port_id(
                rtnl_link_get_ifindex(LINK_CAST(obj))))
          return;

        LOG(INFO) << __FUNCTION__ << ": XXX found" << obj;
        // nl_object_get(obj);
        std::get<1>(*params)->push_back(LINK_CAST(obj)); // XXX unique_ptr?
      },
      &params);

  if (bridge_ports.size() == 0) {
    // l2 domain so far
    return 0;
  }

  // check for each bridge port the vlan overlap
  auto vxlan_vlans = rtnl_link_bridge_get_port_vlan(link);

  // XXX pvid is currently not taken into account
  for (auto br_port : bridge_ports) {
    auto br_port_vlans = rtnl_link_bridge_get_port_vlan(br_port);
    auto matching_vlans = get_matching_vlans(br_port_vlans, vxlan_vlans);
    // XXX FIXME skip other endpoint ports
    create_access_ports(br_port, &matching_vlans);
  }

  return 0;
}

} // namespace basebox
