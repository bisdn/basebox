/*
 * clinkcache.cc
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#include "cnetlink.hpp"

using namespace rofcore;

cnetlink::cnetlink() : thread(this), mngr(0), check_links(false) {
  try {
    init_caches();
    thread.start();
  } catch (...) {
    logging::error << "cnetlink: caught unkown exception during "
                   << __FUNCTION__ << std::endl;
  }
}

cnetlink::~cnetlink() { destroy_caches(); }

void cnetlink::init_caches() {
  int rc = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);
  if (rc < 0) {
    logging::crit
        << "cnetlink::init_caches() failed to allocate netlink cache manager"
        << std::endl;
    throw eNetLinkCritical("cnetlink::init_caches()");
  }

  caches[NL_LINK_CACHE] = NULL;
  caches[NL_ADDR_CACHE] = NULL;
  caches[NL_ROUTE_CACHE] = NULL;
  caches[NL_NEIGH_CACHE] = NULL;

  rc = nl_cache_mngr_add(mngr, "route/link", (change_func_t)&route_link_cb,
                         NULL, &caches[NL_LINK_CACHE]);
  if (0 != rc) {
    logging::error << "cnetlink::init_caches() add route/link to cache mngr"
                   << std::endl;
  }
  rc = nl_cache_mngr_add(mngr, "route/addr", (change_func_t)&route_addr_cb,
                         NULL, &caches[NL_ADDR_CACHE]);
  if (0 != rc) {
    logging::error << "cnetlink::init_caches() add route/addr to cache mngr"
                   << std::endl;
  }
  rc = nl_cache_mngr_add(mngr, "route/route", (change_func_t)&route_route_cb,
                         NULL, &caches[NL_ROUTE_CACHE]);
  if (0 != rc) {
    logging::error << "cnetlink::init_caches() add route/route to cache mngr"
                   << std::endl;
  }
  rc = nl_cache_mngr_add(mngr, "route/neigh", (change_func_t)&route_neigh_cb,
                         NULL, &caches[NL_NEIGH_CACHE]);
  if (0 != rc) {
    logging::error << "cnetlink::init_caches() add route/neigh to cache mngr"
                   << std::endl;
  }

  struct nl_object *obj = nl_cache_get_first(caches[NL_LINK_CACHE]);
  while (0 != obj) {
    nl_object_get(obj);
    logging::debug << "cnetlink::" << __FUNCTION__ << "(): adding "
                   << rtnl_link_get_name((struct rtnl_link *)obj)
                   << " to rtlinks" << std::endl;
    rtlinks.add_link(crtlink((struct rtnl_link *)obj));
    nl_object_put(obj);
    obj = nl_cache_get_next(obj);
  }

  obj = nl_cache_get_first(caches[NL_ADDR_CACHE]);
  while (0 != obj) {
    nl_object_get(obj);
    unsigned int ifindex = rtnl_addr_get_ifindex((struct rtnl_addr *)obj);
    switch (rtnl_addr_get_family((struct rtnl_addr *)obj)) {
    case AF_INET:
      addrs_in4[ifindex].add_addr(crtaddr_in4((struct rtnl_addr *)obj));
      break;
    case AF_INET6:
      addrs_in6[ifindex].add_addr(crtaddr_in6((struct rtnl_addr *)obj));
      break;
    }
    nl_object_put(obj);
    obj = nl_cache_get_next(obj);
  }

  obj = nl_cache_get_first(caches[NL_ROUTE_CACHE]);
  while (0 != obj) {
    nl_object_get(obj);
    int table_id = rtnl_route_get_table((struct rtnl_route *)obj);
    switch (rtnl_route_get_family((struct rtnl_route *)obj)) {
    case AF_INET:
      rtroutes_in4[table_id].add_route(crtroute_in4((struct rtnl_route *)obj));
      break;
    case AF_INET6:
      rtroutes_in6[table_id].add_route(crtroute_in6((struct rtnl_route *)obj));
      break;
    }
    nl_object_put(obj);
    obj = nl_cache_get_next(obj);
  }

  obj = nl_cache_get_first(caches[NL_NEIGH_CACHE]);
  while (0 != obj) {
    nl_object_get(obj);
    // not handled at all? was #if 0
    unsigned int ifindex = rtnl_neigh_get_ifindex((struct rtnl_neigh *)obj);
    switch (rtnl_neigh_get_family((struct rtnl_neigh *)obj)) {
    case AF_INET:
      // set_neigh_in4(crtneigh_in4((struct rtnl_neigh*)obj));
      if (rtlinks.has_link(ifindex)) {
        neighs_in4[ifindex].add_neigh(crtneigh_in4((struct rtnl_neigh *)obj));
      }
      break;
    case AF_INET6:
      if (rtlinks.has_link(ifindex)) {
        neighs_in6[ifindex].add_neigh(crtneigh_in6((struct rtnl_neigh *)obj));
      }
      break;
    case AF_BRIDGE:
      if (rtlinks.has_link(ifindex)) {
        neighs_ll[ifindex].add_neigh(crtneigh((struct rtnl_neigh *)obj));
      }
      break;
    }
    nl_object_put(obj);
    obj = nl_cache_get_next(obj);
  }

  thread.add_read_fd(nl_cache_mngr_get_fd(mngr));
}

void cnetlink::destroy_caches() {
  thread.drop_read_fd(nl_cache_mngr_get_fd(mngr));

  nl_cache_mngr_free(mngr);
}

cnetlink &cnetlink::get_instance() {
  static cnetlink instance;
  return instance;
}

void cnetlink::update_link_cache() {
  logging::info << "[cnetlink][" << __FUNCTION__
                << "] #links=" << get_links().size()
                << " #cacheitems=" << nl_cache_nitems(caches[NL_LINK_CACHE])
                << std::endl;

  struct nl_object *obj;

#ifdef DEBUG
  logging::debug << "existing links in cnetlink:" << std::endl;
  const std::map<unsigned int, crtlink> &links = rtlinks.get_all_links();
  std::for_each(links.cbegin(), links.cend(),
                [](const std::pair<unsigned int, crtlink> &n) {
                  logging::debug << n.second.get_devname() << std::endl;
                });

  logging::debug << "existing links in nl_cache:" << std::endl;
  obj = nl_cache_get_first(caches[NL_LINK_CACHE]);
  while (0 != obj) {
    logging::debug << rtnl_link_get_name((struct rtnl_link *)obj) << std::endl;
    obj = nl_cache_get_next(obj);
  }

#endif

  check_links = false;
  struct nl_sock *sk = NULL;
  if ((sk = nl_socket_alloc()) == NULL) {
    return;
  }

  int sd = 0;
  if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
    nl_socket_free(sk);
    return;
  }

  int rv = nl_cache_refill(sk, caches[NL_LINK_CACHE]);

  nl_close(sk);
  nl_socket_free(sk);
  if (rv != 0) {
    logging::error << "[cnetlink][" << __FUNCTION__
                   << "] nl_cache_refill failed" << std::endl;
    return;
  }

  obj = nl_cache_get_first(caches[NL_LINK_CACHE]);
  while (0 != obj) {
    nl_object_get(obj);
    int i = rtnl_link_get_ifindex((struct rtnl_link *)obj);
    if (missing_links.find(i) != missing_links.end()) {
      rtlinks.add_link(crtlink((struct rtnl_link *)obj));
      missing_links.erase(i);
    }
    nl_object_put(obj);
    obj = nl_cache_get_next(obj);
  }

  if (missing_links.size()) {
    // reschedule
    check_links = true;
    //		rofl::ciosrv::notify(rofl::cevent(EVENT_UPDATE_LINKS));
    logging::info << __FUNCTION__ << ": still some links missing" << std::endl;
  }
}

void cnetlink::handle_read_event(rofl::cthread &thread, int fd) {
  if (fd == nl_cache_mngr_get_fd(mngr)) {
    int rv = nl_cache_mngr_data_ready(mngr);
    logging::debug << "cnetlink #processed=" << rv << std::endl;
  }

  // reregister fd
  // register_filedesc_r(nl_cache_mngr_get_fd(mngr));
}

// /* virtual */void
// cnetlink::handle_event(const rofl::cevent& ev)
//{
//	switch (ev.get_cmd()) {
//	case EVENT_UPDATE_LINKS:
//		update_link_cache();
//		break;
//	case EVENT_NONE:
//	default:
//		break;
//	}
//}

