#include "nl_bond.hpp"

#include <cassert>
#include <glog/logging.h>
#include <netlink/route/link.h>

#include "cnetlink.hpp"
#include "nl_output.hpp"
#include "sai.hpp"

namespace basebox {

nl_bond::nl_bond(cnetlink *nl) : swi(nullptr), nl(nl) {}

uint32_t nl_bond::get_lag_id(rtnl_link *bond) {
  assert(bond);

  auto it = ifi2lag.find(rtnl_link_get_ifindex(bond));
  if (it == ifi2lag.end()) {
    LOG(ERROR) << __FUNCTION__ << ": lag_id not found of lag "
               << OBJ_CAST(bond);
    return 0;
  }

  // XXX currently we do not deal with a real lag since we just deal with
  // a single port in the lag. Thus we just mimic the lag behaviour and map
  // everything on the linux bond/team to the single physical port
  auto rv_lm = lag_members.find(it->second);
  if (rv_lm == lag_members.end()) {
    VLOG(1) << ": no members in lag " << OBJ_CAST(bond);
    return 0;
  }

  return rv_lm->second;
}

int nl_bond::add_lag(rtnl_link *bond) {
  uint32_t lag_id = 0;
  int rv = 0;

  assert(bond);
  rv = swi->lag_create(&lag_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to create lag for "
               << OBJ_CAST(bond);
    return rv;
  }

  auto rv_emp =
      ifi2lag.emplace(std::make_pair(rtnl_link_get_ifindex(bond), lag_id));

  if (!rv_emp.second) {
    LOG(WARNING) << __FUNCTION__
                 << ": lag exists with lag_id=" << rv_emp.first->second
                 << " for bond " << OBJ_CAST(bond);
    return -EEXIST;
  }

  return rv;
}

int nl_bond::remove_lag(rtnl_link *bond) {
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

  return 0;
}

int nl_bond::add_lag_member(rtnl_link *bond, rtnl_link *link) {
  int rv = 0;
  auto it = ifi2lag.find(rtnl_link_get_ifindex(bond));
  if (it == ifi2lag.end()) {
    LOG(ERROR) << __FUNCTION__ << ": no lag_id found for " << OBJ_CAST(bond);
    return -EINVAL;
  }

  uint32_t port_id = nl->get_port_id(link);
  if (port_id == 0) {
    VLOG(1) << __FUNCTION__ << ": ignoring port " << OBJ_CAST(link);
    return -EINVAL;
  }

  auto lm_rv = lag_members.emplace(it->second, port_id);
  if (!lm_rv.second) {
    LOG(ERROR) << __FUNCTION__ << ": cannot add multiple ports to a lag";
  }

  swi->lag_add_member(it->second, port_id);

  return rv;
}

int nl_bond::remove_lag_member(rtnl_link *bond, rtnl_link *link) {
  int rv = 0;
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

  swi->lag_remove_member(it->second, port_id);
  lag_members.erase(lm_rv);

  return rv;
}

} // namespace basebox
