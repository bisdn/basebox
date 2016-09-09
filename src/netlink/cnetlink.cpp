/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <fstream>
#include <glog/logging.h>
#include <iterator>
#include <cassert>

#include <sys/socket.h>
#include <linux/if.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>
#include <netlink/route/route.h>

#include "cnetlink.hpp"

#define LINK_CAST(obj) reinterpret_cast<struct rtnl_link *>(obj)
#define NEIGH_CAST(obj) reinterpret_cast<struct rtnl_neigh *>(obj)
#define ROUTE_CAST(obj) reinterpret_cast<struct rtnl_route *>(obj)
#define ADDR_CAST(obj) reinterpret_cast<struct rtnl_addr *>(obj)

namespace basebox {

cnetlink::cnetlink(switch_interface *swi)
    : swi(swi), thread(this), caches(NL_MAX_CACHE, nullptr), bridge(nullptr),
      nl_proc_max(10), running(false), rfd_scheduled(false), l3(swi) {

  memset(&params, 0, sizeof(struct nl_dump_params));
  params.dp_type = NL_DUMP_LINE;
  params.dp_buf = &dump_buf[0];
  params.dp_buflen = sizeof(dump_buf);

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
                                  (change_func_v2_t)&nl_cb_v2, NULL);
  if (0 != rc) {
    LOG(FATAL) << __FUNCTION__ << ": add route/link to cache mngr";
  }

  /* init route cache */
  rc = rtnl_route_alloc_cache(sock, AF_UNSPEC, 0, &caches[NL_ROUTE_CACHE]);
  if (rc < 0) {
    LOG(FATAL) << __FUNCTION__ << ": add route/route to cache mngr";
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_ROUTE_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, NULL);
  if (rc < 0) {
    LOG(FATAL) << __FUNCTION__ << ": add route/route to cache mngr";
  }

