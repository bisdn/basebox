/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <cassert>
#include <cstring>
#include <fstream>
#include <glog/logging.h>
#include <iterator>

#include <sys/socket.h>
#include <linux/if.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/link/vxlan.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/route.h>

#include "cnetlink.hpp"
#include "netlink-utils.hpp"
#include "nl_output.hpp"
#include "nl_route_query.hpp"
#include "tap_manager.hpp"

#include "nl_l3.hpp"
#include "nl_vlan.hpp"
#include "nl_vxlan.hpp"

namespace basebox {

cnetlink::cnetlink(std::shared_ptr<tap_manager> tap_man)
    : swi(nullptr), thread(this), caches(NL_MAX_CACHE, nullptr),
      tap_man(tap_man), bridge(nullptr), nl_proc_max(10), running(false),
      rfd_scheduled(false), vlan(new nl_vlan(tap_man, this)),
      l3(new nl_l3(tap_man, vlan, this)), vxlan(new nl_vxlan(tap_man, this)) {

  sock = nl_socket_alloc();
  if (NULL == sock) {
    LOG(FATAL) << "cnetlink: failed to create netlink socket" << __FUNCTION__;
    throw eNetLinkCritical(__FUNCTION__);
  }

  try {
    thread.start("netlink");
    init_caches();
  } catch (...) {
    LOG(FATAL) << "cnetlink: caught unknown exception during " << __FUNCTION__;
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
    LOG(FATAL) << __FUNCTION__ << ": failed to allocate netlink cache manager";
  }

  int rx_size = load_from_file("/proc/sys/net/core/rmem_max");
  int tx_size = load_from_file("/proc/sys/net/core/wmem_max");

  if (0 != nl_socket_set_buffer_size(sock, rx_size, tx_size)) {
    LOG(FATAL) << ": failed to resize socket buffers";
  }
  nl_socket_set_msg_buf_size(sock, rx_size);

  caches[NL_LINK_CACHE] = NULL;
  caches[NL_NEIGH_CACHE] = NULL;

  rc = rtnl_link_alloc_cache_flags(sock, AF_UNSPEC, &caches[NL_LINK_CACHE],
                                   NL_CACHE_AF_ITER);
  if (0 != rc) {
    LOG(FATAL) << __FUNCTION__
               << ": rtnl_link_alloc_cache_flags failed rc=" << rc;
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_LINK_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, this);
  if (0 != rc) {
    LOG(FATAL) << __FUNCTION__ << ": add route/link to cache mngr";
  }

  /* init route cache */
  rc = rtnl_route_alloc_cache(sock, AF_UNSPEC, 0, &caches[NL_ROUTE_CACHE]);
  if (rc < 0) {
    LOG(FATAL) << __FUNCTION__ << ": add route/route to cache mngr";
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_ROUTE_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, this);
  if (rc < 0) {
    LOG(FATAL) << __FUNCTION__ << ": add route/route to cache mngr";
  }

  /* init addr cache*/
  rc = rtnl_addr_alloc_cache(sock, &caches[NL_ADDR_CACHE]);
  if (rc < 0) {
    LOG(FATAL) << __FUNCTION__ << ": add route/addr to cache mngr";
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_ADDR_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, this);
  if (rc < 0) {
    LOG(FATAL) << __FUNCTION__ << ": add route/addr to cache mngr";
  }

  /* init neigh cache */
  rc = rtnl_neigh_alloc_cache_flags(sock, &caches[NL_NEIGH_CACHE],
                                    NL_CACHE_AF_ITER);
  if (0 != rc) {
    LOG(FATAL) << __FUNCTION__
               << ": rtnl_link_alloc_cache_flags failed rc=" << rc;
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_NEIGH_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, this);
  if (0 != rc) {
    LOG(FATAL) << __FUNCTION__ << ": add route/neigh to cache mngr";
  }

  thread.wakeup();
}

void cnetlink::destroy_caches() { nl_cache_mngr_free(mngr); }

struct rtnl_link *cnetlink::get_link_by_ifindex(int ifindex) const {
  return rtnl_link_get(caches[NL_LINK_CACHE], ifindex);
}

struct rtnl_link *cnetlink::get_link(int ifindex, int family) const {
  struct rtnl_link *_link;
  std::unique_ptr<rtnl_link, void (*)(rtnl_link *)> filter(rtnl_link_alloc(),
                                                           &rtnl_link_put);

