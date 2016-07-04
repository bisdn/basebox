/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "cnetlink.hpp"
#include "cnetlink_observer.hpp"

using namespace rofcore;

cnetlink::cnetlink() : thread(this) {

  sock = nl_socket_alloc();
  if (NULL == sock) {
    logging::crit << "cnetlink: failed to create netlink socket" << __FUNCTION__
                  << std::endl;
    throw eNetLinkCritical(__FUNCTION__);
  }

  try {
    init_caches();
    thread.start();
  } catch (...) {
    logging::error << "cnetlink: caught unkown exception during "
                   << __FUNCTION__ << std::endl;
  }
}

cnetlink::~cnetlink() {
  destroy_caches();
  nl_socket_free(sock);
}

void cnetlink::init_caches() {

  int rc = nl_cache_mngr_alloc(sock, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);

  if (rc < 0) {
    logging::crit
        << "cnetlink::init_caches() failed to allocate netlink cache manager"
        << std::endl;
    throw eNetLinkCritical("cnetlink::init_caches()");
  }

  // TODO read from /proc/sys/net/core/{r,w}mem_max
  int rx_size = 212992;
  int tx_size = 212992;

  if (0 != nl_socket_set_buffer_size(sock, rx_size, tx_size)) {
    logging::crit << "cnetlink: failed to resize socket buffers" << std::endl;
    throw eNetLinkCritical(__FUNCTION__);
  }
  nl_socket_set_msg_buf_size(sock, rx_size);

  caches[NL_LINK_CACHE] = NULL;
  caches[NL_NEIGH_CACHE] = NULL;

  rtnl_link_alloc_cache_flags(sock, AF_UNSPEC, &caches[NL_LINK_CACHE],
                              NL_CACHE_AF_ITER);
  if (0 != rc) {
    logging::error
        << "cnetlink::init_caches() rtnl_link_alloc_cache_flags failed rc="
        << rc << std::endl;
  }
  rc = nl_cache_mngr_add_cache(mngr, caches[NL_LINK_CACHE],
                               (change_func_t)&route_link_cb, NULL);
  if (0 != rc) {
    logging::error << "cnetlink::init_caches() add route/link to cache mngr"
                   << std::endl;
  }

  rc = rtnl_neigh_alloc_cache_flags(sock, &caches[NL_NEIGH_CACHE],
                                    NL_CACHE_AF_ITER);
  if (0 != rc) {
    logging::error
        << "cnetlink::init_caches() rtnl_link_alloc_cache_flags failed rc="
        << rc << std::endl;
  }
  rc = nl_cache_mngr_add_cache(mngr, caches[NL_NEIGH_CACHE],
                               (change_func_t)&route_neigh_cb, NULL);
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

  obj = nl_cache_get_first(caches[NL_NEIGH_CACHE]);
  while (0 != obj) {
    nl_object_get(obj);
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
    nl_object_put(obj);
    obj = nl_cache_get_next(obj);
  }

  thread.add_read_fd(nl_cache_mngr_get_fd(mngr), true, false);
}

void cnetlink::destroy_caches() {
  thread.drop_read_fd(nl_cache_mngr_get_fd(mngr), false);
  nl_cache_mngr_free(mngr);
}

cnetlink &cnetlink::get_instance() {
  static cnetlink instance;
  return instance;
}

void cnetlink::handle_read_event(rofl::cthread &thread, int fd) {
  if (fd == nl_cache_mngr_get_fd(mngr)) {
    int rv = nl_cache_mngr_data_ready(mngr);
    logging::debug << "cnetlink #processed=" << rv << std::endl;
  }
}

void cnetlink::handle_write_event(rofl::cthread &thread, int fd) {
  logging::debug << "cnetlink write ready on fd=" << fd << std::endl;
  // currently not in use
}

void cnetlink::handle_timeout(rofl::cthread &thread, uint32_t timer_id,
                              const std::list<unsigned int> &ttypes) {
  switch (timer_id) {
  case NL_TIMER_RESYNC: {
    int r = nl_cache_refill(sock, caches[NL_LINK_CACHE]);
    if (r < 0) {
      logging::error << __FUNCTION__ << " failed to refill NL_LINK_CACHE"
                     << std::endl;
      return;
    }
    r = nl_cache_refill(sock, caches[NL_NEIGH_CACHE]);
    if (r < 0) {
      logging::error << __FUNCTION__ << " failed to refill NL_NEIGH_CACHE"
                     << std::endl;
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
          int nbindex = neighs_ll[ifindex].add_neigh(neigh);
          cnetlink::get_instance().notify_neigh_ll_created(ifindex, nbindex);
        } else {
          logging::crit << __FUNCTION__ << " no link ifindex=" << ifindex
                        << std::endl;
        }
      } break;
      default:
        break;
      }
      obj = nl_cache_get_next(obj);
    }

    thread.add_timer(NL_TIMER_RESYNC, rofl::ctimespec().expire_in(5));
  } break;
  default:
    break;
  }
}

