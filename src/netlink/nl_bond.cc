/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nl_bond.h"

#include <cassert>
#include <glog/logging.h>
#include <linux/if_bridge.h>
#include <netlink/route/link.h>
#include <netlink/route/link/bonding.h>

#include "cnetlink.h"
#include "nl_output.h"
#include "sai.h"

namespace basebox {

nl_bond::nl_bond(cnetlink *nl) : swi(nullptr), nl(nl) {}

void nl_bond::clear() noexcept {
  lag_members.clear();
  ifi2lag.clear();
}

uint32_t nl_bond::get_lag_id(rtnl_link *bond) {
  assert(bond);

  return get_lag_id(rtnl_link_get_ifindex(bond));
}

uint32_t nl_bond::get_lag_id(int ifindex) {
  auto it = ifi2lag.find(ifindex);
  if (it == ifi2lag.end()) {
    VLOG(1) << __FUNCTION__ << ": lag_id not found for if=" << ifindex;
    return 0;
  }

  return it->second;
}

std::set<uint32_t> nl_bond::get_members(rtnl_link *bond) {
  auto it = ifi2lag.find(rtnl_link_get_ifindex(bond));
  if (it == ifi2lag.end()) {
    LOG(WARNING) << __FUNCTION__ << ": lag does not exist for "
                 << OBJ_CAST(bond);
    return {};
  }

  auto mem_it = lag_members.find(it->second);
  if (mem_it == lag_members.end()) {
    LOG(WARNING) << __FUNCTION__ << ": lag does not exist for "
                 << OBJ_CAST(bond);
    return {};
  }

  return mem_it->second;
}

std::set<uint32_t> nl_bond::get_members_by_port_id(uint32_t port_id) {
  auto mem_it = lag_members.find(port_id);
  if (mem_it == lag_members.end()) {
    LOG(WARNING) << __FUNCTION__
                 << ": lag does not exist for port_id=" << port_id;
    return {};
  }

  return mem_it->second;
}

int nl_bond::update_lag(rtnl_link *old_link, rtnl_link *new_link) {
#ifdef HAVE_RTNL_LINK_BOND_GET_MODE
  VLOG(1) << __FUNCTION__ << ": updating bond interface ";
  int rv = 0;

  uint8_t o_mode, n_mode;
  uint32_t lag_id = nl->get_port_id(new_link);

  if (lag_id == 0) {
    VLOG(2) << __FUNCTION__ ": ignoring update for foreign bond interface "
	    << OBJ_CAST(new_link);
    return 0;
  }

  rv = rtnl_link_bond_get_mode(old_link, &o_mode);
  if (rv < 0) {
    VLOG(1) << __FUNCTION__ << ": failed to get mode for "
            << OBJ_CAST(new_link);
  }

  rv = rtnl_link_bond_get_mode(new_link, &n_mode);
  if (rv < 0) {
    VLOG(1) << __FUNCTION__ << ": failed to get mode for "
            << OBJ_CAST(new_link);
  }

  if (o_mode != n_mode) {
    VLOG(1) << __FUNCTION__ << ": bond mode updated "
            << static_cast<uint32_t>(n_mode);
    rv = swi->lag_set_mode(lag_id, n_mode);

    if (rv < 0) {
      VLOG(1) << __FUNCTION__ << ": failed to set active state for "
              << OBJ_CAST(new_link);
    }

    return 0;
  }

  nl->add_l3_configuration(new_link);
#endif

  return 0;
}

int nl_bond::add_lag(rtnl_link *bond) {
  uint32_t lag_id = 0;
  int rv = 0;
#ifdef HAVE_RTNL_LINK_BOND_GET_MODE
  uint8_t mode;

  rtnl_link_bond_get_mode(bond, &mode);

  assert(bond);
  rv = swi->lag_create(&lag_id, rtnl_link_get_name(bond), mode);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to create lag for "
               << OBJ_CAST(bond);
    return rv;
  }

  rv = lag_id;

  auto rv_emp =
      ifi2lag.emplace(std::make_pair(rtnl_link_get_ifindex(bond), lag_id));

