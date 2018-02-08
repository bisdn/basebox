/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <tuple>

#include <netlink/cache.h>
#include <rofl/common/cthread.hpp>

#include "netlink/nl_bridge.hpp"
#include "netlink/nl_l3.hpp"
#include "netlink/nl_obj.hpp"
#include "sai.hpp"

namespace basebox {

class eNetLinkBase : public std::runtime_error {
public:
  eNetLinkBase(const std::string &__arg) : std::runtime_error(__arg){};
};
class eNetLinkCritical : public eNetLinkBase {
public:
  eNetLinkCritical(const std::string &__arg) : eNetLinkBase(__arg){};
};
class eNetLinkFailed : public eNetLinkBase {
public:
  eNetLinkFailed(const std::string &__arg) : eNetLinkBase(__arg){};
};

// forward declaration
class tap_manager;

class cnetlink final : public rofl::cthread_env {

  // non copyable
  cnetlink(const cnetlink &other) = delete;
  cnetlink &operator=(const cnetlink &) = delete;

  enum nl_cache_t {
    NL_ADDR_CACHE,
    NL_LINK_CACHE,
    NL_NEIGH_CACHE,
    NL_ROUTE_CACHE,
    NL_MAX_CACHE,
  };

  enum timer {
    NL_TIMER_RESEND_STATE,
    NL_TIMER_RESYNC,
  };

  enum link_type {
    LT_UNKNOWN = 0,
    LT_BRIDGE,
    LT_TUN,
    LT_MAX /* must be last */
  };

  switch_interface *swi;

  rofl::cthread thread;
  struct nl_sock *sock;
  struct nl_cache_mngr *mngr;
  std::vector<struct nl_cache *> caches;
  std::deque<std::tuple<uint32_t, enum nbi::port_status, int>>
      port_status_changes;
  std::mutex pc_mutex;

  std::shared_ptr<tap_manager> tap_man;

  nl_bridge *bridge;
  int nl_proc_max;
  bool running;
  bool rfd_scheduled;
  std::deque<nl_obj> nl_objs;

  nl_l3 l3;

  struct nl_dump_params params;
  char dump_buf[1024];

  std::map<std::string, enum link_type> kind2lt;
  std::vector<std::string> lt2names;

  void route_addr_apply(const nl_obj &obj);
  void route_link_apply(const nl_obj &obj);
  void route_neigh_apply(const nl_obj &obj);
  void route_route_apply(const nl_obj &obj);

  enum cnetlink_event_t {
    EVENT_NONE,
    EVENT_UPDATE_LINKS,
  };

  int load_from_file(const std::string &path);

  void init_caches();

  void destroy_caches();

  void handle_wakeup(rofl::cthread &thread) override;

  void handle_read_event(rofl::cthread &thread, int fd) override;

  void handle_write_event(rofl::cthread &thread, int fd) override;

  void handle_timeout(rofl::cthread &thread, uint32_t timer_id) override;

  void link_created(rtnl_link *) noexcept;
  void link_updated(rtnl_link *old_link, rtnl_link *new_link) noexcept;
  void link_deleted(rtnl_link *) noexcept;

  void neigh_ll_created(rtnl_neigh *neigh) noexcept;
  void neigh_ll_updated(rtnl_neigh *old_neigh, rtnl_neigh *new_neigh) noexcept;
  void neigh_ll_deleted(rtnl_neigh *neigh) noexcept;

  enum link_type kind_to_link_type(const char *type) const noexcept;

public:
  cnetlink(switch_interface *swi, std::shared_ptr<tap_manager> tap_man);
  ~cnetlink() override;

  struct rtnl_link *get_link_by_ifindex(int ifindex) const;

  void resend_state() noexcept;

  void register_switch(switch_interface *) noexcept;
  void unregister_switch(switch_interface *) noexcept;

  void port_status_changed(uint32_t, enum nbi::port_status) noexcept;

  static void nl_cb_v2(struct nl_cache *cache, struct nl_object *old_obj,
                       struct nl_object *new_obj, uint64_t diff, int action,
                       void *data);

  void start() {
    if (running)
      return;
    running = true;
    thread.wakeup();
  }

  void stop() {
    running = false;
    thread.wakeup();
  }
};

} // end of namespace basebox
