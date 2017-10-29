/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <fstream>
#include <glog/logging.h>
#include <iterator>
#include <sys/socket.h>
#include <linux/if.h>

#include "cnetlink.hpp"
#define LINK_CAST(obj) reinterpret_cast<struct rtnl_link *>(obj)
#define NEIGH_CAST(obj) reinterpret_cast<struct rtnl_neigh *>(obj)
namespace basebox {

cnetlink::cnetlink(switch_interface *swi)
    : swi(swi), thread(this), bridge(nullptr), nl_proc_max(10), running(false),
      rfd_scheduled(false) {

  sock = nl_socket_alloc();
  if (NULL == sock) {
    LOG(FATAL) << "cnetlink: failed to create netlink socket" << __FUNCTION__;
    throw eNetLinkCritical(__FUNCTION__);
  }

  try {
    thread.start("netlink");
    init_caches();
  } catch (...) {
    LOG(FATAL) << "cnetlink: caught unkown exception during " << __FUNCTION__;
  }
}

cnetlink::~cnetlink() {
  thread.stop();
  delete bridge;
  destroy_caches();
  nl_socket_free(sock);
}

int cnetlink::load_from_file(const std::string &path) {
  std::ifstream file;
  int out = -1;
  file.open(path);
  if (file.is_open()) {
    while (!file.eof()) {
      file >> out;
    }
  }
  if (out < 0) {
    LOG(FATAL) << "cnetlink: failed to load " << path;
    throw eNetLinkCritical(__FUNCTION__);
  }
  return out;
}

void cnetlink::init_caches() {

  int rc = nl_cache_mngr_alloc(sock, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);

  if (rc < 0) {
    LOG(FATAL)
        << "cnetlink::init_caches() failed to allocate netlink cache manager";
    throw eNetLinkCritical("cnetlink::init_caches()");
  }

  int rx_size = load_from_file("/proc/sys/net/core/rmem_max");
  int tx_size = load_from_file("/proc/sys/net/core/wmem_max");

  if (0 != nl_socket_set_buffer_size(sock, rx_size, tx_size)) {
    LOG(FATAL) << "cnetlink: failed to resize socket buffers";
    throw eNetLinkCritical(__FUNCTION__);
  }
  nl_socket_set_msg_buf_size(sock, rx_size);

  caches[NL_LINK_CACHE] = NULL;
  caches[NL_NEIGH_CACHE] = NULL;

  rc = rtnl_link_alloc_cache_flags(sock, AF_UNSPEC, &caches[NL_LINK_CACHE],
                                   NL_CACHE_AF_ITER);
  if (0 != rc) {
    LOG(FATAL)
        << "cnetlink::init_caches() rtnl_link_alloc_cache_flags failed rc="
        << rc;
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_LINK_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, NULL);
  if (0 != rc) {
    LOG(FATAL) << "cnetlink::init_caches() add route/link to cache mngr";
  }

  rc = rtnl_neigh_alloc_cache_flags(sock, &caches[NL_NEIGH_CACHE],
                                    NL_CACHE_AF_ITER);
  if (0 != rc) {
    LOG(FATAL)
        << "cnetlink::init_caches() rtnl_link_alloc_cache_flags failed rc="
        << rc;
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_NEIGH_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, NULL);
  if (0 != rc) {
    LOG(FATAL) << "cnetlink::init_caches() add route/neigh to cache mngr";
  }

  thread.wakeup();
}

void cnetlink::destroy_caches() { nl_cache_mngr_free(mngr); }

cnetlink &cnetlink::get_instance() {
  static cnetlink instance(nullptr);
  return instance;
}

void cnetlink::register_link(uint32_t id, std::string port_name) {
  {
    std::lock_guard<std::mutex> lock(rp_mutex);
    registered_ports.insert(std::make_pair(port_name, id));
  }
  start();
}

void cnetlink::unregister_link(uint32_t id, std::string port_name) {
  {
    std::lock_guard<std::mutex> lock(rp_mutex);
    registered_ports.erase(port_name);
  }
}

void cnetlink::handle_wakeup(rofl::cthread &thread) {
  bool do_wakeup = false;

  if (not rfd_scheduled) {
    try {
      thread.add_read_fd(nl_cache_mngr_get_fd(mngr), true, false);
      rfd_scheduled = true;
    } catch (std::exception &e) {
      LOG(FATAL) << "caught " << e.what();
    }
  }

  // loop through nl_objs
  for (int cnt = 0; cnt < nl_proc_max && nl_objs.size() && running; cnt++) {
    auto &obj = nl_objs.front();

    switch (obj.get_msg_type()) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
      route_link_apply(obj);
      break;
    case RTM_NEWNEIGH:
    case RTM_DELNEIGH:
      route_neigh_apply(obj);
    default:
      break;
    }

    nl_objs.pop_front();
  }

