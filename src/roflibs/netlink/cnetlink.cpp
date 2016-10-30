/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <fstream>
#include <glog/logging.h>
#include <iterator>

#include "cnetlink.hpp"

namespace rofcore {

cnetlink::cnetlink(switch_interface *swi)
    : swi(swi), thread(this), bridge(nullptr), running(false) {

  sock = nl_socket_alloc();
  if (NULL == sock) {
    LOG(FATAL) << "cnetlink: failed to create netlink socket" << __FUNCTION__;
    throw eNetLinkCritical(__FUNCTION__);
  }

  try {
    init_caches();
    thread.start();
  } catch (...) {
    LOG(FATAL) << "cnetlink: caught unkown exception during " << __FUNCTION__;
  }
}

cnetlink::~cnetlink() {
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
  rc = nl_cache_mngr_add_cache(mngr, caches[NL_LINK_CACHE],
                               (change_func_t)&nl_cb, NULL);
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
  rc = nl_cache_mngr_add_cache(mngr, caches[NL_NEIGH_CACHE],
                               (change_func_t)&nl_cb, NULL);
  if (0 != rc) {
    LOG(FATAL) << "cnetlink::init_caches() add route/neigh to cache mngr";
  }

  struct nl_object *obj = nl_cache_get_first(caches[NL_LINK_CACHE]);
  while (0 != obj) {
    VLOG(1) << "cnetlink::" << __FUNCTION__ << "(): adding "
            << rtnl_link_get_name((struct rtnl_link *)obj) << " to rtlinks";
    rtlinks.add_link(crtlink((struct rtnl_link *)obj));
    obj = nl_cache_get_next(obj);
  }

  obj = nl_cache_get_first(caches[NL_NEIGH_CACHE]);
  while (0 != obj) {
    unsigned int ifindex = rtnl_neigh_get_ifindex((struct rtnl_neigh *)obj);
    switch (rtnl_neigh_get_family((struct rtnl_neigh *)obj)) {
    case AF_BRIDGE:
      if (rtlinks.has_link(ifindex)) {
        neighs_ll[ifindex].add_neigh(crtneigh((struct rtnl_neigh *)obj));
      }
      break;
    case AF_INET:
    case AF_INET6:
    default:
      break;
    }
    obj = nl_cache_get_next(obj);
  }

  thread.add_read_fd(nl_cache_mngr_get_fd(mngr), true, false);
}

void cnetlink::destroy_caches() {
  thread.drop_read_fd(nl_cache_mngr_get_fd(mngr), false);
  nl_cache_mngr_free(mngr);
}

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
  start();
}