  if (!rv_emp.second) {
    VLOG(1) << __FUNCTION__
            << ": lag exists with lag_id=" << rv_emp.first->second
            << " for bond " << OBJ_CAST(bond);
    rv = rv_emp.first->second;

    if (lag_id != rv_emp.first->second)
      swi->lag_remove(lag_id);
  }

#endif

  return rv;
}

int nl_bond::remove_lag(rtnl_link *bond) {
#ifdef HAVE_RTNL_LINK_BOND_GET_MODE
  int rv = 0;
  auto it = ifi2lag.find(rtnl_link_get_ifindex(bond));

  if (it == ifi2lag.end()) {
    LOG(WARNING) << __FUNCTION__ << ": lag does not exist for "
                 << OBJ_CAST(bond);
    return -ENODEV;
  }

  rv = swi->lag_remove(it->second);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__
               << ": failed to remove lag with lag_id=" << it->second
               << " for bond " << OBJ_CAST(bond);
    return rv;
  }

  ifi2lag.erase(it);
#endif

  return 0;
}

int nl_bond::add_lag_member(rtnl_link *bond, rtnl_link *link) {
  int rv = 0;
#ifdef HAVE_RTNL_LINK_BOND_GET_MODE
  uint32_t lag_id;
  uint8_t state = 0;

  auto it = ifi2lag.find(rtnl_link_get_ifindex(bond));
  if (it == ifi2lag.end()) {
    VLOG(1) << __FUNCTION__ << ": no lag_id found creating new for "
            << OBJ_CAST(bond);

    rv = add_lag(bond);
    if (rv < 0)
      return rv;

    lag_id = rv;
  } else {
    lag_id = it->second;
  }

  uint32_t port_id = nl->get_port_id(link);
  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": ignoring port " << OBJ_CAST(link);
    return -EINVAL;
  }

  auto mem_it = lag_members.find(it->second);
  if (mem_it == lag_members.end()) { // No ports in lag
    std::set<uint32_t> members;
    members.insert(port_id);
    auto lm_rv = lag_members.emplace(lag_id, members);
  } else {
    mem_it->second.insert(port_id);
  }

  rv = rtnl_link_bond_slave_get_state(link, &state);
  if (rv < 0) {
    VLOG(1) << __FUNCTION__ << ": failed to get slave state for "
            << OBJ_CAST(link);
  }

  rv = swi->lag_add_member(lag_id, port_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed add member " << port_id;
    return -EINVAL;
  }

  rv = swi->lag_set_member_active(lag_id, port_id, state == 0);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed set active member " << port_id;
    return -EINVAL;
  }

  if (rtnl_link_get_master(bond)) {
    // check bridge attachement
    auto br_link = nl->get_link(rtnl_link_get_ifindex(bond), AF_BRIDGE);
    if (br_link) {
      VLOG(2) << __FUNCTION__
              << ": bond was already bridge slave: " << OBJ_CAST(br_link);
      nl->link_created(br_link);

      if (nl->get_bridge_stp_state() == 0)
        return rv;

      auto new_state = rtnl_link_bridge_get_port_state(br_link);
      std::string state;

      switch (new_state) {
      case BR_STATE_FORWARDING:
        state = "forward";
        break;
      case BR_STATE_BLOCKING:
        state = "block";
        break;
      case BR_STATE_DISABLED:
        state = "disable";
        break;
      case BR_STATE_LISTENING:
        state = "listen";
        break;
      case BR_STATE_LEARNING:
        state = "learn";
        break;
      default:
        VLOG(1) << __FUNCTION__ << ": stp state change not supported";
        return rv;
      }

      swi->ofdpa_stg_state_port_set(port_id, 1, state);
    }

    rv = nl->set_bridge_port_vlan_tpid(br_link);
    if (rv < 0)
      LOG(ERROR) << __FUNCTION__
                 << ": failed to set egress TPID entry for port "
                 << OBJ_CAST(link);
  } else {
    std::deque<uint16_t> vlans;

    nl->get_vlans(rtnl_link_get_ifindex(bond), &vlans);
    for (auto vid : vlans) {
      swi->ingress_port_vlan_add(port_id, vid, false);
      swi->egress_port_vlan_add(port_id, vid, false);
    }
  }

  // XXX FIXME check for vlan interfaces
  // Adding an IP address here will ensure that every slave that is
  // added will retry to add the address. It will not be written to the
  // ASIC, but repeated messages will be seen
  // This should be done in ::add_lag, but for currently unknown reasons
  // this fails when the lag has no members yet. So keep it here for now.
  nl->add_l3_configuration(bond);