  rtnl_link_set_ifindex(filter.get(), ifindex);
  rtnl_link_set_family(filter.get(), family);

  // search link by filter
  nl_cache_foreach_filter(caches[NL_LINK_CACHE], OBJ_CAST(filter.get()),
                          [](struct nl_object *obj, void *arg) {
                            *static_cast<nl_object **>(arg) = obj;
                          },
                          &_link);

  return _link;
}

struct rtnl_neigh *cnetlink::get_neighbour(int ifindex,
                                           struct nl_addr *a) const {
  assert(ifindex);
  assert(a);
  return rtnl_neigh_get(caches[NL_NEIGH_CACHE], ifindex, a);
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
      break;
    case RTM_NEWROUTE:
    case RTM_DELROUTE:
      route_route_apply(obj);
      break;
    case RTM_NEWADDR:
    case RTM_DELADDR:
      route_addr_apply(obj);
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": unexprected netlink type "
                 << obj.get_msg_type();
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
    ifindex = tap_man->get_ifindex(std::get<0>(change));
    if (0 == ifindex) {
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
                  << " disabling";
      } else {
        rtnl_link_set_flags(lchange, IFF_UP);
        LOG(INFO) << __FUNCTION__ << ": " << rtnl_link_get_name(link)
                  << " enabling";
      }
    } else {
      LOG(INFO) << __FUNCTION__ << ": notification of port "
                << rtnl_link_get_name(link) << " received. State unchanged.";
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
  VLOG(1) << "diff=" << diff << " action=" << action
          << " old_obj=" << static_cast<void *>(old_obj)
          << " new_obj=" << static_cast<void *>(new_obj);

  assert(data);
  static_cast<cnetlink *>(data)->nl_objs.emplace_back(action, old_obj, new_obj);
}

void cnetlink::route_addr_apply(const nl_obj &obj) {
  int family;

  switch (obj.get_action()) {
  case NL_ACT_NEW:
    assert(obj.get_new_obj());

    VLOG(2) << __FUNCTION__ << ": new addr " << obj.get_new_obj();

    switch (family = rtnl_addr_get_family(ADDR_CAST(obj.get_new_obj()))) {
    case AF_INET:
      l3->add_l3_termination(ADDR_CAST(obj.get_new_obj()));
      break;
    case AF_INET6:
      VLOG(2) << __FUNCTION__ << ": new IPv6 addr (not supported)";
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": unsupported family " << family;
      break;
    }
    break;

  case NL_ACT_CHANGE:
    assert(obj.get_new_obj());
    assert(obj.get_old_obj());

    VLOG(2) << __FUNCTION__ << ": change new addr " << obj.get_new_obj();
    VLOG(2) << __FUNCTION__ << ": change old addr " << obj.get_new_obj();

    switch (family = rtnl_addr_get_family(ADDR_CAST(obj.get_new_obj()))) {
    case AF_INET:
      LOG(WARNING) << __FUNCTION__ << ": changed IPv4 (not supported)";
      break;
    case AF_INET6:
      VLOG(2) << __FUNCTION__ << ": changed IPv6 not supported";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << ": unsupported family: " << family;
      break;
    }
    break;

  case NL_ACT_DEL:
    assert(obj.get_old_obj());

    VLOG(2) << __FUNCTION__ << ": del addr " << obj.get_old_obj();

    switch (family = rtnl_addr_get_family(ADDR_CAST(obj.get_old_obj()))) {
    case AF_INET:
      l3->del_l3_termination(ADDR_CAST(obj.get_old_obj()));
      break;
    case AF_INET6:
      VLOG(2) << __FUNCTION__ << ": deleted IPv6 (not supported)";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << ": unsupported family: " << family;
      break;
    }
    break;

  default:
    LOG(ERROR) << __FUNCTION__ << ": invalid netlink action "
               << obj.get_action();
    break;
  }
}

void cnetlink::route_link_apply(const nl_obj &obj) {
  try {
    switch (obj.get_action()) {
    case NL_ACT_NEW:
      assert(obj.get_new_obj());

      VLOG(2) << __FUNCTION__ << ": new link " << obj.get_new_obj();

      link_created(LINK_CAST(obj.get_new_obj()));
      break;

    case NL_ACT_CHANGE:
      assert(obj.get_new_obj());
      assert(obj.get_old_obj());

      VLOG(2) << __FUNCTION__ << ": change new link " << obj.get_new_obj();
      VLOG(2) << __FUNCTION__ << ": change old link " << obj.get_old_obj();

      link_updated(LINK_CAST(obj.get_old_obj()), LINK_CAST(obj.get_new_obj()));
      break;

    case NL_ACT_DEL: {
      assert(obj.get_old_obj());

      VLOG(2) << __FUNCTION__ << ": del link " << obj.get_old_obj();

      link_deleted(LINK_CAST(obj.get_old_obj()));
      break;
    }
    default:
      LOG(ERROR) << __FUNCTION__ << ": invalid netlink action "
                 << obj.get_action();
      break;
    }

  } catch (std::exception &e) { // XXX likely can be dropped now
    LOG(FATAL) << __FUNCTION__ << ": oops unknown exception " << e.what();
  }
}

void cnetlink::route_neigh_apply(const nl_obj &obj) {

  try {
    int family;
    switch (obj.get_action()) {
    case NL_ACT_NEW:
      assert(obj.get_new_obj());

      VLOG(2) << __FUNCTION__ << ": new neigh " << obj.get_new_obj();

      family = rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()));

      switch (family) {
      case PF_BRIDGE:
        neigh_ll_created(NEIGH_CAST(obj.get_new_obj()));
        break;
      case AF_INET:
        l3->add_l3_neigh(NEIGH_CAST(obj.get_new_obj()));
        break;
      case AF_INET6:
        VLOG(2) << __FUNCTION__ << ": new IPv6 neighbour (not supported) "
                << obj.get_new_obj();
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": invalid family " << family;
        break;
      }
      break;

    case NL_ACT_CHANGE:
      assert(obj.get_new_obj());
      assert(obj.get_old_obj());
      assert(rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj())) ==
             rtnl_neigh_get_family(NEIGH_CAST(obj.get_old_obj())));

      VLOG(2) << __FUNCTION__ << ": change new neigh " << obj.get_new_obj();
      VLOG(2) << __FUNCTION__ << ": change old neigh " << obj.get_old_obj();

      family = rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()));

      switch (rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()))) {
      case PF_BRIDGE:
        neigh_ll_updated(NEIGH_CAST(obj.get_old_obj()),
                         NEIGH_CAST(obj.get_new_obj()));
        break;
      case AF_INET6:
        LOG(INFO) << __FUNCTION__ << ": change IPv6 neighbour (not supported)";
        break;
      case AF_INET:
        l3->update_l3_neigh(NEIGH_CAST(obj.get_old_obj()),
                            NEIGH_CAST(obj.get_new_obj()));
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": invalid family " << family;
        break;
      }
      break;

    case NL_ACT_DEL:
      assert(obj.get_old_obj());

      VLOG(2) << __FUNCTION__ << ": del neigh " << obj.get_old_obj();

      family = rtnl_neigh_get_family(NEIGH_CAST(obj.get_old_obj()));

      switch (rtnl_neigh_get_family(NEIGH_CAST(obj.get_old_obj()))) {
      case PF_BRIDGE:
        neigh_ll_deleted(NEIGH_CAST(obj.get_old_obj()));
        break;
      case AF_INET6:
        VLOG(2) << __FUNCTION__ << ": delete IPv6 neighbour (not supported)";
        break;
      case AF_INET:
        l3->del_l3_neigh(NEIGH_CAST(obj.get_old_obj()));
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": invalid family " << family;
        break;
      }
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": invalid netlink action "
                 << obj.get_action();
    }
  } catch (std::exception &e) {
    LOG(FATAL) << __FUNCTION__ << ": oops unknown exception " << e.what();
  }
}