void cnetlink::handle_wakeup(rofl::cthread &thread) {
  // loop through nl_objs
  for (int cnt = 0; cnt < 10 && nl_objs.size() && running;
       cnt++) { // TODO cnt_max as member
    auto obj = nl_objs.front();

    switch (nl_object_get_msgtype(obj.second.get_obj())) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
      route_link_apply(obj.first, obj.second);
      break;
    case RTM_NEWNEIGH:
    case RTM_DELNEIGH:
      route_neigh_apply(obj.first, obj.second);
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
      continue;
    }

    int flags = rtnl_link_get_flags(link);
    // check admin state change
    if (!((flags & IFF_UP) && !(std::get<1>(change) & nbi::PORT_STATUS_ADMIN_DOWN))) {
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
  }

  {
    std::lock_guard<std::mutex> scoped_lock(pc_mutex);
    std::copy(make_move_iterator(_pc_back.begin()),
              make_move_iterator(_pc_back.end()),
              std::back_inserter(port_status_changes));
    this->thread.wakeup();
  }

  if (nl_objs.size()) {
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

void cnetlink::handle_timeout(rofl::cthread &thread, uint32_t timer_id,
                              const std::list<unsigned int> &ttypes) {
  switch (timer_id) {
  case NL_TIMER_RESYNC: {
    int r = nl_cache_refill(sock, caches[NL_LINK_CACHE]);
    if (r < 0) {
      LOG(ERROR) << __FUNCTION__ << " failed to refill NL_LINK_CACHE";
      return;
    }
    r = nl_cache_refill(sock, caches[NL_NEIGH_CACHE]);
    if (r < 0) {
      LOG(ERROR) << __FUNCTION__ << " failed to refill NL_NEIGH_CACHE";
      return;
    }

    struct nl_object *obj = nl_cache_get_first(caches[NL_NEIGH_CACHE]);
    while (0 != obj) {
      unsigned int ifindex = rtnl_neigh_get_ifindex((struct rtnl_neigh *)obj);
      switch (rtnl_neigh_get_family((struct rtnl_neigh *)obj)) {
      case AF_INET:
      case AF_INET6:
        break;
      case AF_BRIDGE: {
        crtneigh neigh((struct rtnl_neigh *)obj);
        if (rtlinks.has_link(ifindex)) {
          // force an update
          neighs_ll[ifindex].add_neigh(neigh);
          neigh_ll_created(ifindex, neigh);
        } else {
          LOG(FATAL) << __FUNCTION__ << " no link ifindex=" << ifindex;
        }
      } break;
      default:
        break;
      }
      obj = nl_cache_get_next(obj);
    }

    thread.add_timer(NL_TIMER_RESYNC, rofl::ctimespec().expire_in(5));
  } break;
  case NL_TIMER_RESEND_STATE:
    for (const auto &i : rtlinks.keys()) {
      const crtlink &link = rtlinks.get_link(i);
      link_created(link, get_port_id(link.get_ifindex()));
    }

    for (const auto &i : neighs_ll) {
      for (const auto &j : neighs_ll[i.first].keys()) {
        neigh_ll_created(i.first, neighs_ll[i.first].get_neigh(j));
      }
    }
    // was stopped before
    start();
    break;
  default:
    break;
  }
}

/* static C-callback */
void cnetlink::nl_cb(struct nl_cache *cache, struct nl_object *obj, int action,
                     void *data) {
  assert(obj);

  nl_obj lobj(obj);
  cnetlink::get_instance().nl_objs.push_back(
      std::make_pair(action, std::move(lobj)));
}

void cnetlink::route_link_apply(int action, const nl_obj &obj) {
  int ifindex = rtnl_link_get_ifindex((struct rtnl_link *)obj.get_obj());

  auto s1 = ifindex_to_registered_port.find(ifindex);
  if (s1 == ifindex_to_registered_port.end()) {
    // try search using name
    char *portname = rtnl_link_get_name((struct rtnl_link *)obj.get_obj());
    std::lock_guard<std::mutex> lock(rp_mutex);
    auto s2 = registered_ports.find(std::string(portname));
    if (s2 == registered_ports.end()) {
      // not a registered port
      LOG(INFO) << __FUNCTION__ << ": port " << portname
                << " having ifindex=" << ifindex << " not registered";
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
              << " registered, storted ifindex=" << ifindex;
    }
  }

  crtlink rtlink((struct rtnl_link *)obj.get_obj());

  try {
    switch (action) {
    case NL_ACT_NEW: {
      switch (rtlink.get_family()) {
      case AF_BRIDGE:
        /* new bridge */
        set_links().add_link(rtlink); // overwrite old link
        LOG(INFO) << "link new (bridge "
                  << ((0 == rtlink.get_master()) ? "master" : "slave")
                  << "): " << get_links().get_link(ifindex).str();
        link_created(rtlink, s1->second);

        break;
      default:
        set_links().add_link(rtlink);
        LOG(INFO) << "link new: " << get_links().get_link(ifindex).str();
        link_created(rtlink, s1->second);
        break;
      }
    } break;
    case NL_ACT_CHANGE: {
      switch (rtlink.get_family()) {
      case AF_UNSPEC:
        VLOG(1) << "currently ignoring AF_UNSPEC changes. this change is wrt "
                << rtlink.get_devname();
        break;
      // fallthrough
      default:
        link_updated(rtlink, s1->second);
        set_links().set_link(rtlink);
        break;
      }
    } break;
    case NL_ACT_DEL: {
      // XXX check if this has to be handled like new
      link_deleted(rtlink, s1->second);
      LOG(INFO) << "link deleted: " << get_links().get_link(ifindex).str();
      set_links().drop_link(ifindex);
      uint32_t port_id = get_port_id(ifindex);
      ifindex_to_registered_port.erase(ifindex);
      registered_port_to_ifindex.erase(port_id);
    } break;
    default: { LOG(WARNING) << "route/link: unknown NL action"; }
    }

  } catch (eNetLinkNotFound &e) {
    // NL_ACT_CHANGE => ifindex not found
    LOG(ERROR) << "cnetlink::route_link_cb() oops, route_link_cb() caught "
                  "eNetLinkNotFound";
  } catch (crtlink::eRtLinkNotFound &e) {
    LOG(ERROR) << "cnetlink::route_link_cb() oops, route_link_cb() caught "
                  "eRtLinkNotFound";
  } catch (std::exception &e) {
    LOG(FATAL) << "cnetlink::route_neigh_cb() oops unknown exception"
               << e.what();
  }
}

void cnetlink::route_neigh_apply(int action, const nl_obj &obj) {

  struct rtnl_neigh *neigh = (struct rtnl_neigh *)obj.get_obj();

  int ifindex = rtnl_neigh_get_ifindex(neigh);
  int family = rtnl_neigh_get_family(neigh);

  if (0 == ifindex) {
    LOG(ERROR) << __FUNCTION__ << "() ignoring not existing link";
    return;
  }

  crtneigh n(neigh);

  try {
    switch (action) {
    case NL_ACT_NEW: {
      switch (family) {
      case PF_BRIDGE: {
        VLOG(1) << __FUNCTION__ << ": new neigh_ll" << std::endl << n;
        neighs_ll[ifindex].add_neigh(n);
        neigh_ll_created(ifindex, n);
      } break;
      case AF_INET6:
      case AF_INET:
      default:
        break;
      }
    } break;
    case NL_ACT_CHANGE: {
      switch (family) {
      case PF_BRIDGE: {
        VLOG(1) << __FUNCTION__ << ": updated neigh_ll" << std::endl << n;
        neighs_ll[ifindex].set_neigh(n);
        neigh_ll_updated(ifindex, n);
      } break;
      case AF_INET:
      case AF_INET6:
      default:
        break;
      }
    } break;
    case NL_ACT_DEL: {
      switch (family) {
      case PF_BRIDGE: {
        VLOG(1) << __FUNCTION__ << ": deleted neigh_ll" << std::endl << n;
        unsigned int nbindex = neighs_ll[ifindex].get_neigh(n);
        neigh_ll_deleted(ifindex, n);
        neighs_ll[ifindex].drop_neigh(nbindex);
      } break;
      case AF_INET:
      case AF_INET6:
      default:
        break;
      }
    } break;
    default: { LOG(WARNING) << "route/addr: unknown NL action"; }
    }
    VLOG(2) << __FUNCTION__ << ": status" << std::endl
            << cnetlink::get_instance();
  } catch (eNetLinkNotFound &e) {
    LOG(ERROR) << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was "
                  "called with an invalid link";
  } catch (crtneigh::eRtNeighNotFound &e) {
    LOG(ERROR) << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was "
                  "called with an invalid neighbor";
  } catch (std::exception &e) {
    LOG(FATAL) << "cnetlink::route_neigh_cb() oops unknown exception"
               << e.what();
  }

  set_neigh_timeout();
}

void cnetlink::set_neigh_timeout() {
  //  thread.add_timer(NL_TIMER_RESYNC, rofl::ctimespec().expire_in(5));
}

void cnetlink::add_neigh_ll(int ifindex, uint16_t vlan,
                            const rofl::caddress_ll &addr) {
  if (AF_BRIDGE != get_links().get_link(ifindex).get_family()) {
    throw eNetLinkNotFound("cnetlink::add_neigh_ll(): no bridge link");
  }

  struct rtnl_neigh *neigh = rtnl_neigh_alloc();

  rtnl_neigh_set_ifindex(neigh, ifindex);
  rtnl_neigh_set_family(neigh, PF_BRIDGE);
  rtnl_neigh_set_state(neigh, NUD_NOARP | NUD_REACHABLE);
  rtnl_neigh_set_flags(neigh, NTF_MASTER);
  rtnl_neigh_set_vlan(neigh, vlan);

  struct nl_addr *_addr = nl_addr_build(AF_LLC, addr.somem(), addr.memlen());
  rtnl_neigh_set_lladdr(neigh, _addr);
  nl_addr_put(_addr);

  struct nl_sock *sk = NULL;
  if ((sk = nl_socket_alloc()) == NULL) {
    rtnl_neigh_put(neigh);
    throw eNetLinkFailed("cnetlink::add_neigh_ll() nl_socket_alloc()");
  }

  int sd = 0;
  if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
    rtnl_neigh_put(neigh);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_neigh_ll() nl_connect()");
  }

  int rc;
  if ((rc = rtnl_neigh_add(sk, neigh, NLM_F_CREATE)) < 0) {
    rtnl_neigh_put(neigh);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_neigh_ll() rtnl_neigh_add()");
  }

  nl_close(sk);
  nl_socket_free(sk);
  rtnl_neigh_put(neigh);
}

void cnetlink::drop_neigh_ll(int ifindex, uint16_t vlan,
                             const rofl::caddress_ll &addr) {
  struct rtnl_neigh *neigh = rtnl_neigh_alloc();

  rtnl_neigh_set_ifindex(neigh, ifindex);
  rtnl_neigh_set_family(neigh, PF_BRIDGE);
  rtnl_neigh_set_state(neigh, NUD_NOARP | NUD_REACHABLE);
  rtnl_neigh_set_flags(neigh, NTF_MASTER);
  rtnl_neigh_set_vlan(neigh, vlan);

  struct nl_addr *_addr = nl_addr_build(AF_LLC, addr.somem(), addr.memlen());
  rtnl_neigh_set_lladdr(neigh, _addr);
  nl_addr_put(_addr);

  struct nl_sock *sk = NULL;
  if ((sk = nl_socket_alloc()) == NULL) {
    rtnl_neigh_put(neigh);
    throw eNetLinkFailed("cnetlink::drop_neigh_ll() nl_socket_alloc()");
  }

  int sd = 0;
  if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
    rtnl_neigh_put(neigh);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_neigh_ll() nl_connect()");
  }

  int rc;
  if ((rc = rtnl_neigh_delete(sk, neigh, 0)) < 0) {
    rtnl_neigh_put(neigh);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_neigh_ll() rtnl_neigh_delete()");
  }

  nl_close(sk);
  nl_socket_free(sk);
  rtnl_neigh_put(neigh);
}

