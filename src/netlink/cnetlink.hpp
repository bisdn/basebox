/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <tuple>

#include <netlink/cache.h>
#include <rofl/common/cthread.hpp>

#include "nl_bridge.hpp"
#include "nl_obj.hpp"
#include "sai.hpp"

namespace basebox {

// forward declaration
class nl_l3;
class nl_vlan;
class tap_manager;

class cnetlink final : public rofl::cthread_env {

  enum nl_cache_t {
    NL_ADDR_CACHE,
    NL_LINK_CACHE,
    NL_NEIGH_CACHE,
    NL_ROUTE_CACHE,
    NL_MAX_CACHE,
  };

public:
  cnetlink();
  ~cnetlink() override;

  /**
   * rtnl_link_put has to be called
   */
  struct rtnl_link *get_link_by_ifindex(int ifindex) const;
  struct rtnl_link *get_link(int ifindex, int family) const;
  struct rtnl_neigh *get_neighbour(int ifindex, struct nl_addr *a) const;

  bool is_bridge_interface(rtnl_link *l) const;
  bool is_bridge_configured(rtnl_link *l);
  int get_port_id(rtnl_link *l) const;
  int get_port_id(int ifindex) const;

  void resend_state() noexcept;

  void register_switch(switch_interface *) noexcept;
  void unregister_switch(switch_interface *) noexcept;

  void port_status_changed(uint32_t, enum nbi::port_status) noexcept;

  static void nl_cb_v2(struct nl_cache *cache, struct nl_object *old_obj,
                       struct nl_object *new_obj, uint64_t diff, int action,
                       void *data);

  void set_tapmanager(std::shared_ptr<tap_manager> tm);

  int send_nl_msg(nl_msg *msg);
  void learn_l2(uint32_t port_id, int fd, packet *pkt);

  void fdb_timeout(uint32_t port_id, uint16_t vid,
                   const rofl::caddress_ll &mac);

  void start() {
    if (running)
      return;
    running = true;
    thread.wakeup(this);
  }

  void stop() {
    running = false;
    thread.wakeup(this);
  }

private:
  // non copyable
  cnetlink(const cnetlink &other) = delete;
  cnetlink &operator=(const cnetlink &) = delete;

  enum timer {
    NL_TIMER_RESEND_STATE,
    NL_TIMER_RESYNC,
  };

  switch_interface *swi;

  rofl::cthread thread;
  struct nl_sock *sock_mon;
  struct nl_sock *sock_tx;
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
  bool lo_processed;
  std::deque<nl_obj> nl_objs;

  std::shared_ptr<nl_vlan> vlan;
  std::shared_ptr<nl_l3> l3;

  struct nl_pkt_in {
    nl_pkt_in(uint32_t port_id, int fd, packet *pkt)
        : port_id(port_id), fd(fd), pkt(pkt) {}
    uint32_t port_id;
    int fd;
    packet *pkt;
  };

  std::mutex pi_mutex;
  std::deque<nl_pkt_in> packet_in;

  struct fdb_ev {
    fdb_ev(uint32_t port_id, uint16_t vid, const rofl::caddress_ll &mac)
        : port_id(port_id), vid(vid), mac(mac) {}
    uint32_t port_id;
    uint16_t vid;
    rofl::caddress_ll mac;
  };

  std::mutex fdb_ev_mutex;
  std::deque<fdb_ev> fdb_evts;

  int handle_port_status_events();
  int handle_source_mac_learn();
  int handle_fdb_timeout();

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

  int set_nl_socket_buffer_sizes(nl_sock *sk);

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

  int config_lo_addr() noexcept;
};

} // end of namespace basebox