void cnetlink::update_link_cache(unsigned int ifindex) {
  logging::notice << __FUNCTION__ << "(): missing ifindex=" << ifindex
                  << std::endl;
  missing_links.insert(ifindex);

  if (not check_links) {
    check_links = true;
    // rofl::ciosrv::notify(rofl::cevent(EVENT_UPDATE_LINKS));
  }
}

/* static C-callback */
void cnetlink::route_link_cb(struct nl_cache *cache, struct nl_object *obj,
                             int action, void *data) {
  nl_object_get(obj); // increment reference counter by one
  if (std::string(nl_object_get_type(obj)) != std::string("route/link")) {
    logging::warn
        << "cnetlink::route_link_cb() ignoring non link object received"
        << std::endl;
    return;
  }

  unsigned int ifindex = rtnl_link_get_ifindex((struct rtnl_link *)obj);
  crtlink rtlink((struct rtnl_link *)obj);

  nl_object_put(obj); // decrement reference counter by one

  try {
    switch (action) {
    case NL_ACT_NEW: {
      switch (rtlink.get_family()) {
      case AF_BRIDGE:
        /* new bridge */
        cnetlink::get_instance().set_links().add_link(
            rtlink); // overwrite old link
        logging::notice
            << "link new (bridge "
            << ((0 == rtlink.get_master()) ? "master" : "slave") << "): "
            << cnetlink::get_instance().get_links().get_link(ifindex).str()
            << std::endl;
        cnetlink::get_instance().notify_link_created(ifindex);

        break;
      default:
        cnetlink::get_instance().set_links().add_link(rtlink);
        logging::notice
            << "link new: "
            << cnetlink::get_instance().get_links().get_link(ifindex).str()
            << std::endl;
        cnetlink::get_instance().notify_link_created(ifindex);
        break;
      }
    } break;
    case NL_ACT_CHANGE: {
      cnetlink::get_instance().notify_link_updated(rtlink);
      cnetlink::get_instance().set_links().set_link(rtlink);
      logging::debug
          << cnetlink::get_instance().get_links().get_link(ifindex).str()
          << std::endl;
    } break;
    case NL_ACT_DEL: {
      // xxx check if this has to be handled like new
      cnetlink::get_instance().notify_link_deleted(ifindex);
      logging::notice
          << "link deleted: "
          << cnetlink::get_instance().get_links().get_link(ifindex).str()
          << std::endl;
      cnetlink::get_instance().set_links().drop_link(ifindex);
    } break;
    default: { logging::warn << "route/link: unknown NL action" << std::endl; }
    }

  } catch (eNetLinkNotFound &e) {
    // NL_ACT_CHANGE => ifindex not found
    logging::error << "cnetlink::route_link_cb() oops, route_link_cb() caught "
                      "eNetLinkNotFound"
                   << std::endl;
  } catch (crtlink::eRtLinkNotFound &e) {
    logging::error << "cnetlink::route_link_cb() oops, route_link_cb() caught "
                      "eRtLinkNotFound"
                   << std::endl;
  }
}