  std::deque<std::tuple<uint32_t, enum nbi::port_status, int>> _pc_changes;
  std::deque<std::tuple<uint32_t, enum nbi::port_status, int>> _pc_back;

  {
    std::lock_guard<std::mutex> scoped_lock(pc_mutex);
    _pc_changes.swap(port_status_changes);
  }

  for (auto change : _pc_changes) {
    int ifindex, rv;
    rtnl_link *lchange, *link;

    // get ifindex
    try {
      ifindex = get_ifindex(std::get<0>(change));
    } catch (std::out_of_range &e) {
      // XXX not yet registered push back, this should be done async
      int n_retries = std::get<2>(change);
      if (n_retries < 10) {
        std::get<2>(change) = ++n_retries;
        _pc_back.push_back(change);
      } else {
        LOG(ERROR) << __FUNCTION__
                   << ": no ifindex of port_id=" << std::get<0>(change)
                   << " found";
      }
      continue;
    }

    VLOG(1) << __FUNCTION__ << ": update link with ifindex=" << ifindex
            << " port_id=" << std::get<0>(change) << " status=" << std::hex
            << std::get<1>(change) << std::dec << ") ";

    // lookup link
    link = rtnl_link_get(caches[NL_LINK_CACHE], ifindex);
    if (!link) {
      LOG(ERROR) << __FUNCTION__ << ": link not found with ifindex=" << ifindex;
      continue;
    }

    // change request
    lchange = rtnl_link_alloc();
    if (!lchange) {
      LOG(ERROR) << __FUNCTION__ << ": out of memory";
      rtnl_link_put(link);
      continue;
    }

    int flags = rtnl_link_get_flags(link);
    // check admin state change
    if (!((flags & IFF_UP) &&
          !(std::get<1>(change) & nbi::PORT_STATUS_ADMIN_DOWN))) {
      if (std::get<1>(change) & nbi::PORT_STATUS_ADMIN_DOWN) {
        rtnl_link_unset_flags(lchange, IFF_UP);
        LOG(INFO) << __FUNCTION__ << ": " << rtnl_link_get_name(link)
                  << " was enabled";
      } else {
        rtnl_link_set_flags(lchange, IFF_UP);
        LOG(INFO) << __FUNCTION__ << ": " << rtnl_link_get_name(link)
                  << " was disabled";
      }
    }

    // apply changes
    if ((rv = rtnl_link_change(sock, link, lchange, 0)) < 0) {
      LOG(ERROR) << __FUNCTION__ << ": Unable to change link, "
                 << nl_geterror(rv);
    }
    rtnl_link_put(link);
    rtnl_link_put(lchange);
  }

  if (_pc_back.size()) {
    std::lock_guard<std::mutex> scoped_lock(pc_mutex);
    std::copy(make_move_iterator(_pc_back.begin()),
              make_move_iterator(_pc_back.end()),
              std::back_inserter(port_status_changes));
    do_wakeup = true;
  }

  if (do_wakeup || nl_objs.size()) {
    this->thread.wakeup();
  }
}

void cnetlink::handle_read_event(rofl::cthread &thread, int fd) {
  if (fd == nl_cache_mngr_get_fd(mngr)) {
    int rv = nl_cache_mngr_data_ready(mngr);
    VLOG(1) << "cnetlink #processed=" << rv;
    // notify update
    if (running) {
      this->thread.wakeup();
    }
  }
}

void cnetlink::handle_write_event(rofl::cthread &thread, int fd) {
  VLOG(1) << "cnetlink write ready on fd=" << fd;
  // currently not in use
}

void cnetlink::handle_timeout(rofl::cthread &thread, uint32_t timer_id) {
  switch (timer_id) {
  case NL_TIMER_RESEND_STATE:
// XXX loop through link cache
#if 0
    for (const auto &i : rtlinks.keys()) {
      const crtlink &link = rtlinks.get_link(i);
      link_created(link, get_port_id(link.get_ifindex()));
    }

    // XXX loop through neigh cache
    for (const auto &i : neighs_ll) {
      for (const auto &j : neighs_ll[i.first].keys()) {
        neigh_ll_created(i.first, neighs_ll[i.first].get_neigh(j));
      }
    }
#endif
    // was stopped before
    start();
    break;
  default:
    break;
  }
}

/* static C-callback */
void cnetlink::nl_cb_v2(struct nl_cache *cache, struct nl_object *old_obj,
                        struct nl_object *new_obj, uint64_t diff, int action,
                        void *data) {
  VLOG(1) << "diff=" << diff << " action=" << action << " old_obj=" << old_obj
          << " new_obj=" << new_obj;

  cnetlink::get_instance().nl_objs.emplace_back(action, old_obj, new_obj);
}