#endif

  return rv;
}

int nl_bond::remove_lag_member(rtnl_link *link) {
  assert(link);

  int master_id = rtnl_link_get_master(link);
  auto master = nl->get_link(master_id, AF_UNSPEC);

  return remove_lag_member(master, link);
}

int nl_bond::remove_lag_member(rtnl_link *bond, rtnl_link *link) {
  int rv = 0;
#ifdef HAVE_RTNL_LINK_BOND_GET_MODE
  auto it = ifi2lag.find(rtnl_link_get_ifindex(bond));
  if (it == ifi2lag.end()) {
    LOG(FATAL) << __FUNCTION__ << ": no lag_id found for " << OBJ_CAST(bond);
  }

  uint32_t port_id = nl->get_port_id(link);
  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": ignore invalid lag port " << OBJ_CAST(link);
    return -EINVAL;
  }

  auto lm_rv = lag_members.find(it->second);
  if (lm_rv == lag_members.end()) {
    VLOG(1) << __FUNCTION__ << ": ignore invalid attached port "
            << OBJ_CAST(link);
    return -EINVAL;
  }

  rv = swi->lag_remove_member(it->second, port_id);
  lag_members.erase(lm_rv);

  if (nl->is_bridge_interface(bond)) {
    swi->ofdpa_stg_state_port_set(port_id, 1, "forward");

    auto br_link = nl->get_link(rtnl_link_get_ifindex(bond), AF_BRIDGE);
    rv = nl->unset_bridge_port_vlan_tpid(br_link);

    if (rv < 0)
      LOG(ERROR) << __FUNCTION__
                 << ": failed to set egress TPID entry for port "
                 << OBJ_CAST(link);
  } else {
    std::deque<uint16_t> vlans;

    nl->get_vlans(rtnl_link_get_ifindex(bond), &vlans);

    if (lm_rv->second.empty())
      nl->remove_l3_configuration(bond);

    if (nl->is_bridge_interface(bond))
      swi->ofdpa_stg_state_port_set(port_id, 1, "forward");

    for (auto vid : vlans) {
      swi->ingress_port_vlan_remove(port_id, vid, false);
      swi->egress_port_vlan_remove(port_id, vid);
    }
  }
#endif

  return rv;
}

int nl_bond::update_lag_member(rtnl_link *old_slave, rtnl_link *new_slave) {
#ifdef HAVE_RTNL_LINK_BOND_GET_MODE
  assert(new_slave);

  int rv;
  uint8_t new_state;
  uint8_t old_state;

  int n_master_id = rtnl_link_get_master(new_slave);
  int o_master_id = rtnl_link_get_master(old_slave);
  auto new_master = nl->get_link(n_master_id, AF_UNSPEC);
  auto old_master = nl->get_link(o_master_id, AF_UNSPEC);
  auto port_id = nl->get_port_id(new_slave);

  if (old_master != new_master || new_master == 0) {
    return -EINVAL;
  }

  rv = rtnl_link_bond_slave_get_state(new_slave, &new_state);
  if (rv != 0) {
    VLOG(1) << __FUNCTION__ << ": failed to get state";
    return -EINVAL;
  }
  rtnl_link_bond_slave_get_state(old_slave, &old_state);
  if (rv != 0) {
    VLOG(1) << __FUNCTION__ << ": failed to get state";
    return -EINVAL;
  }

  rv = swi->lag_set_member_active(nl->get_port_id(new_master), port_id,
                                  new_state == 0);
#endif
  return 0;
}

} // namespace basebox