/* static C-callback */
void cnetlink::route_addr_cb(struct nl_cache *cache, struct nl_object *obj,
                             int action, void *data) {
  if (std::string(nl_object_get_type(obj)) != std::string("route/addr")) {
    logging::debug
        << "cnetlink::route_addr_cb() ignoring non addr object received"
        << std::endl;
    return;
  }

  nl_object_get(obj); // get reference to object

  int ifindex = rtnl_addr_get_ifindex((struct rtnl_addr *)obj);
  int family = rtnl_addr_get_family((struct rtnl_addr *)obj);

  try {
    switch (action) {
    case NL_ACT_NEW: {
      switch (family) {
      case AF_INET: {
        logging::debug << "[roflibs][cnetlink][route_addr_cb] new addr_in4"
                       << std::endl
                       << crtaddr_in4((struct rtnl_addr *)obj);
        unsigned int adindex =
            cnetlink::get_instance().addrs_in4[ifindex].add_addr(
                crtaddr_in4((struct rtnl_addr *)obj));
        cnetlink::get_instance().notify_addr_in4_created(ifindex, adindex);
      } break;
      case AF_INET6: {
        logging::debug << "[roflibs][cnetlink][route_addr_cb] new addr_in6"
                       << std::endl
                       << crtaddr_in6((struct rtnl_addr *)obj);
        unsigned int adindex =
            cnetlink::get_instance().addrs_in6[ifindex].add_addr(
                crtaddr_in6((struct rtnl_addr *)obj));
        cnetlink::get_instance().notify_addr_in6_created(ifindex, adindex);
      } break;
      }

    } break;
    case NL_ACT_CHANGE: {
      switch (family) {
      case AF_INET: {
        logging::debug << "[roflibs][cnetlink][route_addr_cb] updated addr_in4"
                       << std::endl
                       << crtaddr_in4((struct rtnl_addr *)obj);
        unsigned int adindex =
            cnetlink::get_instance().addrs_in4[ifindex].set_addr(
                crtaddr_in4((struct rtnl_addr *)obj));
        cnetlink::get_instance().notify_addr_in4_updated(ifindex, adindex);
      } break;
      case AF_INET6: {
        logging::debug << "[roflibs][cnetlink][route_addr_cb] updated addr_in6"
                       << std::endl
                       << crtaddr_in6((struct rtnl_addr *)obj);
        unsigned int adindex =
            cnetlink::get_instance().addrs_in6[ifindex].set_addr(
                crtaddr_in6((struct rtnl_addr *)obj));
        cnetlink::get_instance().notify_addr_in6_updated(ifindex, adindex);
      } break;
      }

    } break;
    case NL_ACT_DEL: {
      switch (family) {
      case AF_INET: {
        logging::debug << "[roflibs][cnetlink][route_addr_cb] deleted addr_in4"
                       << std::endl
                       << crtaddr_in4((struct rtnl_addr *)obj);
        unsigned int adindex =
            cnetlink::get_instance().addrs_in4[ifindex].get_addr(
                crtaddr_in4((struct rtnl_addr *)obj));
        cnetlink::get_instance().notify_addr_in4_deleted(ifindex, adindex);
        cnetlink::get_instance().addrs_in4[ifindex].drop_addr(adindex);
      } break;
      case AF_INET6: {
        logging::debug << "[roflibs][cnetlink][route_addr_cb] deleted addr_in6"
                       << std::endl
                       << crtaddr_in6((struct rtnl_addr *)obj);
        unsigned int adindex =
            cnetlink::get_instance().addrs_in6[ifindex].get_addr(
                crtaddr_in6((struct rtnl_addr *)obj));
        cnetlink::get_instance().notify_addr_in6_deleted(ifindex, adindex);
        cnetlink::get_instance().addrs_in6[ifindex].drop_addr(adindex);
      } break;
      }

    } break;
    default: { logging::warn << "route/addr: unknown NL action" << std::endl; }
    }
    logging::trace << "[roflibs][cnetlink][route_addr_cb] status" << std::endl
                   << cnetlink::get_instance();
  } catch (eNetLinkNotFound &e) {
    logging::error << "cnetlink::route_addr_cb() oops, route_addr_cb() was "
                      "called with an invalid link [1]"
                   << std::endl;
  } catch (crtaddr::eRtAddrNotFound &e) {
    logging::error << "cnetlink::route_addr_cb() oops, route_addr_cb() was "
                      "called with an invalid address [2]"
                   << std::endl;
  }

  nl_object_put(obj); // release reference to object
}