void cnetlink::route_route_apply(const nl_obj &obj) {
  int family;

  switch (obj.get_action()) {
  case NL_ACT_NEW:
    assert(obj.get_new_obj());

    VLOG(2) << __FUNCTION__ << ": new route " << obj.get_new_obj();

    switch (family = rtnl_route_get_family(ROUTE_CAST(obj.get_new_obj()))) {
    case AF_INET:
      VLOG(2) << __FUNCTION__ << ": new IPv4 route (not supported)";
      break;
    case AF_INET6:
      VLOG(2) << __FUNCTION__ << ": new IPv6 route (not supported)";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << ": family not supported: " << family;
      break;
    }
    break;

  case NL_ACT_CHANGE:
    assert(obj.get_new_obj());
    assert(obj.get_old_obj());

    VLOG(2) << __FUNCTION__ << ": change new route " << obj.get_new_obj();
    VLOG(2) << __FUNCTION__ << ": change old route " << obj.get_old_obj();

    switch (family = rtnl_route_get_family(ROUTE_CAST(obj.get_new_obj()))) {
    case AF_INET:
      VLOG(2) << __FUNCTION__ << ": changed IPv4 route (not supported)";
      break;
    case AF_INET6:
      VLOG(2) << __FUNCTION__ << ": changed IPv6 route (not supported)";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << ": family not supported: " << family;
      break;
    }
    break;

  case NL_ACT_DEL:
    assert(obj.get_old_obj());

    VLOG(2) << __FUNCTION__ << ": del route " << obj.get_old_obj();

    switch (family = rtnl_route_get_family(ROUTE_CAST(obj.get_old_obj()))) {
    case AF_INET:
      VLOG(2) << __FUNCTION__ << ": changed IPv4 route (not supported)";
      break;
    case AF_INET6:
      VLOG(2) << __FUNCTION__ << ": changed IPv6 route (not supported)";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << ": family not supported: " << family;
      break;
    }
    break;

  default:
    LOG(ERROR) << __FUNCTION__ << ": invalid action " << obj.get_action();
    break;
  }
}