void cnetlink::route_link_apply(const nl_obj &obj) {
  int ifindex;
  if (obj.get_action() == NL_ACT_NEW) {
    ifindex = rtnl_link_get_ifindex(LINK_CAST(obj.get_new_obj()));
  } else {
    ifindex = rtnl_link_get_ifindex(LINK_CAST(obj.get_old_obj()));
  }

  auto s1 = ifindex_to_registered_port.find(ifindex);

  if (obj.get_action() == NL_ACT_NEW &&
      s1 == ifindex_to_registered_port.end()) {
    // try search using name
    char *portname = rtnl_link_get_name(LINK_CAST(obj.get_new_obj()));
    std::lock_guard<std::mutex> lock(rp_mutex);
    auto s2 = registered_ports.find(std::string(portname));
    if (s2 == registered_ports.end()) {
      // a non registered port appeared -> ignore
      LOG(INFO) << __FUNCTION__ << ": port " << portname
                << " ifindex=" << ifindex << " ignored";
      return;
    } else {
      auto r1 = ifindex_to_registered_port.insert(
          std::make_pair(ifindex, s2->second));
      if (r1.second) {
        s1 = r1.first;
      } else {
        LOG(FATAL) << __FUNCTION__
                   << ": insertion to ifindex_to_registered_port failed";
      }

      auto r2 = registered_port_to_ifindex.insert(
          std::make_pair(s2->second, ifindex));
      if (!r2.second) {
        LOG(FATAL) << __FUNCTION__
                   << ": insertion to registered_port_to_ifindex failed";
      }
      VLOG(1) << __FUNCTION__ << ": port " << portname
              << " registered, stored ifindex=" << ifindex;
    }
  }

  if (s1 == ifindex_to_registered_port.end()) {
    // ignore non registered link
    return;
  }

  try {
    switch (obj.get_action()) {
    case NL_ACT_NEW:
      assert(obj.get_new_obj());
      link_created(LINK_CAST(obj.get_new_obj()), s1->second);
      break;
    case NL_ACT_CHANGE:
      assert(obj.get_new_obj());
      assert(obj.get_old_obj());
      if (rtnl_link_get_family(LINK_CAST(obj.get_new_obj())) == AF_UNSPEC) {
        VLOG(1) << __FUNCTION__ << ": ignoring AF_UNSPEC change of "
                << rtnl_link_get_name(LINK_CAST(obj.get_old_obj()));
      } else {
        link_updated(LINK_CAST(obj.get_old_obj()), LINK_CAST(obj.get_new_obj()),
                     s1->second);
      }
      break;
    case NL_ACT_DEL: {
      link_deleted(LINK_CAST(obj.get_old_obj()), s1->second);
      uint32_t port_id = get_port_id(ifindex);
      ifindex_to_registered_port.erase(ifindex);
      registered_port_to_ifindex.erase(port_id);
    } break;
    default: { LOG(WARNING) << "route/link: unknown NL action"; }
    }

  } catch (std::exception &e) {
    LOG(FATAL) << "cnetlink::route_neigh_cb() oops unknown exception "
               << e.what();
  }
}

void cnetlink::route_neigh_apply(const nl_obj &obj) {

  try {
    switch (obj.get_action()) {
    case NL_ACT_NEW:
      assert(obj.get_new_obj());
      switch (rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()))) {
      case PF_BRIDGE:
        neigh_ll_created(NEIGH_CAST(obj.get_new_obj()));
        break;
      case AF_INET6:
      case AF_INET:
      default:
        break;
      }
      break;
    case NL_ACT_CHANGE: {
      assert(obj.get_new_obj());
      assert(obj.get_old_obj());
      switch (rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()))) {
      case PF_BRIDGE: {
        neigh_ll_updated(NEIGH_CAST(obj.get_old_obj()),
                         NEIGH_CAST(obj.get_new_obj()));
      } break;
      case AF_INET:
      case AF_INET6:
      default:
        break;
      }
    } break;
    case NL_ACT_DEL:
      assert(obj.get_old_obj());
      switch (rtnl_neigh_get_family(NEIGH_CAST(obj.get_old_obj()))) {
      case PF_BRIDGE:
        VLOG(1) << __FUNCTION__ << ": deleted neigh_ll"; // XXX print addr
        neigh_ll_deleted(NEIGH_CAST(obj.get_old_obj()));
        break;
      case AF_INET:
      case AF_INET6:
      default:
        break;
      }
      break;
    default: { LOG(WARNING) << "route/addr: unknown NL action"; }
    }
  } catch (std::exception &e) {
    LOG(FATAL) << "cnetlink::route_neigh_cb() oops unknown exception"
               << e.what();
  }
}