void cnetlink::link_created(const crtlink &rtl, uint32_t port_id) noexcept {
  try {
    if (AF_BRIDGE == rtl.get_family()) {

      // check for new bridge slaves
      if (rtl.get_master()) {
        // slave interface
        LOG(INFO) << __FUNCTION__ << ": " << rtl.get_devname()
                  << " is new slave interface";

        // use only first bridge an of interface is attached to
        if (nullptr == bridge) {
          bridge = new ofdpa_bridge(this->swi);
          bridge->set_bridge_interface(get_links().get_link(rtl.get_master()));
        }

        bridge->add_interface(port_id, rtl);
      } else {
        // bridge (master)
        LOG(INFO) << __FUNCTION__ << ": is new bridge";
      }
    }
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " failed: " << e.what();
  }
}

void cnetlink::link_updated(const crtlink &newlink, uint32_t port_id) noexcept {
  try {
    const crtlink &oldlink = get_links().get_link(newlink.get_ifindex());
    VLOG(1) << __FUNCTION__ << ":" << std::endl
            << "=== oldlink ===" << std::endl
            << oldlink << "=== newlink ===" << std::endl
            << newlink;

    if (nullptr != bridge) {
      bridge->update_interface(port_id, oldlink, newlink);

      LOG(INFO) << __FUNCTION__ << "(): " << newlink.str();
    }
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " failed: " << e.what();
  }
}