void cnetlink::link_created(rtnl_link *link) noexcept {
  assert(link);
  assert(tap_man);

  enum link_type lt = kind_to_link_type(rtnl_link_get_type(link));
  int af = rtnl_link_get_family(link);

  switch (lt) {
  case LT_UNKNOWN:
    switch (af) {
    case AF_BRIDGE: { // a new bridge slave was created

      try {
        // check for new bridge slaves
        if (rtnl_link_get_master(link) == 0) {
          LOG(ERROR) << __FUNCTION__ << ": unknown link " << OBJ_CAST(link);
          return;
        }

        // slave interface
        LOG(INFO) << __FUNCTION__ << ": " << rtnl_link_get_name(link)
                  << " is new slave interface";

        // use only the first bridge an interface is attached to
        // XXX TODO more bridges!
        if (nullptr == bridge) {
          LOG(INFO) << __FUNCTION__ << ": using bridge "
                    << rtnl_link_get_master(link);
          bridge = new nl_bridge(this->swi, tap_man, this, vxlan);
          rtnl_link *br_link =
              rtnl_link_get(caches[NL_LINK_CACHE], rtnl_link_get_master(link));
          bridge->set_bridge_interface(br_link);
          rtnl_link_put(br_link);
        }

        bridge->add_interface(link);
      } catch (std::exception &e) {
        LOG(ERROR) << __FUNCTION__ << ": failed: " << e.what();
      }
    } break;
    default:
      LOG(WARNING) << __FUNCTION__ << ": ignoring link with af=" << af
                   << " link:" << link;
      break;
    } // LT_UNKNOWN, switch family
    break;
  case LT_VXLAN: {
    vxlan->create_endpoint_port(link);
  } break;
  case LT_TUN: {
    int ifindex = rtnl_link_get_ifindex(link);
    std::string name(rtnl_link_get_name(link));
    tap_man->tap_dev_ready(ifindex, name);
  } break;
  default:
    LOG(WARNING) << __FUNCTION__ << ": ignoring link with lt=" << lt
                 << " link:" << link;
    break;
  } // switch link type
}

