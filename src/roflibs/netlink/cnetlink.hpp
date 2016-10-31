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

#include "roflibs/netlink/crtlinks.hpp"
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
class eNetLinkNotFound : public eNetLinkBase {
public:
  eNetLinkNotFound(const std::string &__arg) : eNetLinkBase(__arg){};
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

  bool running;
  std::deque<std::pair<int, nl_obj>> nl_objs;

  crtlinks
      rtlinks; // all links in system => key:ifindex, value:crtlink instance
  std::map<int, crtneighs_ll> neighs_ll;

  std::set<int> missing_links;

  void route_link_apply(int action, const nl_obj &obj);
  void route_neigh_apply(int action, const nl_obj &obj);

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

  void handle_timeout(rofl::cthread &thread, uint32_t timer_id,
                      const std::list<unsigned int> &ttypes) override;

  void set_neigh_timeout();

  void link_created(const crtlink &link, uint32_t port_id) noexcept;
  void link_updated(const crtlink &link, uint32_t port_id) noexcept;
  void link_deleted(const crtlink &link, uint32_t port_id) noexcept;
  void neigh_ll_created(unsigned int ifindex, const crtneigh &neigh) noexcept;

  void neigh_ll_updated(unsigned int ifindex, const crtneigh &neigh) noexcept;

  void neigh_ll_deleted(unsigned int ifindex, const crtneigh &neigh) noexcept;

  uint32_t get_port_id(int ifindex) {
    return ifindex_to_registered_port.at(ifindex);
  }

  int get_ifindex(uint32_t port_id) {
    return registered_port_to_ifindex.at(port_id);
  }

public:
  friend std::ostream &operator<<(std::ostream &os, const cnetlink &netlink) {
    os << "<cnetlink>" << std::endl;
    os << netlink.rtlinks;
    return os;
  }

  void resend_state() noexcept;

  void register_switch(switch_interface *) noexcept;

  void port_status_changed(uint32_t, enum nbi::port_status) noexcept;

  static void nl_cb(struct nl_cache *cache, struct nl_object *obj, int action,
                    void *data);

  static cnetlink &get_instance();

  const crtlinks &get_links() const { return rtlinks; };

  crtlinks &set_links() { return rtlinks; };

  void register_link(uint32_t, std::string);

  void unregister_link(uint32_t id, std::string port_name);

  void start() {
    if (running)
      return;
    running = true;
    thread.wakeup();
  }

  void stop() { running = false; }

  void add_neigh_ll(int ifindex, uint16_t vlan, const rofl::caddress_ll &addr);

  void drop_neigh_ll(int ifindex, uint16_t vlan, const rofl::caddress_ll &addr);
};

}; // end of namespace rofcore

#endif /* CLINKCACHE_H_ */
