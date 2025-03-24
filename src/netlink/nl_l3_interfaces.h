/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <netlink/addr.h>
#include <glog/logging.h>

namespace basebox {

struct net_params {
  net_params(nl_addr *addr, int ifindex) : addr(addr), ifindex(ifindex) {
    VLOG(4) << __FUNCTION__ << ": this=" << this;
    nl_addr_get(addr);
  }

  net_params(const net_params &p) : addr(p.addr), ifindex(p.ifindex) {
    VLOG(4) << __FUNCTION__ << ": this=" << this
            << ", refcnt=" << nl_object_get_refcnt(OBJ_CAST(addr));
    nl_addr_get(addr);
  }

  ~net_params() {
    VLOG(4) << __FUNCTION__ << ": this=" << this
            << ", refcnt=" << nl_object_get_refcnt(OBJ_CAST(addr));
    nl_addr_put(addr);
  }

  bool operator==(const int ifindex) const { return this->ifindex == ifindex; }

  nl_addr *addr;
  int ifindex;
};

struct nh_stub {
  nh_stub(nl_addr *nh, int ifindex, uint32_t nhid = 0)
      : nh(nh), ifindex(ifindex), nhid(nhid) {
    VLOG(4) << __FUNCTION__ << ": this=" << this;
    nl_addr_get(nh);
  }

  nh_stub(const nh_stub &r) : nh(r.nh), ifindex(r.ifindex), nhid(r.nhid) {
    VLOG(4) << __FUNCTION__ << ": this=" << this
            << ", refcnt=" << nl_object_get_refcnt(OBJ_CAST(nh));
    nl_addr_get(nh);
  }

  ~nh_stub() {
    VLOG(4) << __FUNCTION__ << ": this=" << this
            << ", refcnt=" << nl_object_get_refcnt(OBJ_CAST(nh));
    nl_addr_put(nh);
  }

  bool operator<(const nh_stub &other) const {

    int cmp = nl_addr_cmp(nh, other.nh);

    if (cmp < 0)
      return true;
    if (cmp > 0)
      return false;

    if (nhid < other.nhid)
      return true;
    if (nhid > other.nhid)
      return false;

    return ifindex < other.ifindex;
  }

  bool operator==(const nh_stub &other) const {
    if (nl_addr_cmp(nh, other.nh) != 0)
      return false;

    if (nhid != other.nhid)
      return false;

    return ifindex == other.ifindex;
  }

  nl_addr *nh;
  int ifindex;
  uint32_t nhid;
};

struct nh_params {
  nh_params(net_params np, nh_stub nh) : np(np), nh(nh) {}
  net_params np;
  nh_stub nh;
};

class net_reachable {
public:
  virtual ~net_reachable() = default;
  virtual void net_reachable_notification(struct net_params) noexcept = 0;
};

class nh_reachable {
public:
  virtual ~nh_reachable() = default;
  virtual void nh_reachable_notification(struct rtnl_neigh *,
                                         struct nh_params) noexcept = 0;
};

class nh_unreachable {
public:
  virtual ~nh_unreachable() = default;
  virtual void nh_unreachable_notification(struct rtnl_neigh *,
                                           struct nh_params) noexcept = 0;
};

} // namespace basebox
