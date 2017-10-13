/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CNETLINK_H_
#define CNETLINK_H_ 1

#include <deque>
#include <exception>
#include <mutex>
#include <tuple>

#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>
#include <rofl/common/cthread.hpp>

#include "roflibs/netlink/nl_obj.hpp"
#include "roflibs/netlink/ofdpa_bridge.hpp"
#include "roflibs/netlink/sai.hpp"

namespace rofcore {

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

class cnetlink : public rofl::cthread_env {
  enum nl_cache_t {
    NL_LINK_CACHE,
    NL_NEIGH_CACHE,
  };

  enum timer {
    NL_TIMER_RESEND_STATE,
    NL_TIMER_RESYNC,
  };

  switch_interface *swi;

  rofl::cthread thread;
  struct nl_sock *sock;
  struct nl_cache_mngr *mngr;
  std::map<enum nl_cache_t, struct nl_cache *> caches;
  std::map<std::string, uint32_t> registered_ports;
  std::mutex rp_mutex;
  std::map<int, uint32_t> ifindex_to_registered_port;
  std::map<uint32_t, int> registered_port_to_ifindex;
  std::deque<std::tuple<uint32_t, enum nbi::port_status, int>>
      port_status_changes;
  std::mutex pc_mutex;

  ofdpa_bridge *bridge;
  int nl_proc_max;
  bool running;
  bool rfd_scheduled;
  std::deque<nl_obj> nl_objs;

  void route_link_apply(const nl_obj &obj);
  void route_neigh_apply(const nl_obj &obj);

  enum cnetlink_event_t {
    EVENT_NONE,
    EVENT_UPDATE_LINKS,
  };

  cnetlink(switch_interface *);

  ~cnetlink() override;

  int load_from_file(const std::string &path);

  void init_caches();

  void destroy_caches();

  void handle_wakeup(rofl::cthread &thread) override;

  void handle_read_event(rofl::cthread &thread, int fd) override;

  void handle_write_event(rofl::cthread &thread, int fd) override;

  void handle_timeout(rofl::cthread &thread, uint32_t timer_id) override;

  void link_created(rtnl_link *, uint32_t port_id) noexcept;
  void link_updated(rtnl_link *old_link, rtnl_link *new_link,
                    uint32_t port_id) noexcept;
  void link_deleted(rtnl_link *, uint32_t port_id) noexcept;

  void neigh_ll_created(rtnl_neigh *neigh) noexcept;
  void neigh_ll_updated(rtnl_neigh *old_neigh, rtnl_neigh *new_neigh) noexcept;
  void neigh_ll_deleted(rtnl_neigh *neigh) noexcept;

  uint32_t get_port_id(int ifindex) {
    return ifindex_to_registered_port.at(ifindex);
  }

  int get_ifindex(uint32_t port_id) {
    return registered_port_to_ifindex.at(port_id);
  }

public:
  void resend_state() noexcept;

  void register_switch(switch_interface *) noexcept;
  void unregister_switch(switch_interface *) noexcept;

  void port_status_changed(uint32_t, enum nbi::port_status) noexcept;

  static void nl_cb(struct nl_cache *cache, struct nl_object *obj, int action,
                    void *data);
  static void nl_cb_v2(struct nl_cache *cache, struct nl_object *old_obj,
                       struct nl_object *new_obj, uint64_t diff, int action,
                       void *data);

  static cnetlink &get_instance();

  void register_link(uint32_t, std::string);

  void unregister_link(uint32_t id, std::string port_name);

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

} // end of namespace rofcore

#endif /* CLINKCACHE_H_ */
