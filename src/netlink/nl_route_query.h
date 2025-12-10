// SPDX-FileCopyrightText: © 2019 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#pragma once

#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#include <glog/logging.h>

#include <netlink/attr.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>

#include <linux/rtnetlink.h>

#include "nl_output.h"

namespace basebox {

class nl_route_query final {
  struct nl_sock *sock;

  static void parse_cb(struct nl_object *obj, void *arg) {
    assert(arg);
    VLOG(2) << __FUNCTION__ << ": got obj " << obj;
    nl_object_get(obj);
    *static_cast<nl_object **>(arg) = obj;
  }

  static int cb(struct nl_msg *msg, void *arg) {
    int err;

    assert(arg);

    if ((err = nl_msg_parse(msg, &parse_cb, arg)) < 0)
      LOG(FATAL) << __FUNCTION__
                 << ": Unable to parse object: " << nl_geterror(err);

    return 0;
  }

public:
  nl_route_query() {
    sock = nl_socket_alloc();
    int err;
    if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0)
      LOG(FATAL) << __FUNCTION__ << ": Unable to connect netlink socket: %s"
                 << nl_geterror(err);
  }

  ~nl_route_query() { nl_socket_free(sock); }

  /**
   * return object has to be freed using nl_object_put
   */
  struct rtnl_route *query_route(struct nl_addr *dst, uint32_t flags = 0) {
    int err = 1;
    struct nl_msg *m;
    struct rtmsg rmsg;
    memset(&rmsg, 0, sizeof(rmsg));
    rmsg.rtm_family = nl_addr_get_family(dst);
    rmsg.rtm_dst_len = nl_addr_get_prefixlen(dst);
    rmsg.rtm_flags = flags;

    struct nl_object *route = nullptr;
    nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, cb, &route);

    m = nlmsg_alloc_simple(RTM_GETROUTE, 0);
    if (!m)
      LOG(FATAL) << __FUNCTION__ << ": out of memory";
    if (nlmsg_append(m, &rmsg, sizeof(rmsg), NLMSG_ALIGNTO) < 0)
      LOG(FATAL) << __FUNCTION__ << ": out of memory";
    if (nla_put_addr(m, RTA_DST, dst) < 0)
      LOG(FATAL) << __FUNCTION__ << ": out of memory";

    VLOG(2) << __FUNCTION__ << ": query route for dst=" << dst;
    err = nl_send_auto_complete(sock, m);
    nlmsg_free(m);
    if (err < 0)
      LOG(FATAL) << __FUNCTION__ << ":%s" << nl_geterror(err);

    if ((err = nl_recvmsgs_default(sock)) < 0) {
      /*
       * if there is no default route, and no matching route, the kernel will
       * respond with -ENETUNREACH or -EHOSTUNREACH, which libnl translates to
       * -NLE_FAILURE resp -NLE_HOSTUNREACH.
       */
      if (err == -NLE_HOSTUNREACH || err == -NLE_FAILURE)
        return nullptr;

      LOG(FATAL) << __FUNCTION__ << ":%s" << nl_geterror(err);
    }

    VLOG(3) << __FUNCTION__ << ": got route " << route;

    return reinterpret_cast<struct rtnl_route *>(route);
  }
};

} // namespace basebox