void cnetlink::link_created(rtnl_link *link, uint32_t port_id) noexcept {
  assert(link);
  try {
    if (AF_BRIDGE == rtnl_link_get_family(link)) {

      // check for new bridge slaves
      if (rtnl_link_get_master(link)) {
        // slave interface
        LOG(INFO) << __FUNCTION__ << ": " << rtnl_link_get_name(link)
                  << " is new slave interface";

        // use only the first bridge were an interface is attached to
        if (nullptr == bridge) {
          bridge = new ofdpa_bridge(this->swi);
          rtnl_link *br_link =
              rtnl_link_get(caches[NL_LINK_CACHE], rtnl_link_get_master(link));
          bridge->set_bridge_interface(br_link);
          rtnl_link_put(br_link);
        }

        bridge->add_interface(port_id, link);
      } else {
        // bridge (master)
        LOG(INFO) << __FUNCTION__ << ": is new bridge";
      }
    }
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " failed: " << e.what();
  }
}

void cnetlink::link_updated(rtnl_link *old_link, rtnl_link *new_link,
                            uint32_t port_id) noexcept {
  try {
    if (nullptr != bridge) {
      bridge->update_interface(port_id, old_link, new_link);
    }
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " failed: " << e.what();
  }
}

void cnetlink::link_deleted(rtnl_link *link, uint32_t port_id) noexcept {
  assert(link);

  try {
    if (bridge != nullptr && rtnl_link_get_family(link) == AF_BRIDGE) {
      bridge->delete_interface(port_id, link);
    }
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " failed: " << e.what();
  }
}

void cnetlink::neigh_ll_created(rtnl_neigh *neigh) noexcept {
  try {
    if (nullptr != bridge) {
      try {
        auto port =
            ifindex_to_registered_port.at(rtnl_neigh_get_ifindex(neigh));

        rtnl_link *l =
            rtnl_link_get(caches[NL_LINK_CACHE], rtnl_neigh_get_ifindex(neigh));
        assert(l);

        if (nl_addr_cmp(rtnl_link_get_addr(l), rtnl_neigh_get_lladdr(neigh))) {
          // mac of interface itself is ignored, others added
          try {
            bridge->add_mac_to_fdb(port, rtnl_neigh_get_vlan(neigh),
                                   rtnl_neigh_get_lladdr(neigh));
          } catch (std::exception &e) {
            LOG(ERROR)
                << __FUNCTION__
                << ": failed to add mac to fdb"; // TODO log mac, port,...?
          }
        }
        rtnl_link_put(l);
      } catch (std::out_of_range &e) {
        LOG(ERROR) << __FUNCTION__
                   << ": unknown link ifindex=" << rtnl_neigh_get_ifindex(neigh)
                   << " of new L2 neighbour: " << e.what();
      }
    }
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << "() failed: " << e.what();
  }
}

void cnetlink::neigh_ll_updated(rtnl_neigh *old_neigh,
                                rtnl_neigh *new_neigh) noexcept {
  LOG(WARNING) << __FUNCTION__ << ": neighbor update not supported";
}

void cnetlink::neigh_ll_deleted(rtnl_neigh *neigh) noexcept {
  auto i2r = ifindex_to_registered_port.find(rtnl_neigh_get_ifindex(neigh));
  if (i2r == ifindex_to_registered_port.end()) {
    VLOG(2) << __FUNCTION__ << ": neighbor delete on unregistered port ifindex="
            << rtnl_neigh_get_ifindex(neigh);
    return;
  }

  if (nullptr != bridge) {
    bridge->remove_mac_from_fdb(i2r->second, neigh);
  } else {
    LOG(INFO) << __FUNCTION__ << ": no bridge interface";
  }
}

void cnetlink::resend_state() noexcept {
  stop();
  thread.add_timer(NL_TIMER_RESEND_STATE, rofl::ctimespec().expire_in(0));
}

void cnetlink::register_switch(switch_interface *swi) noexcept {
  assert(swi);
  this->swi = swi;
}

void cnetlink::unregister_switch(switch_interface *swi) noexcept {
  // TODO we should remove the swi here
  stop();
}

void cnetlink::port_status_changed(uint32_t port_no,
                                   enum nbi::port_status ps) noexcept {
  try {
    auto pps = std::make_tuple(port_no, ps, 0);
    std::lock_guard<std::mutex> scoped_lock(pc_mutex);
    port_status_changes.push_back(pps);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": unknown exception " << e.what();
    return;
  }
  thread.wakeup();
}

} // namespace basebox