/* static C-callback */
void cnetlink::route_route_cb(struct nl_cache *cache, struct nl_object *obj,
                              int action, void *data) {
  if (std::string(nl_object_get_type(obj)) != std::string("route/route")) {
    logging::debug
        << "cnetlink::route_route_cb() ignoring non route object received"
        << std::endl;
    return;
  }

  nl_object_get(obj); // get reference to object

  // logging::debug << "cnetlink::route_route_cb() called" << std::endl;

  int family = rtnl_route_get_family((struct rtnl_route *)obj);
  int table_id = rtnl_route_get_table((struct rtnl_route *)obj);

  try {
    switch (action) {
    case NL_ACT_NEW: {
      switch (family) {
      case AF_INET: {
        logging::debug << "[roflibs][cnetlink][route_route_cb] new route_in4"
                       << std::endl
                       << crtroute_in4((struct rtnl_route *)obj);
        unsigned int rtindex =
            cnetlink::get_instance().set_routes_in4(table_id).add_route(
                crtroute_in4((struct rtnl_route *)obj));
        logging::debug
            << cnetlink::get_instance().get_routes_in4(table_id).str()
            << std::endl;
        cnetlink::get_instance().notify_route_in4_created(table_id, rtindex);
      } break;
      case AF_INET6: {
        logging::debug << "[roflibs][cnetlink][route_route_cb] new route_in6"
                       << std::endl
                       << crtroute_in6((struct rtnl_route *)obj);
        unsigned int rtindex =
            cnetlink::get_instance().set_routes_in6(table_id).add_route(
                crtroute_in6((struct rtnl_route *)obj));
        logging::debug
            << cnetlink::get_instance().get_routes_in6(table_id).str()
            << std::endl;
        cnetlink::get_instance().notify_route_in6_created(table_id, rtindex);
      } break;
      }
    } break;
    case NL_ACT_CHANGE: {
      switch (family) {
      case AF_INET: {
        logging::debug
            << "[roflibs][cnetlink][route_route_cb] updated route_in4"
            << std::endl
            << crtroute_in4((struct rtnl_route *)obj);
        unsigned int rtindex =
            cnetlink::get_instance().set_routes_in4(table_id).set_route(
                crtroute_in4((struct rtnl_route *)obj));
        logging::debug
            << cnetlink::get_instance().get_routes_in4(table_id).str()
            << std::endl;
        cnetlink::get_instance().notify_route_in4_updated(table_id, rtindex);
      } break;
      case AF_INET6: {
        logging::debug
            << "[roflibs][cnetlink][route_route_cb] updated route_in6"
            << std::endl
            << crtroute_in6((struct rtnl_route *)obj);
        unsigned int rtindex =
            cnetlink::get_instance().set_routes_in6(table_id).set_route(
                crtroute_in6((struct rtnl_route *)obj));
        logging::debug
            << cnetlink::get_instance().get_routes_in6(table_id).str()
            << std::endl;
        cnetlink::get_instance().notify_route_in6_updated(table_id, rtindex);
      } break;
      }
    } break;
    case NL_ACT_DEL: {
      switch (family) {
      case AF_INET: {
        logging::debug
            << "[roflibs][cnetlink][route_route_cb] deleted route_in4"
            << std::endl
            << crtroute_in4((struct rtnl_route *)obj);
        unsigned int rtindex =
            cnetlink::get_instance().get_routes_in4(table_id).get_route(
                crtroute_in4((struct rtnl_route *)obj));
        cnetlink::get_instance().notify_route_in4_deleted(table_id, rtindex);
        cnetlink::get_instance().set_routes_in4(table_id).drop_route(rtindex);
        logging::debug
            << cnetlink::get_instance().get_routes_in4(table_id).str()
            << std::endl;
      } break;
      case AF_INET6: {
        logging::debug
            << "[roflibs][cnetlink][route_route_cb] deleted route_in6"
            << std::endl
            << crtroute_in6((struct rtnl_route *)obj);
        unsigned int rtindex =
            cnetlink::get_instance().get_routes_in6(table_id).get_route(
                crtroute_in6((struct rtnl_route *)obj));
        cnetlink::get_instance().notify_route_in6_deleted(table_id, rtindex);
        cnetlink::get_instance().set_routes_in6(table_id).drop_route(rtindex);
        logging::debug
            << cnetlink::get_instance().get_routes_in6(table_id).str()
            << std::endl;
      } break;
      }
    } break;
    default: { logging::warn << "route/route: unknown NL action" << std::endl; }
    }
    logging::trace << "[roflibs][cnetlink][route_route_cb] status" << std::endl
                   << cnetlink::get_instance();
  } catch (eNetLinkNotFound &e) {
    logging::error << "cnetlink::route_route_cb() oops, route_route_cb() was "
                      "called with an invalid link"
                   << std::endl;
  } catch (crtroute::eRtRouteNotFound &e) {
    logging::error << "cnetlink::route_route_cb() oops, route_route_cb() was "
                      "called with an invalid route"
                   << std::endl;
  }

  nl_object_put(obj); // release reference to object
}