/* static C-callback */
void cnetlink::route_link_cb(struct nl_cache *cache, struct nl_object *obj,
                             int action, void *data) {
  nl_object_get(obj); // increment reference counter by one
  if (std::string(nl_object_get_type(obj)) != std::string("route/link")) {
    logging::warn
        << "cnetlink::route_link_cb() neigh without link object received"
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
      switch (rtlink.get_family()) {
      case AF_UNSPEC:
        logging::info << "ignore AF_UNSPEC change:" << rtlink;
        if (rtlink.get_master()) {
          break; // ignore AF_UNSPEC changes for slaves
        }
      // fallthrough
      default:
        cnetlink::get_instance().notify_link_updated(rtlink);
        cnetlink::get_instance().set_links().set_link(rtlink);
        logging::debug
            << cnetlink::get_instance().get_links().get_link(ifindex).str()
            << std::endl;
        break;
      }
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
  } catch (std::exception &e) {
    logging::crit << "cnetlink::route_neigh_cb() oops unknown exception"
                  << e.what() << std::endl;
  }
}

/* static C-callback */
void cnetlink::route_neigh_cb(struct nl_cache *cache, struct nl_object *obj,
                              int action, void *data) {
  if (std::string(nl_object_get_type(obj)) != std::string("route/neigh")) {
    logging::crit
        << "cnetlink::route_neigh_cb() ignoring non neighbor object received"
        << std::endl;
    return;
  }

  nl_object_get(obj); // get reference to object

  struct rtnl_neigh *neigh = (struct rtnl_neigh *)obj;

  int ifindex = rtnl_neigh_get_ifindex(neigh);
  int family = rtnl_neigh_get_family((struct rtnl_neigh *)obj);

  if (0 == ifindex) {
    logging::error << __FUNCTION__ << "() ignoring not existing link"
                   << std::endl;
    return;
  }

  try {
    switch (action) {
    case NL_ACT_NEW: {
      switch (family) {
      case PF_BRIDGE: {
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] new neigh_ll"
                       << std::endl
                       << crtneigh((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_ll[ifindex].add_neigh(
                crtneigh((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_ll_created(ifindex, nbindex);
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
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] updated neigh_ll"
                       << std::endl
                       << crtneigh((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_ll[ifindex].set_neigh(
                crtneigh((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_ll_updated(ifindex, nbindex);
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
        logging::debug << "[roflibs][cnetlink][route_neigh_cb] deleted neigh_ll"
                       << std::endl
                       << crtneigh((struct rtnl_neigh *)obj);
        unsigned int nbindex =
            cnetlink::get_instance().neighs_ll[ifindex].get_neigh(
                crtneigh((struct rtnl_neigh *)obj));
        cnetlink::get_instance().notify_neigh_ll_deleted(ifindex, nbindex);
        cnetlink::get_instance().neighs_ll[ifindex].drop_neigh(nbindex);
      } break;
      case AF_INET:
      case AF_INET6:
      default:
        break;
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
  } catch (std::exception &e) {
    logging::crit << "cnetlink::route_neigh_cb() oops unknown exception"
                  << e.what() << std::endl;
  }

  nl_object_put(obj); // release reference to object
  cnetlink::get_instance().set_neigh_timeout();
}

void cnetlink::set_neigh_timeout() {
  thread.add_timer(NL_TIMER_RESYNC, rofl::ctimespec().expire_in(5));
}

void cnetlink::notify_link_created(unsigned int ifindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->link_created(ifindex);
  }
}

void cnetlink::notify_link_updated(const crtlink &newlink) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->link_updated(newlink);
  }
}

void cnetlink::notify_link_deleted(unsigned int ifindex) {
  for (std::set<cnetlink_common_observer *>::iterator it = observers.begin();
       it != observers.end(); ++it) {
    (*it)->link_deleted(ifindex);
  }
}

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
}

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
}

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