void cnetlink::link_deleted(const crtlink &rtl, uint32_t port_id) noexcept {
  try {
    bridge->delete_interface(port_id, rtl);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " failed: " << e.what();
  }
}

void cnetlink::neigh_ll_created(unsigned int ifindex,
                                const crtneigh &rtn) noexcept {
  try {
    const crtlink &rtl = get_links().get_link(ifindex);

    if (nullptr != bridge) {
      try {
        // valid vlan id?
        if (0 > rtn.get_vlan() || 0x1000 < rtn.get_vlan()) {
          VLOG(1) << __FUNCTION__ << ": invalid vlan" << rtn.get_vlan();
          return;
        }

        // local mac address of parent?
        if (rtn.get_lladdr() == rtl.get_hwaddr()) {
          VLOG(1) << __FUNCTION__ << ": ignore master lladdr";
          return;
        } else {
          LOG(INFO) << __FUNCTION__ << ": rtn=" << rtn;
          VLOG(1) << __FUNCTION__ << ": parent=" << rtl.get_hwaddr();
        }

        auto port = ifindex_to_registered_port.at(rtl.get_ifindex());
        bridge->add_mac_to_fdb(port, rtn.get_vlan(), rtn.get_lladdr());
      } catch (std::out_of_range &e) {
        LOG(ERROR) << __FUNCTION__ << ": port " << rtl
                   << " not in ifindex_to_registered_port: " << e.what();
      }
    }
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << "() failed: " << e.what();
  }
}