/* static C-callback */
void cnetlink::route_neigh_cb(struct nl_cache *cache, struct nl_object *obj,
                              int action, void *data) {
  if (std::string(nl_object_get_type(obj)) != std::string("route/neigh")) {
    logging::debug
        << "cnetlink::route_neigh_cb() ignoring non neighbor object received"
        << std::endl;
    return;
  }

  nl_object_get(obj); // get reference to object

  struct rtnl_neigh *neigh = (struct rtnl_neigh *)obj;

  int ifindex = rtnl_neigh_get_ifindex(neigh);
  int family = rtnl_neigh_get_family((struct rtnl_neigh *)obj);

  if (0 == ifindex) {
    logging::info << "cnetlink::route_neigh_cb() ignoring not existing link"
                  << std::endl;
    return;
  }

  try {
    switch (action) {
    case NL_ACT_NEW: {
      switch (family) {
      case AF_INET: {
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] new neigh_in4"
                       << std::endl
                       << crtneigh_in4((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_in4[ifindex].add_neigh(
                crtneigh_in4((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_in4_created(ifindex, nbindex);
      } break;
      case AF_INET6: {
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] new neigh_in6"
                       << std::endl
                       << crtneigh_in6((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_in6[ifindex].add_neigh(
                crtneigh_in6((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_in6_created(ifindex, nbindex);
      } break;
      case PF_BRIDGE: {
        // xxx implement bridge handling
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] new neigh_ll"
                       << std::endl
                       << crtneigh((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_ll[ifindex].add_neigh(
                crtneigh((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_ll_created(ifindex, nbindex);
      } break;
      }
    } break;
    case NL_ACT_CHANGE: {
      switch (family) {
      case AF_INET: {
        logging::debug
            << "[roflibs][cnetlink][route_neigh_cb] updated neigh_in4"
            << std::endl
            << crtneigh_in4((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_in4[ifindex].set_neigh(
                crtneigh_in4((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_in4_updated(ifindex, nbindex);
      } break;
      case AF_INET6: {
        logging::debug
            << "[roflibs][cnetlink][route_neigh_cb] updated neigh_in6"
            << std::endl
            << crtneigh_in6((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_in6[ifindex].set_neigh(
                crtneigh_in6((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_in6_updated(ifindex, nbindex);
      } break;
      case PF_BRIDGE: {
        // xxx implement bridge handling
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] updated neigh_ll"
                       << std::endl
                       << crtneigh((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_ll[ifindex].set_neigh(
                crtneigh((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_ll_updated(ifindex, nbindex);
      } break;
      }
    } break;
    case NL_ACT_DEL: {
      switch (family) {
      case AF_INET: {
        logging::debug
            << "[roflibs][cnetlink][route_neigh_cb] deleted neigh_in4"
            << std::endl
            << crtneigh_in4((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_in4[ifindex].get_neigh(
                crtneigh_in4((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_in4_deleted(ifindex, nbindex);
        cnetlink::get_instance().neighs_in4[ifindex].drop_neigh(nbindex);
      } break;
      case AF_INET6: {
        logging::debug
            << "[roflibs][cnetlink][route_neigh_cb] deleted neigh_in6"
            << std::endl
            << crtneigh_in6((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_in6[ifindex].get_neigh(
                crtneigh_in6((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_in6_deleted(ifindex, nbindex);
        cnetlink::get_instance().neighs_in6[ifindex].drop_neigh(nbindex);
      } break;
      case PF_BRIDGE: {
        // xxx implement bridge handling
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] deleted neigh_ll"
                       << std::endl
                       << crtneigh((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_ll[ifindex].get_neigh(
                crtneigh((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_ll_deleted(ifindex, nbindex);
        cnetlink::get_instance().neighs_ll[ifindex].drop_neigh(nbindex);
      } break;
      }
    } break;
    default: { logging::warn << "route/addr: unknown NL action" << std::endl; }
    }
    logging::trace << "[roflibs][cnetlink][route_neigh_cb] status" << std::endl
                   << cnetlink::get_instance();
  } catch (eNetLinkNotFound &e) {
    logging::error << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was "
                      "called with an invalid link"
                   << std::endl;
  } catch (crtneigh::eRtNeighNotFound &e) {
    logging::error << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was "
                      "called with an invalid neighbor"
                   << std::endl;
  }

  nl_object_put(obj); // release reference to object
}

void cnetlink::notify_link_created(unsigned int ifindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->link_created(ifindex);
  }
};

void cnetlink::notify_link_updated(const crtlink &newlink) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->link_updated(newlink);
  }
};

void cnetlink::notify_link_deleted(unsigned int ifindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->link_deleted(ifindex);
  }
};

void cnetlink::notify_addr_in4_created(unsigned int ifindex,
                                       unsigned int adindex) {

  logging::debug << "[cnetlink][notify_addr_in4_created] sending notifications:"
                 << std::endl;
  logging::debug << "<ifindex:" << ifindex << " adindex:" << adindex << " >"
                 << std::endl;
  logging::debug << addrs_in4[ifindex];

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->addr_in4_created(ifindex, adindex);
  }
};

void cnetlink::notify_addr_in6_created(unsigned int ifindex,
                                       unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->addr_in6_created(ifindex, adindex);
  }
};

void cnetlink::notify_addr_in4_updated(unsigned int ifindex,
                                       unsigned int adindex) {

  logging::debug << "[cnetlink][notify_addr_in4_updated] sending notifications:"
                 << std::endl;
  logging::debug << "<ifindex:" << ifindex << " adindex:" << adindex << " >"
                 << std::endl;
  logging::debug << addrs_in4[ifindex];

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->addr_in4_updated(ifindex, adindex);
  }
};

void cnetlink::notify_addr_in6_updated(unsigned int ifindex,
                                       unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->addr_in6_updated(ifindex, adindex);
  }
};

void cnetlink::notify_addr_in4_deleted(unsigned int ifindex,
                                       unsigned int adindex) {

  logging::debug << "[cnetlink][notify_addr_in4_deleted] sending notifications:"
                 << std::endl;
  logging::debug << "<ifindex:" << ifindex << " adindex:" << adindex << " >"
                 << std::endl;
  logging::debug << addrs_in4[ifindex];

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->addr_in4_deleted(ifindex, adindex);
  }
};

void cnetlink::notify_addr_in6_deleted(unsigned int ifindex,
                                       unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->addr_in6_deleted(ifindex, adindex);
  }
};

void cnetlink::notify_neigh_ll_created(unsigned int ifindex,
                                       unsigned int nbindex) {
  const rofl::caddress_ll &dst =
      neighs_ll[ifindex].get_neigh(nbindex).get_lladdr();

  logging::debug << "[cnetlink][notify_neigh_ll_created] sending notifications:"
                 << std::endl;
  logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex
                 << " dst:" << dst.str() << " >" << std::endl;
  logging::debug << neighs_ll[ifindex];

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->neigh_ll_created(ifindex, nbindex);
  }

  for (std::set<cnetlink_neighbour_observer *>::iterator it =
           nbobservers_ll[ifindex][dst].begin();
       it != nbobservers_ll[ifindex][dst].end(); ++it) {

    (*it)->neigh_ll_created(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_in4_created(unsigned int ifindex,
                                        unsigned int nbindex) {
  const rofl::caddress_in4 &dst =
      neighs_in4[ifindex].get_neigh(nbindex).get_dst();

  logging::debug
      << "[cnetlink][notify_neigh_in4_created] sending notifications:"
      << std::endl;
  logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex
                 << " dst:" << dst.str() << " >" << std::endl;
  logging::debug << neighs_in4[ifindex];

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->neigh_in4_created(ifindex, nbindex);
  }

  for (std::set<cnetlink_neighbour_observer *>::iterator it =
           nbobservers_in4[ifindex][dst].begin();
       it != nbobservers_in4[ifindex][dst].end(); ++it) {

    (*it)->neigh_in4_created(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_in6_created(unsigned int ifindex,
                                        unsigned int nbindex) {
  const rofl::caddress_in6 &dst =
      neighs_in6[ifindex].get_neigh(nbindex).get_dst();

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->neigh_in6_created(ifindex, nbindex);
  }

  for (std::set<cnetlink_neighbour_observer *>::iterator it =
           nbobservers_in6[ifindex][dst].begin();
       it != nbobservers_in6[ifindex][dst].end(); ++it) {
    (*it)->neigh_in6_created(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_ll_updated(unsigned int ifindex,
                                       unsigned int nbindex) {
  const rofl::caddress_ll &dst =
      neighs_ll[ifindex].get_neigh(nbindex).get_lladdr();

  logging::debug << "[cnetlink][notify_neigh_ll_updated] sending notifications:"
                 << std::endl;
  logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex
                 << " dst:" << dst.str() << " >" << std::endl;
  logging::debug << neighs_ll[ifindex];

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->neigh_ll_updated(ifindex, nbindex);
  }

  for (std::set<cnetlink_neighbour_observer *>::iterator it =
           nbobservers_ll[ifindex][dst].begin();
       it != nbobservers_ll[ifindex][dst].end(); ++it) {
    (*it)->neigh_ll_updated(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_in4_updated(unsigned int ifindex,
                                        unsigned int nbindex) {
  const rofl::caddress_in4 &dst =
      neighs_in4[ifindex].get_neigh(nbindex).get_dst();

  logging::debug
      << "[cnetlink][notify_neigh_in4_updated] sending notifications:"
      << std::endl;
  logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex
                 << " dst:" << dst.str() << " >" << std::endl;
  logging::debug << neighs_in4[ifindex];

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->neigh_in4_updated(ifindex, nbindex);
  }

  for (std::set<cnetlink_neighbour_observer *>::iterator it =
           nbobservers_in4[ifindex][dst].begin();
       it != nbobservers_in4[ifindex][dst].end(); ++it) {
    (*it)->neigh_in4_updated(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_in6_updated(unsigned int ifindex,
                                        unsigned int nbindex) {
  const rofl::caddress_in6 &dst =
      neighs_in6[ifindex].get_neigh(nbindex).get_dst();

  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->neigh_in6_updated(ifindex, nbindex);
  }

  for (std::set<cnetlink_neighbour_observer *>::iterator it =
           nbobservers_in6[ifindex][dst].begin();
       it != nbobservers_in6[ifindex][dst].end(); ++it) {
    (*it)->neigh_in6_updated(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_ll_deleted(unsigned int ifindex,
                                       unsigned int nbindex) {
  const rofl::caddress_ll &dst =
      neighs_ll[ifindex].get_neigh(nbindex).get_lladdr();

  logging::debug << "[cnetlink][notify_neigh_ll_deleted] sending notifications:"
                 << std::endl;
  logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex
                 << " dst:" << dst.str() << " >" << std::endl;
  logging::debug << neighs_ll[ifindex];

  // make local copy of set
  std::set<cnetlink_common_observer *> obs(observers);
  for (std::set<cnetlink_common_observer *>::iterator it = obs.begin();
       it != obs.end(); ++it) {
    (*it)->neigh_ll_deleted(ifindex, nbindex);
  }

  // make local copy of set
  std::set<cnetlink_neighbour_observer *> nbobs_ll(
      nbobservers_ll[ifindex][dst]);
  for (std::set<cnetlink_neighbour_observer *>::iterator it = nbobs_ll.begin();
       it != nbobs_ll.end(); ++it) {
    (*it)->neigh_ll_deleted(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_in4_deleted(unsigned int ifindex,
                                        unsigned int nbindex) {
  const rofl::caddress_in4 &dst =
      neighs_in4[ifindex].get_neigh(nbindex).get_dst();

  logging::debug
      << "[cnetlink][notify_neigh_in4_deleted] sending notifications:"
      << std::endl;
  logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex
                 << " dst:" << dst.str() << " >" << std::endl;
  logging::debug << neighs_in4[ifindex];

  // make local copy of set
  std::set<cnetlink_common_observer *> obs(observers);
  for (std::set<cnetlink_common_observer *>::iterator it = obs.begin();
       it != obs.end(); ++it) {
    (*it)->neigh_in4_deleted(ifindex, nbindex);
  }

  // make local copy of set
  std::set<cnetlink_neighbour_observer *> nbobs_in4(
      nbobservers_in4[ifindex][dst]);
  for (std::set<cnetlink_neighbour_observer *>::iterator it = nbobs_in4.begin();
       it != nbobs_in4.end(); ++it) {
    (*it)->neigh_in4_deleted(ifindex, nbindex);
  }
};

void cnetlink::notify_neigh_in6_deleted(unsigned int ifindex,
                                        unsigned int nbindex) {
  const rofl::caddress_in6 &dst =
      neighs_in6[ifindex].get_neigh(nbindex).get_dst();

  // make local copy of set
  std::set<cnetlink_common_observer *> obs(observers);
  for (std::set<cnetlink_common_observer *>::iterator it = obs.begin();
       it != obs.end(); ++it) {
    (*it)->neigh_in6_deleted(ifindex, nbindex);
  }

  // make local copy of set
  std::set<cnetlink_neighbour_observer *> nbobs_in6(
      nbobservers_in6[ifindex][dst]);
  for (std::set<cnetlink_neighbour_observer *>::iterator it = nbobs_in6.begin();
       it != nbobs_in6.end(); ++it) {
    (*it)->neigh_in6_deleted(ifindex, nbindex);
  }
};

void cnetlink::notify_route_in4_created(uint8_t table_id,
                                        unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->route_in4_created(table_id, adindex);
  }
};

void cnetlink::notify_route_in6_created(uint8_t table_id,
                                        unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->route_in6_created(table_id, adindex);
  }
};

void cnetlink::notify_route_in4_updated(uint8_t table_id,
                                        unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->route_in4_updated(table_id, adindex);
  }
};

void cnetlink::notify_route_in6_updated(uint8_t table_id,
                                        unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->route_in6_updated(table_id, adindex);
  }
};

void cnetlink::notify_route_in4_deleted(uint8_t table_id,
                                        unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->route_in4_deleted(table_id, adindex);
  }
};

void cnetlink::notify_route_in6_deleted(uint8_t table_id,
                                        unsigned int adindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->route_in6_deleted(table_id, adindex);
  }
};

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

void cnetlink::add_addr_in4(int ifindex, const rofl::caddress_in4 &laddr,
                            int prefixlen) {
  int rc = 0;

  struct nl_sock *sk = (struct nl_sock *)0;
  if ((sk = nl_socket_alloc()) == NULL) {
    throw eNetLinkFailed("cnetlink::add_addr_in4() nl_socket_alloc()");
  }

  int sd = 0;

  if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_addr_in4() nl_connect()");
  }

  struct rtnl_addr *addr = (struct rtnl_addr *)0;
  if ((addr = rtnl_addr_alloc()) == NULL) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_addr_in4() rtnl_addr_alloc()");
  }

  struct nl_addr *local = (struct nl_addr *)0;
  nl_addr_parse(laddr.str().c_str(), AF_INET, &local);
  rtnl_addr_set_local(addr, local);

  rtnl_addr_set_family(addr, AF_INET);
  rtnl_addr_set_ifindex(addr, ifindex);
  rtnl_addr_set_prefixlen(addr, prefixlen);
  rtnl_addr_set_flags(addr, 0);

  if ((rc = rtnl_addr_add(sk, addr, 0)) < 0) {
    rtnl_addr_put(addr);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_addr_in4() rtnl_addr_add()");
  }

  nl_addr_put(local);
  rtnl_addr_put(addr);
  nl_close(sk);
  nl_socket_free(sk);
};

void cnetlink::drop_addr_in4(int ifindex, const rofl::caddress_in4 &laddr,
                             int prefixlen) {
  int rc = 0;

  struct nl_sock *sk = (struct nl_sock *)0;
  if ((sk = nl_socket_alloc()) == NULL) {
    throw eNetLinkFailed("cnetlink::drop_addr_in4() nl_socket_alloc()");
  }

  int sd = 0;

  if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_addr_in4() nl_connect()");
  }

  struct rtnl_addr *addr = (struct rtnl_addr *)0;
  if ((addr = rtnl_addr_alloc()) == NULL) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_addr_in4() rtnl_addr_alloc()");
  }

  struct nl_addr *local = (struct nl_addr *)0;
  nl_addr_parse(laddr.str().c_str(), AF_INET, &local);
  rtnl_addr_set_local(addr, local);

  rtnl_addr_set_family(addr, AF_INET);
  rtnl_addr_set_ifindex(addr, ifindex);
  rtnl_addr_set_prefixlen(addr, prefixlen);
  rtnl_addr_set_flags(addr, 0);

  if ((rc = rtnl_addr_delete(sk, addr, 0)) < 0) {
    rtnl_addr_put(addr);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_addr_in4() rtnl_addr_delete()");
  }

  nl_addr_put(local);
  rtnl_addr_put(addr);
  nl_close(sk);
  nl_socket_free(sk);
};

void cnetlink::add_addr_in6(int ifindex, const rofl::caddress_in6 &laddr,
                            int prefixlen) {
  int rc = 0;

  struct nl_sock *sk = (struct nl_sock *)0;
  if ((sk = nl_socket_alloc()) == NULL) {
    throw eNetLinkFailed("cnetlink::add_addr_in6() nl_socket_alloc()");
  }

  int sd = 0;

  if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_addr_in6() nl_connect()");
  }

  struct rtnl_addr *addr = (struct rtnl_addr *)0;
  if ((addr = rtnl_addr_alloc()) == NULL) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_addr_in6() rtnl_addr_alloc()");
  }

  struct nl_addr *local = (struct nl_addr *)0;
  nl_addr_parse(laddr.str().c_str(), AF_INET6, &local);
  rtnl_addr_set_local(addr, local);

  rtnl_addr_set_family(addr, AF_INET6);
  rtnl_addr_set_ifindex(addr, ifindex);
  rtnl_addr_set_prefixlen(addr, prefixlen);
  rtnl_addr_set_flags(addr, 0);

  if ((rc = rtnl_addr_add(sk, addr, 0)) < 0) {
    rtnl_addr_put(addr);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::add_addr_in6() rtnl_addr_add()");
  }

  nl_addr_put(local);
  rtnl_addr_put(addr);
  nl_close(sk);
  nl_socket_free(sk);
};

void cnetlink::drop_addr_in6(int ifindex, const rofl::caddress_in6 &laddr,
                             int prefixlen) {
  int rc = 0;

  struct nl_sock *sk = (struct nl_sock *)0;
  if ((sk = nl_socket_alloc()) == NULL) {
    throw eNetLinkFailed("cnetlink::drop_addr_in6() nl_socket_alloc()");
  }

  int sd = 0;

  if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_addr_in6() nl_connect()");
  }

  struct rtnl_addr *addr = (struct rtnl_addr *)0;
  if ((addr = rtnl_addr_alloc()) == NULL) {
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_addr_in6() rtnl_addr_alloc()");
  }

  struct nl_addr *local = (struct nl_addr *)0;
  nl_addr_parse(laddr.str().c_str(), AF_INET6, &local);
  rtnl_addr_set_local(addr, local);

  rtnl_addr_set_family(addr, AF_INET6);
  rtnl_addr_set_ifindex(addr, ifindex);
  rtnl_addr_set_prefixlen(addr, prefixlen);
  rtnl_addr_set_flags(addr, 0);

  if ((rc = rtnl_addr_delete(sk, addr, 0)) < 0) {
    rtnl_addr_put(addr);
    nl_socket_free(sk);
    throw eNetLinkFailed("cnetlink::drop_addr_in6() rtnl_addr_delete()");
  }

  nl_addr_put(local);
  rtnl_addr_put(addr);
  nl_close(sk);
  nl_socket_free(sk);
};