void cnetlink::link_updated(rtnl_link *old_link, rtnl_link *new_link) noexcept {

  link_type lt_old = kind_to_link_type(rtnl_link_get_type(old_link));
  link_type lt_new = kind_to_link_type(rtnl_link_get_type(new_link));

  int af_old = rtnl_link_get_family(old_link);
  int af_new = rtnl_link_get_family(new_link);

  LOG(INFO) << __FUNCTION__ << ": new family=" << af_new << ", lt=" << lt_new
            << ", name=" << rtnl_link_get_name(new_link);
  LOG(INFO) << __FUNCTION__ << ": old family=" << af_old << ", lt=" << lt_old
            << ", name=" << rtnl_link_get_name(old_link);

  if (lt_old != lt_new) {
    VLOG(1) << __FUNCTION__ << ": link type changed from " << lt_old << " to "
            << lt_new;
    // this is currently handled elsewhere
    return;
  }

  if (af_old != af_new) {
    VLOG(1) << __FUNCTION__ << ": af changed from " << af_old << " to "
            << af_new;
    // this is currently handled elsewhere
    return;
  }

  // AF_UNSPEC changes are not yet supported for links
  if (rtnl_link_get_family(new_link) == AF_UNSPEC) {
    // XXX TODO handle ifup/ifdown...
    return;
  }

  switch (lt_old) {
  case LT_UNKNOWN:
    switch (af_old) {

    case AF_BRIDGE: { // a bridge slave was changed
      if (bridge != nullptr) {
        try {
          bridge->update_interface(old_link, new_link);
        } catch (std::exception &e) {
          LOG(ERROR) << __FUNCTION__ << ": failed: " << e.what();
        }
      }

    } break;
    default:
      LOG(ERROR) << __FUNCTION__ << ": AF not handled " << af_old;
      break;
    }
    break;
  default:
    LOG(ERROR) << __FUNCTION__ << ": link type not handled " << lt_old;
    break;
  }
}

void cnetlink::link_deleted(rtnl_link *link) noexcept {
  assert(link);
  enum link_type lt = kind_to_link_type(rtnl_link_get_type(link));
  int af = rtnl_link_get_family(link);

  LOG(INFO) << __FUNCTION__ << ": family=" << af << ", lt=" << lt
            << ", kind=" << rtnl_link_get_type(link);

  switch (lt) {
  case LT_UNKNOWN:
    switch (af) {
    case AF_BRIDGE:
      try {
        if (bridge != nullptr) {
          bridge->delete_interface(link);
        }
      } catch (std::exception &e) {
        LOG(ERROR) << __FUNCTION__ << ": failed: " << e.what();
      }
      break;
    default:
      break;
    }
    break;
  case LT_TUN:
    tap_man->tap_dev_removed(rtnl_link_get_ifindex(link));
    break;
  default:;
  }
}

void cnetlink::neigh_ll_created(rtnl_neigh *neigh) noexcept {
  if (nullptr == bridge) {
    LOG(ERROR) << __FUNCTION__ << ": bridge not set";
    return;
  }

  int ifindex = rtnl_neigh_get_ifindex(neigh);

  if (ifindex == 0) {
    VLOG(1) << __FUNCTION__ << ": no ifindex for neighbour " << neigh;
    return;
  }

  rtnl_link *l = get_link(ifindex, AF_UNSPEC);
  if (l == nullptr) {
    LOG(ERROR) << __FUNCTION__
               << ": unknown link ifindex=" << rtnl_neigh_get_ifindex(neigh)
               << " of new L2 neighbour";
    return;
  }

  if (vxlan->add_l2_neigh(neigh, l) == 0) {
    // vxlan domain
  } else {
    // XXX TODO maybe we have to do this as well wrt. local bridging
    // normal bridging
    if (nl_addr_cmp(rtnl_link_get_addr(l), rtnl_neigh_get_lladdr(neigh))) {
      // mac of interface itself is ignored, others added
      try {
        bridge->add_neigh_to_fdb(neigh);
      } catch (std::exception &e) {
        LOG(ERROR) << __FUNCTION__
                   << ": failed to add mac to fdb"; // TODO log mac, port,...?
      }
    } else {
      VLOG(2) << __FUNCTION__ << ": bridge port mac address is ignored";
    }
  }
}

void cnetlink::neigh_ll_updated(rtnl_neigh *old_neigh,
                                rtnl_neigh *new_neigh) noexcept {
  LOG(WARNING) << __FUNCTION__ << ": neighbor update not supported";
}

void cnetlink::neigh_ll_deleted(rtnl_neigh *neigh) noexcept {
  if (nullptr != bridge) {
    bridge->remove_mac_from_fdb(neigh);
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
  l3->register_switch_interface(swi);
  vlan->register_switch_interface(swi);
  vxlan->register_switch_interface(swi);
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
