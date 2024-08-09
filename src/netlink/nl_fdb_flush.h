/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#include <glog/logging.h>

#include <netlink/attr.h>
#include <linux/rtnetlink.h>

#include "nl_output.h"

namespace basebox {

class nl_fdb_flush final {
  struct nl_sock *sock;

public:
  nl_fdb_flush() {
    sock = nl_socket_alloc();
    int err;
    if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0)
      LOG(FATAL) << __FUNCTION__ << ": Unable to connect netlink socket: %s"
                 << nl_geterror(err);
  }

  ~nl_fdb_flush() { nl_socket_free(sock); }

  /**
   * flush fdb
   */
  int flush_fdb(int ifindex, uint16_t vlan, uint8_t flags) {
    struct nl_msg *m;
    struct ndmsg ndm;
    int err;

    memset(&ndm, 0, sizeof(ndm));
    ndm.ndm_family = PF_BRIDGE;
    ndm.ndm_ifindex = ifindex;
    ndm.ndm_flags = flags;

    m = nlmsg_alloc_simple(RTM_DELNEIGH, NLM_F_REQUEST | NLM_F_BULK);
    if (!m)
      LOG(FATAL) << __FUNCTION__ << ": out of memory";

    if (nlmsg_append(m, &ndm, sizeof(ndm), NLMSG_ALIGNTO) < 0)
      LOG(FATAL) << __FUNCTION__ << ": out of memory";

    if (vlan > 0) {
      if (nla_put_u16(m, NDA_VLAN, vlan) < 0)
        LOG(FATAL) << __FUNCTION__ << ": out of memory";
    }

    if (flags > 0) {
      if (nla_put_u8(m, NDA_NDM_FLAGS_MASK, flags) < 0)
        LOG(FATAL) << __FUNCTION__ << ": out of memory";
    }

    err = nl_send_auto_complete(sock, m);
    nlmsg_free(m);
    if (err < 0)
      LOG(FATAL) << __FUNCTION__ << ": " << nl_geterror(err);

    if ((err = nl_recvmsgs_default(sock)) < 0) {
      LOG(FATAL) << __FUNCTION__ << ": " << nl_geterror(err);
    }

    return 0;
  }
};

} // namespace basebox