void cnetlink::neigh_ll_updated(unsigned int ifindex,
                                const crtneigh &rtn) noexcept {
  try {
    LOG(WARNING) << __FUNCTION__ << "]: NOT handled neighbor:" << std::endl
                 << rtn;
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " failed: " << e.what();
  }
}

void cnetlink::neigh_ll_deleted(unsigned int ifindex,
                                const crtneigh &rtn) noexcept {
  try {
    auto i2r = ifindex_to_registered_port.find(ifindex);
    if (i2r == ifindex_to_registered_port.end()) {
      VLOG(2) << __FUNCTION__ << ": port with ifindex=" << ifindex;
    }

    const crtlink &rtl = get_links().get_link(ifindex);

    LOG(INFO) << __FUNCTION__ << ": " << rtn;

    if (nullptr != bridge) {
      try {
        if (rtn.get_lladdr() == rtl.get_hwaddr()) {
          LOG(INFO) << __FUNCTION__ << ": ignore master lladdr";
          return;
        }
        bridge->remove_mac_from_fdb(i2r->second, rtn.get_vlan(),
                                    rtn.get_lladdr());
      } catch (std::out_of_range &e) {
        LOG(ERROR) << __FUNCTION__ << ": port " << rtl
                   << " not in ifindex_to_registered_port: " << e.what();
      }
    } else {
      LOG(INFO) << __FUNCTION__ << ": no bridge interface";
    }
  } catch (crtlink::eRtLinkNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": eRtLinkNotFound: " << e.what();
  } catch (crtneigh::eRtNeighNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": eRtNeighNotFound" << e.what();
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

} // namespace rofcore