  /* init addr cache*/
  rc = rtnl_addr_alloc_cache(sock, &caches[NL_ADDR_CACHE]);
  if (rc < 0) {
    LOG(FATAL) << __FUNCTION__ << ": add route/addr to cache mngr";
  }
  rc = nl_cache_mngr_add_cache_v2(mngr, caches[NL_ADDR_CACHE],
                                  (change_func_v2_t)&nl_cb_v2, NULL);
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
                                  (change_func_v2_t)&nl_cb_v2, NULL);
  if (0 != rc) {
    LOG(FATAL) << __FUNCTION__ << ": add route/neigh to cache mngr";
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

struct rtnl_link *cnetlink::get_link_by_ifindex(int ifindex) const {
  return rtnl_link_get(caches[NL_LINK_CACHE], ifindex);
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
      LOG(INFO) << __FUNCTION__ << " unknown netlink object";
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

void cnetlink::route_addr_apply(const nl_obj &obj) {
  int family;

  switch (obj.get_action()) {
  case NL_ACT_NEW:
    assert(obj.get_new_obj());

    if (VLOG_IS_ON(2)) {
      nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": new addr " << std::string(dump_buf);
    }

    switch (family = rtnl_addr_get_family(ADDR_CAST(obj.get_new_obj()))) {
    case AF_INET:
      LOG(INFO) << __FUNCTION__ << " got new v4 addr";
      l3.add_l3_termination(ADDR_CAST(obj.get_new_obj()));
      break;
    case AF_INET6:
      LOG(INFO) << __FUNCTION__ << " new v6 not yet supported";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << " unsupported family: " << family;
      break;
    }
    break;

  case NL_ACT_CHANGE:
    assert(obj.get_new_obj());
    assert(obj.get_old_obj());

    if (VLOG_IS_ON(2)) {
      nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": change new addr "
                << std::string(dump_buf);
      nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": change old addr "
                << std::string(dump_buf);
    }

    switch (family = rtnl_addr_get_family(ADDR_CAST(obj.get_new_obj()))) {
    case AF_INET:
      LOG(WARNING) << __FUNCTION__ << " changed v4 not supported";
      break;
    case AF_INET6:
      LOG(INFO) << __FUNCTION__ << " changed v6 not supported";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << " unsupported family: " << family;
      break;
    }
    break;

  case NL_ACT_DEL:
    assert(obj.get_old_obj());

    if (VLOG_IS_ON(2)) {
      nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": del addr " << std::string(dump_buf);
    }

    switch (family = rtnl_addr_get_family(ADDR_CAST(obj.get_old_obj()))) {
    case AF_INET:
      l3.del_l3_termination(ADDR_CAST(obj.get_old_obj()));
      break;
    case AF_INET6:
      LOG(INFO) << __FUNCTION__ << " del v6 not yet supported";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << " unsupported family: " << family;
      break;
    }
    break;

  default:
    LOG(ERROR) << __FUNCTION__ << " invalid netlink action "
               << obj.get_action();
    break;
  }
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

      if (VLOG_IS_ON(2)) {
        nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": change new link "
                  << std::string(dump_buf);
      }

      link_created(LINK_CAST(obj.get_new_obj()), s1->second);
      break;

    case NL_ACT_CHANGE:
      assert(obj.get_new_obj());
      assert(obj.get_old_obj());

      if (VLOG_IS_ON(2)) {
        nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": change new link "
                  << std::string(dump_buf);
        nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": change old link "
                  << std::string(dump_buf);
      }

      if (rtnl_link_get_family(LINK_CAST(obj.get_new_obj())) == AF_UNSPEC) {
        VLOG(1) << __FUNCTION__ << ": ignoring AF_UNSPEC change of "
                << rtnl_link_get_name(LINK_CAST(obj.get_old_obj()));
      } else {
        link_updated(LINK_CAST(obj.get_old_obj()), LINK_CAST(obj.get_new_obj()),
                     s1->second);
      }
      break;

    case NL_ACT_DEL: {
      assert(obj.get_old_obj());

      if (VLOG_IS_ON(2)) {
        nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": del link " << std::string(dump_buf);
      }

      link_deleted(LINK_CAST(obj.get_old_obj()), s1->second);
      uint32_t port_id = get_port_id(ifindex);
      ifindex_to_registered_port.erase(ifindex);
      registered_port_to_ifindex.erase(port_id);

      break;
    }
    default:
      LOG(ERROR) << __FUNCTION__ << " invalid netlink action "
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

      if (VLOG_IS_ON(2)) {
        nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": new neigh " << std::string(dump_buf);
      }

      family = rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()));

      switch (family) {
      case PF_BRIDGE:
        neigh_ll_created(NEIGH_CAST(obj.get_new_obj()));
        break;
      case AF_INET:
        l3.add_l3_neigh(NEIGH_CAST(obj.get_new_obj()));
        break;
      case AF_INET6:
        LOG(INFO) << __FUNCTION__ << ": new neigh_v6 not supported yet "
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

      if (VLOG_IS_ON(2)) {
        nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": change new neigh "
                  << std::string(dump_buf);
        nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": change old neigh "
                  << std::string(dump_buf);
      }

      family = rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()));

      switch (rtnl_neigh_get_family(NEIGH_CAST(obj.get_new_obj()))) {
      case PF_BRIDGE:
        neigh_ll_updated(NEIGH_CAST(obj.get_old_obj()),
                         NEIGH_CAST(obj.get_new_obj()));
        break;
      case AF_INET6:
        LOG(INFO) << __FUNCTION__ << ": update neigh_v6 not supported";
        break;
      case AF_INET:
        l3.update_l3_neigh(NEIGH_CAST(obj.get_old_obj()),
                           NEIGH_CAST(obj.get_new_obj()));
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": invalid family " << family;
        break;
      }
      break;

    case NL_ACT_DEL:
      assert(obj.get_old_obj());

      if (VLOG_IS_ON(2)) {
        nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
        LOG(INFO) << __FUNCTION__ << ": del neigh " << std::string(dump_buf);
      }

      family = rtnl_neigh_get_family(NEIGH_CAST(obj.get_old_obj()));

      switch (rtnl_neigh_get_family(NEIGH_CAST(obj.get_old_obj()))) {
      case PF_BRIDGE:
        neigh_ll_deleted(NEIGH_CAST(obj.get_old_obj()));
        break;
      case AF_INET6:
        LOG(INFO) << __FUNCTION__ << ": delete neigh_v6 not supported "
                  << obj.get_old_obj();
        break;
      case AF_INET:
        l3.del_l3_neigh(NEIGH_CAST(obj.get_old_obj()));
        break;
      default:
        LOG(ERROR) << __FUNCTION__ << ": invalid family " << family;
        break;
      }
      break;
    default:
      LOG(ERROR) << __FUNCTION__ << " invalid netlink action "
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

    if (VLOG_IS_ON(2)) {
      nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": new route " << std::string(dump_buf);
    }

    switch (family = rtnl_route_get_family(ROUTE_CAST(obj.get_new_obj()))) {
    case AF_INET:
      LOG(INFO) << __FUNCTION__ << ": new v4 route not yet suppported";
      break;
    case AF_INET6:
      LOG(INFO) << __FUNCTION__ << ": new v6 route not yet suppported";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << " family not supported: " << family;
      break;
    }
    break;

  case NL_ACT_CHANGE:
    assert(obj.get_new_obj());
    assert(obj.get_old_obj());

    if (VLOG_IS_ON(2)) {
      nl_object_dump(OBJ_CAST(obj.get_new_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": change new route "
                << std::string(dump_buf);
      nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": change old route "
                << std::string(dump_buf);
    }

    switch (family = rtnl_route_get_family(ROUTE_CAST(obj.get_new_obj()))) {
    case AF_INET:
      LOG(INFO) << __FUNCTION__ << ": changed v4 route not yet suppported";
      break;
    case AF_INET6:
      LOG(INFO) << __FUNCTION__ << ": changed v6 route not yet suppported";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << " family not supported: " << family;
      break;
    }
    break;

  case NL_ACT_DEL:
    assert(obj.get_old_obj());

    if (VLOG_IS_ON(2)) {
      nl_object_dump(OBJ_CAST(obj.get_old_obj()), &params);
      LOG(INFO) << __FUNCTION__ << ": del route " << std::string(dump_buf);
    }

    switch (family = rtnl_route_get_family(ROUTE_CAST(obj.get_old_obj()))) {
    case AF_INET:
      LOG(INFO) << __FUNCTION__ << ": changed v4 route not yet suppported";
      break;
    case AF_INET6:
      LOG(INFO) << __FUNCTION__ << ": changed v6 route not yet suppported";
      break;
    default:
      LOG(WARNING) << __FUNCTION__ << " family not supported: " << family;
      break;
    }
    break;

  default:
    LOG(ERROR) << __FUNCTION__ << " invalid action " << obj.get_action();
    break;
  }

#if 0
  if (RT_TABLE_LOCAL == rtnl_route_get_table(route)) {
    VLOG(2) << __FUNCTION__ << " skip local route";
    return;
  }
  int nh_cnt = rtnl_route_get_nnexthops(route);
  LOG(INFO) << "having nh_cnt=" << nh_cnt;

  // dump ALL NHs
  rtnl_route_foreach_nexthop(route,
                             [](struct rtnl_nexthop *nh, void *pp) {
                               rtnl_route_nh_dump(nh,
                                                  (struct nl_dump_params *)pp);
                             },
                             &params);

  // XXX schedule for further checks
  if (nh_cnt) {
    struct rtnl_nexthop *nh = rtnl_route_nexthop_n(route, 0);
    assert(nh && "no NH");
    struct nl_addr *gw = rtnl_route_nh_get_gateway(nh);
    if (gw) {
      LOG(INFO) << "gw is " << gw;
      struct rtnl_neigh *neigh = rtnl_neigh_get(
          caches[NL_NEIGH_CACHE], rtnl_route_nh_get_ifindex(nh), gw);
      if (neigh) {
        LOG(INFO) << "having neigh";
      } else {
        LOG(INFO) << "no neigh";
      }
    }
  }
  nl_object_dump(OBJ_CAST(route), &params);
#endif
}

void cnetlink::set_neigh_timeout() {
  //  thread.add_timer(NL_TIMER_RESYNC, rofl::ctimespec().expire_in(5));
}

#if 0
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
#endif

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
  l3.register_switch_interface(swi);
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
