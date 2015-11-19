/*
 * clinkcache.cc
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#include "cnetlink.hpp"

using namespace rofcore;

cnetlink* cnetlink::netlink = (cnetlink*)0;



cnetlink::cnetlink() :
		mngr(0)
{
	try {
		init_caches();
	} catch (...) {
		logging::error << "cnetlink: caught unkown exception during " << __FUNCTION__ << std::endl;
	}
}



cnetlink::cnetlink(cnetlink const& linkcache) :
		mngr(0)
{
	init_caches();
}



cnetlink::~cnetlink()
{
	destroy_caches();
}

void
cnetlink::init_caches()
{
	int rc = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);
	if (rc < 0) {
		logging::crit << "cnetlink::init_caches() failed to allocate netlink cache manager" << std::endl;
		throw eNetLinkCritical("cnetlink::init_caches()");
	}

	caches[NL_LINK_CACHE] = NULL;
	caches[NL_ADDR_CACHE] = NULL;
	caches[NL_ROUTE_CACHE] = NULL;
	caches[NL_NEIGH_CACHE] = NULL;

	rc = nl_cache_mngr_add(mngr, "route/link", (change_func_t)&route_link_cb, NULL, &caches[NL_LINK_CACHE]);
	if (0 != rc){
		logging::error << "cnetlink::init_caches() add route/link to cache mngr" << std::endl;
	}
	rc = nl_cache_mngr_add(mngr, "route/addr", (change_func_t)&route_addr_cb, NULL, &caches[NL_ADDR_CACHE]);
	if (0 != rc){
		logging::error << "cnetlink::init_caches() add route/addr to cache mngr" << std::endl;
	}
	rc = nl_cache_mngr_add(mngr, "route/route", (change_func_t)&route_route_cb, NULL, &caches[NL_ROUTE_CACHE]);
	if (0 != rc){
		logging::error << "cnetlink::init_caches() add route/route to cache mngr" << std::endl;
	}
	rc = nl_cache_mngr_add(mngr, "route/neigh", (change_func_t)&route_neigh_cb, NULL, &caches[NL_NEIGH_CACHE]);
	if (0 != rc){
		logging::error << "cnetlink::init_caches() add route/neigh to cache mngr" << std::endl;
	}
	struct nl_object* obj = nl_cache_get_first(caches[NL_LINK_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		rtlinks.add_link(crtlink((struct rtnl_link*)obj));
		nl_object_put(obj);
		obj = nl_cache_get_next(obj);
	}

	obj = nl_cache_get_first(caches[NL_ADDR_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		unsigned int ifindex = rtnl_addr_get_ifindex((struct rtnl_addr*)obj);
		switch (rtnl_addr_get_family((struct rtnl_addr*)obj)) {
		case AF_INET:
			rtlinks.set_link(ifindex).set_addrs_in4().add_addr(crtaddr_in4((struct rtnl_addr*)obj));
			break;
		case AF_INET6:
			rtlinks.set_link(ifindex).set_addrs_in6().add_addr(crtaddr_in6((struct rtnl_addr*)obj));
			break;
		}
		nl_object_put(obj);
		obj = nl_cache_get_next(obj);
	}

	obj = nl_cache_get_first(caches[NL_ROUTE_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		int table_id = rtnl_route_get_table((struct rtnl_route*)obj);
		switch (rtnl_route_get_family((struct rtnl_route*)obj)) {
		case AF_INET:
			rtroutes_in4[table_id].add_route(crtroute_in4((struct rtnl_route*)obj));
			break;
		case AF_INET6:
			rtroutes_in6[table_id].add_route(crtroute_in6((struct rtnl_route*)obj));
			break;
		}
		nl_object_put(obj);
		obj = nl_cache_get_next(obj);
	}

	obj = nl_cache_get_first(caches[NL_NEIGH_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		// not handled at all? was #if 0
#if 0
		switch (rtnl_neigh_get_family((struct rtnl_neigh*)obj)) {
		case AF_INET:	set_neigh_in4(crtneigh_in4((struct rtnl_neigh*)obj)); break;
		case AF_INET6:	set_neigh_in6(crtneigh_in6((struct rtnl_neigh*)obj)); break;
		}
#endif
		nl_object_put(obj);
		obj = nl_cache_get_next(obj);
	}

	register_filedesc_r(nl_cache_mngr_get_fd(mngr));
}



void
cnetlink::destroy_caches()
{
	deregister_filedesc_r(nl_cache_mngr_get_fd(mngr));

	nl_cache_mngr_free(mngr);
}





cnetlink&
cnetlink::get_instance()
{
	if (0 == cnetlink::netlink) {
		cnetlink::netlink = new cnetlink();
	}
	return *(cnetlink::netlink);
}


void
cnetlink::update_link_cache()
{
	if (cnetlink::get_instance().get_links().size() != nl_cache_nitems(caches[NL_LINK_CACHE])) {

		logging::warn << "[cnetlink][" << __FUNCTION__ << "] #links=" << cnetlink::get_instance().get_links().size()
				<< " #cacheitems=" << nl_cache_nitems(caches[NL_LINK_CACHE]) << std::endl;

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
		if (rv != 0) {
			logging::error << "[cnetlink][" << __FUNCTION__ << "] nl_cache_refill failed" << std::endl;
		}

		nl_close(sk);
		nl_socket_free(sk);
	}

	// fixme sync with local links
}


void
cnetlink::handle_revent(int fd)
{
	if (fd == nl_cache_mngr_get_fd(mngr)) {
		int rv = nl_cache_mngr_data_ready(mngr);
		logging::debug << "cnetlink #processed=" << rv << std::endl;
	}

	// fixme these should rather be event based
	update_link_cache();


	// reregister fd
	register_filedesc_r(nl_cache_mngr_get_fd(mngr));
}



/* static C-callback */
void
cnetlink::route_link_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	try {
		if (std::string(nl_object_get_type(obj)) != std::string("route/link")) {
			logging::warn << "cnetlink::route_link_cb() ignoring non link object received" << std::endl;
			return;
		}

		unsigned int ifindex = rtnl_link_get_ifindex((struct rtnl_link*)obj);
		nl_object_get(obj); // get reference to object
		crtlink rtlink((struct rtnl_link*)obj);

		switch (action) {
		case NL_ACT_NEW: {
			switch(rtlink.get_family()) {
			case AF_BRIDGE:
				/* link got enslaved, check if its existing and update */
				if (cnetlink::get_instance().get_links().has_link(rtlink.get_ifindex())) {

					// todo this could be optimized by just changing the existing link
					rtlink.set_addrs_in4() = cnetlink::get_instance().get_links().get_link(rtlink.get_ifindex()).get_addrs_in4();
					rtlink.set_addrs_in6() = cnetlink::get_instance().get_links().get_link(rtlink.get_ifindex()).get_addrs_in6();
					rtlink.set_neighs_in4() = cnetlink::get_instance().get_links().get_link(rtlink.get_ifindex()).get_neighs_in4();
					rtlink.set_neighs_in6() = cnetlink::get_instance().get_links().get_link(rtlink.get_ifindex()).get_neighs_in6();
					rtlink.set_neighs_ll() = cnetlink::get_instance().get_links().get_link(rtlink.get_ifindex()).get_neighs_ll();

					cnetlink::get_instance().set_links().add_link(rtlink); // overwrite old link
					logging::notice << "link new (bridge slave): " << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
					cnetlink::get_instance().notify_link_created(ifindex);
				} else {
					// fixme can this happen?
					logging::crit << "unexpected behavior" << std::endl;
				}

				break;
			default:
				cnetlink::get_instance().set_links().add_link(rtlink); // fixme this might overwrite addr/neighs/routes (due to missing entries in the cache)
				logging::notice << "link new: " << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_link_created(ifindex);
				break;
			}
		} break;
		case NL_ACT_CHANGE: {
			 // fixme this might overwrite addr/neighs/routes (due to missing entries in the cache)
			cnetlink::get_instance().set_links().set_link(rtlink);
			logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
			cnetlink::get_instance().notify_link_updated(ifindex);
		} break;
		case NL_ACT_DEL: {
			// xxx check if this has to be handled like new
			//notify_link_deleted(ifindex);
			cnetlink::get_instance().set_links().drop_link(ifindex);
			logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
			cnetlink::get_instance().notify_link_deleted(ifindex);
		} break;
		default: {
			logging::debug << "route/link: unknown NL action" << std::endl;
		}
		}

		nl_object_put(obj); // release reference to object

	} catch (eNetLinkNotFound& e) {
		// NL_ACT_CHANGE => ifindex not found
		logging::error << "cnetlink::route_link_cb() oops, route_link_cb() caught eNetLinkNotFound" << std::endl;
	} catch (crtlink::eRtLinkNotFound& e) {
		logging::error << "cnetlink::route_link_cb() oops, route_link_cb() caught eRtLinkNotFound" << std::endl;
	}
}


/* static C-callback */
void
cnetlink::route_addr_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/addr")) {
		logging::debug << "cnetlink::route_addr_cb() ignoring non addr object received" << std::endl;
		return;
	}

	nl_object_get(obj); // get reference to object

	unsigned int ifindex = rtnl_addr_get_ifindex((struct rtnl_addr*)obj);
	int family = rtnl_addr_get_family((struct rtnl_addr*)obj);

	try {
		switch (action) {
		case NL_ACT_NEW: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_addr_cb] new addr_in4" << std::endl << crtaddr_in4((struct rtnl_addr*)obj);
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in4().add_addr(crtaddr_in4((struct rtnl_addr*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_addr_in4_created(ifindex, adindex);
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_addr_cb] new addr_in6" << std::endl << crtaddr_in6((struct rtnl_addr*)obj);
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in6().add_addr(crtaddr_in6((struct rtnl_addr*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_addr_in6_created(ifindex, adindex);
			} break;
			}

		} break;
		case NL_ACT_CHANGE: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_addr_cb] updated addr_in4" << std::endl << crtaddr_in4((struct rtnl_addr*)obj);
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in4().set_addr(crtaddr_in4((struct rtnl_addr*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_addr_in4_updated(ifindex, adindex);
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_addr_cb] updated addr_in6" << std::endl << crtaddr_in6((struct rtnl_addr*)obj);
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in6().set_addr(crtaddr_in6((struct rtnl_addr*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_addr_in6_updated(ifindex, adindex);
			} break;
			}

		} break;
		case NL_ACT_DEL: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_addr_cb] deleted addr_in4" << std::endl << crtaddr_in4((struct rtnl_addr*)obj);
				unsigned int adindex = cnetlink::get_instance().set_links().get_link(ifindex).get_addrs_in4().get_addr(crtaddr_in4((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in4_deleted(ifindex, adindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in4().drop_addr(adindex);
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_addr_cb] deleted addr_in6" << std::endl << crtaddr_in6((struct rtnl_addr*)obj);
				unsigned int adindex = cnetlink::get_instance().set_links().get_link(ifindex).get_addrs_in6().get_addr(crtaddr_in6((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in6_deleted(ifindex, adindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in6().drop_addr(adindex);
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
			} break;
			}

		} break;
		default: {
			logging::debug << "route/addr: unknown NL action" << std::endl;
		}
		}
		logging::trace << "[roflibs][cnetlink][route_addr_cb] status" << std::endl << cnetlink::get_instance();
	} catch (eNetLinkNotFound& e) {
		logging::error << "cnetlink::route_addr_cb() oops, route_addr_cb() was called with an invalid link [1]" << std::endl;
	} catch (crtaddr::eRtAddrNotFound& e) {
		logging::error << "cnetlink::route_addr_cb() oops, route_addr_cb() was called with an invalid address [2]" << std::endl;
	}

	nl_object_put(obj); // release reference to object
}


/* static C-callback */
void
cnetlink::route_route_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/route")) {
		logging::debug << "cnetlink::route_route_cb() ignoring non route object received" << std::endl;
		return;
	}

	nl_object_get(obj); // get reference to object

	//logging::debug << "cnetlink::route_route_cb() called" << std::endl;

	int family = rtnl_route_get_family((struct rtnl_route*)obj);
	int table_id = rtnl_route_get_table((struct rtnl_route*)obj);

	try {
		switch (action) {
		case NL_ACT_NEW: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_route_cb] new route_in4" << std::endl << crtroute_in4((struct rtnl_route*)obj);
				unsigned int rtindex = cnetlink::get_instance().set_routes_in4(table_id).add_route(crtroute_in4((struct rtnl_route*)obj));
				logging::debug << cnetlink::get_instance().get_routes_in4(table_id).str() << std::endl;
				cnetlink::get_instance().notify_route_in4_created(table_id, rtindex);
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_route_cb] new route_in6" << std::endl << crtroute_in6((struct rtnl_route*)obj);
				unsigned int rtindex = cnetlink::get_instance().set_routes_in6(table_id).add_route(crtroute_in6((struct rtnl_route*)obj));
				logging::debug << cnetlink::get_instance().get_routes_in6(table_id).str() << std::endl;
				cnetlink::get_instance().notify_route_in6_created(table_id, rtindex);
			} break;
			}
		} break;
		case NL_ACT_CHANGE: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_route_cb] updated route_in4" << std::endl << crtroute_in4((struct rtnl_route*)obj);
				unsigned int rtindex = cnetlink::get_instance().set_routes_in4(table_id).set_route(crtroute_in4((struct rtnl_route*)obj));
				logging::debug << cnetlink::get_instance().get_routes_in4(table_id).str() << std::endl;
				cnetlink::get_instance().notify_route_in4_updated(table_id, rtindex);
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_route_cb] updated route_in6" << std::endl << crtroute_in6((struct rtnl_route*)obj);
				unsigned int rtindex = cnetlink::get_instance().set_routes_in6(table_id).set_route(crtroute_in6((struct rtnl_route*)obj));
				logging::debug << cnetlink::get_instance().get_routes_in6(table_id).str() << std::endl;
				cnetlink::get_instance().notify_route_in6_updated(table_id, rtindex);
			} break;
			}
		} break;
		case NL_ACT_DEL: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_route_cb] deleted route_in4" << std::endl << crtroute_in4((struct rtnl_route*)obj);
				unsigned int rtindex = cnetlink::get_instance().get_routes_in4(table_id).get_route(crtroute_in4((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in4_deleted(table_id, rtindex);
				cnetlink::get_instance().set_routes_in4(table_id).drop_route(rtindex);
				logging::debug << cnetlink::get_instance().get_routes_in4(table_id).str() << std::endl;
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_route_cb] deleted route_in6" << std::endl << crtroute_in6((struct rtnl_route*)obj);
				unsigned int rtindex = cnetlink::get_instance().get_routes_in6(table_id).get_route(crtroute_in6((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in6_deleted(table_id, rtindex);
				cnetlink::get_instance().set_routes_in6(table_id).drop_route(rtindex);
				logging::debug << cnetlink::get_instance().get_routes_in6(table_id).str() << std::endl;
			} break;
			}
		} break;
		default: {
			logging::debug << "route/route: unknown NL action" << std::endl;
		}
		}
		logging::trace << "[roflibs][cnetlink][route_route_cb] status" << std::endl << cnetlink::get_instance();
	} catch (eNetLinkNotFound& e) {
		logging::error << "cnetlink::route_route_cb() oops, route_route_cb() was called with an invalid link" << std::endl;
	} catch (crtroute::eRtRouteNotFound& e) {
		logging::error << "cnetlink::route_route_cb() oops, route_route_cb() was called with an invalid route" << std::endl;
	}

	nl_object_put(obj); // release reference to object
}


/* static C-callback */
void
cnetlink::route_neigh_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/neigh")) {
		logging::debug << "cnetlink::route_neigh_cb() ignoring non neighbor object received" << std::endl;
		return;
	}

	nl_object_get(obj); // get reference to object

	struct rtnl_neigh* neigh = (struct rtnl_neigh*)obj;

	int ifindex = rtnl_neigh_get_ifindex(neigh);
	int family = rtnl_neigh_get_family((struct rtnl_neigh*)obj);

	try {
		switch (action) {
		case NL_ACT_NEW: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] new neigh_in4" << std::endl << crtneigh_in4((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in4().add_neigh(crtneigh_in4((struct rtnl_neigh*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_neigh_in4_created(ifindex, nbindex);
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] new neigh_in6" << std::endl << crtneigh_in6((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in6().add_neigh(crtneigh_in6((struct rtnl_neigh*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_neigh_in6_created(ifindex, nbindex);
			} break;
			case PF_BRIDGE: {
				// xxx implement bridge handling
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] new neigh_ll" << std::endl << crtneigh((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_ll().add_neigh(crtneigh((struct rtnl_neigh*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_neigh_ll_created(ifindex, nbindex);
			} break;

			}
		} break;
		case NL_ACT_CHANGE: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] updated neigh_in4" << std::endl << crtneigh_in4((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in4().set_neigh(crtneigh_in4((struct rtnl_neigh*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_neigh_in4_updated(ifindex, nbindex);
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] updated neigh_in6" << std::endl << crtneigh_in6((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in6().set_neigh(crtneigh_in6((struct rtnl_neigh*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_neigh_in6_updated(ifindex, nbindex);
			} break;
			case PF_BRIDGE: {
				// xxx implement bridge handling
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] updated neigh_ll" << std::endl << crtneigh((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_ll().set_neigh(crtneigh((struct rtnl_neigh*)obj));
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
				cnetlink::get_instance().notify_neigh_ll_updated(ifindex, nbindex);
			} break;
			}
		} break;
		case NL_ACT_DEL: {
			switch (family) {
			case AF_INET: {
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] deleted neigh_in4" << std::endl << crtneigh_in4((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in4().get_neigh(crtneigh_in4((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in4_deleted(ifindex, nbindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in4().drop_neigh(nbindex);
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
			} break;
			case AF_INET6: {
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] deleted neigh_in6" << std::endl << crtneigh_in6((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in6().get_neigh(crtneigh_in6((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in6_deleted(ifindex, nbindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in6().drop_neigh(nbindex);
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
			} break;
			case PF_BRIDGE: {
				// xxx implement bridge handling
				logging::debug << "[roflibs][cnetlink][route_neigh_cb] deleted neigh_ll" << std::endl << crtneigh((struct rtnl_neigh*)obj);
				unsigned int nbindex = cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_ll().get_neigh(crtneigh((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_ll_deleted(ifindex, nbindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_ll().drop_neigh(nbindex);
				logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).str() << std::endl;
			} break;
			}
		} break;
		default: {
			logging::debug << "route/addr: unknown NL action" << std::endl;
		}
		}
		logging::trace << "[roflibs][cnetlink][route_neigh_cb] status" << std::endl << cnetlink::get_instance();
	} catch (eNetLinkNotFound& e) {
		logging::error << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was called with an invalid link" << std::endl;
	} catch (crtneigh::eRtNeighNotFound& e) {
		logging::error << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was called with an invalid neighbor" << std::endl;
	}

	nl_object_put(obj); // release reference to object
}



void
cnetlink::notify_link_created(unsigned int ifindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->link_created(ifindex);
	}
};



void
cnetlink::notify_link_updated(unsigned int ifindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->link_updated(ifindex);
	}
};



void
cnetlink::notify_link_deleted(unsigned int ifindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->link_deleted(ifindex);
	}
};



void
cnetlink::notify_addr_in4_created(unsigned int ifindex, unsigned int adindex) {

	logging::debug << "[cnetlink][notify_addr_in4_created] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " adindex:" << adindex << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->addr_in4_created(ifindex, adindex);
	}
};



void
cnetlink::notify_addr_in6_created(unsigned int ifindex, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->addr_in6_created(ifindex, adindex);
	}
};



void
cnetlink::notify_addr_in4_updated(unsigned int ifindex, unsigned int adindex) {

	logging::debug << "[cnetlink][notify_addr_in4_updated] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " adindex:" << adindex << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->addr_in4_updated(ifindex, adindex);
	}
};



void
cnetlink::notify_addr_in6_updated(unsigned int ifindex, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->addr_in6_updated(ifindex, adindex);
	}
};



void
cnetlink::notify_addr_in4_deleted(unsigned int ifindex, unsigned int adindex) {

	logging::debug << "[cnetlink][notify_addr_in4_deleted] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " adindex:" << adindex << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_addrs_in4();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->addr_in4_deleted(ifindex, adindex);
	}
};



void
cnetlink::notify_addr_in6_deleted(unsigned int ifindex, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->addr_in6_deleted(ifindex, adindex);
	}
};

void
cnetlink::notify_neigh_ll_created(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_ll& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_ll().get_neigh(nbindex).get_lladdr();

	logging::debug << "[cnetlink][notify_neigh_ll_created] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex << " dst:" << dst.str() << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_ll();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_ll_created(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_ll[ifindex][dst].begin();
				it != nbobservers_ll[ifindex][dst].end(); ++it) {

		(*it)->neigh_ll_created(ifindex, nbindex);
	}
};

void
cnetlink::notify_neigh_in4_created(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in4& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in4().get_neigh(nbindex).get_dst();

	logging::debug << "[cnetlink][notify_neigh_in4_created] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex << " dst:" << dst.str() << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in4();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_in4_created(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_in4[ifindex][dst].begin();
				it != nbobservers_in4[ifindex][dst].end(); ++it) {

		(*it)->neigh_in4_created(ifindex, nbindex);
	}
};



void
cnetlink::notify_neigh_in6_created(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in6& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in6().get_neigh(nbindex).get_dst();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_in6_created(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_in6[ifindex][dst].begin();
				it != nbobservers_in6[ifindex][dst].end(); ++it) {
		(*it)->neigh_in6_created(ifindex, nbindex);
	}
};

void
cnetlink::notify_neigh_ll_updated(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_ll& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_ll().get_neigh(nbindex).get_lladdr();

	logging::debug << "[cnetlink][notify_neigh_ll_updated] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex << " dst:" << dst.str() << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_ll();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_ll_updated(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_ll[ifindex][dst].begin();
				it != nbobservers_ll[ifindex][dst].end(); ++it) {
		(*it)->neigh_ll_updated(ifindex, nbindex);
	}
};

void
cnetlink::notify_neigh_in4_updated(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in4& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in4().get_neigh(nbindex).get_dst();

	logging::debug << "[cnetlink][notify_neigh_in4_updated] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex << " dst:" << dst.str() << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in4();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_in4_updated(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_in4[ifindex][dst].begin();
				it != nbobservers_in4[ifindex][dst].end(); ++it) {
		(*it)->neigh_in4_updated(ifindex, nbindex);
	}
};



void
cnetlink::notify_neigh_in6_updated(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in6& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in6().get_neigh(nbindex).get_dst();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_in6_updated(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_in6[ifindex][dst].begin();
				it != nbobservers_in6[ifindex][dst].end(); ++it) {
		(*it)->neigh_in6_updated(ifindex, nbindex);
	}
};

void
cnetlink::notify_neigh_ll_deleted(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_ll& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_ll().get_neigh(nbindex).get_lladdr();

	logging::debug << "[cnetlink][notify_neigh_ll_deleted] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex << " dst:" << dst.str() << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_ll();

	// make local copy of set
	std::set<cnetlink_common_observer*> obs(observers);
	for (std::set<cnetlink_common_observer*>::iterator
			it = obs.begin(); it != obs.end(); ++it) {
		(*it)->neigh_ll_deleted(ifindex, nbindex);
	}

	// make local copy of set
	std::set<cnetlink_neighbour_observer*> nbobs_ll(nbobservers_ll[ifindex][dst]);
	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobs_ll.begin(); it != nbobs_ll.end(); ++it) {
		(*it)->neigh_ll_deleted(ifindex, nbindex);
	}
};

void
cnetlink::notify_neigh_in4_deleted(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in4& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in4().get_neigh(nbindex).get_dst();

	logging::debug << "[cnetlink][notify_neigh_in4_deleted] sending notifications:" << std::endl;
	logging::debug << "<ifindex:" << ifindex << " nbindex:" << nbindex << " dst:" << dst.str() << " >" << std::endl;
	logging::debug << cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in4();

	// make local copy of set
	std::set<cnetlink_common_observer*> obs(observers);
	for (std::set<cnetlink_common_observer*>::iterator
			it = obs.begin(); it != obs.end(); ++it) {
		(*it)->neigh_in4_deleted(ifindex, nbindex);
	}

	// make local copy of set
	std::set<cnetlink_neighbour_observer*> nbobs_in4(nbobservers_in4[ifindex][dst]);
	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobs_in4.begin(); it != nbobs_in4.end(); ++it) {
		(*it)->neigh_in4_deleted(ifindex, nbindex);
	}
};



void
cnetlink::notify_neigh_in6_deleted(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in6& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in6().get_neigh(nbindex).get_dst();

	// make local copy of set
	std::set<cnetlink_common_observer*> obs(observers);
	for (std::set<cnetlink_common_observer*>::iterator
			it = obs.begin(); it != obs.end(); ++it) {
		(*it)->neigh_in6_deleted(ifindex, nbindex);
	}

	// make local copy of set
	std::set<cnetlink_neighbour_observer*> nbobs_in6(nbobservers_in6[ifindex][dst]);
	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobs_in6.begin(); it != nbobs_in6.end(); ++it) {
		(*it)->neigh_in6_deleted(ifindex, nbindex);
	}
};



void
cnetlink::notify_route_in4_created(uint8_t table_id, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->route_in4_created(table_id, adindex);
	}
};



void
cnetlink::notify_route_in6_created(uint8_t table_id, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->route_in6_created(table_id, adindex);
	}
};



void
cnetlink::notify_route_in4_updated(uint8_t table_id, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->route_in4_updated(table_id, adindex);
	}
};



void
cnetlink::notify_route_in6_updated(uint8_t table_id, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->route_in6_updated(table_id, adindex);
	}
};



void
cnetlink::notify_route_in4_deleted(uint8_t table_id, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->route_in4_deleted(table_id, adindex);
	}
};



void
cnetlink::notify_route_in6_deleted(uint8_t table_id, unsigned int adindex) {
	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->route_in6_deleted(table_id, adindex);
	}
};

void
cnetlink::add_neigh_ll(int ifindex, const rofl::caddress_ll& addr)
{
	// fixme check if ifindex is a bridge interface

	struct rtnl_neigh *neigh = rtnl_neigh_alloc();

	rtnl_neigh_set_ifindex(neigh, ifindex);
	rtnl_neigh_set_family(neigh, PF_BRIDGE);
	rtnl_neigh_set_state(neigh, NUD_NOARP|NUD_REACHABLE);
	rtnl_neigh_set_flags(neigh, NTF_MASTER);
	rtnl_neigh_set_vlan(neigh, 1); // todo expose

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

void
cnetlink::drop_neigh_ll(int ifindex, const rofl::caddress_ll& addr)
{
	struct rtnl_neigh *neigh = rtnl_neigh_alloc();

	rtnl_neigh_set_ifindex(neigh, ifindex);
	rtnl_neigh_set_family(neigh, PF_BRIDGE);
	rtnl_neigh_set_state(neigh, NUD_NOARP|NUD_REACHABLE);
	rtnl_neigh_set_flags(neigh, NTF_MASTER);
	rtnl_neigh_set_vlan(neigh, 1); // todo expose

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
	if ((rc = rtnl_neigh_delete(sk, neigh, 0)) < 0) {
		rtnl_neigh_put(neigh);
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::add_neigh_ll() rtnl_neigh_add()");
	}

	nl_close(sk);
	nl_socket_free(sk);
	rtnl_neigh_put(neigh);
}

void
cnetlink::add_addr_in4(int ifindex, const rofl::caddress_in4& laddr, int prefixlen)
{
	int rc = 0;

	struct nl_sock *sk = (struct nl_sock*)0;
	if ((sk = nl_socket_alloc()) == NULL) {
		throw eNetLinkFailed("cnetlink::add_addr_in4() nl_socket_alloc()");
	}

	int sd = 0;

	if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::add_addr_in4() nl_connect()");
	}

	struct rtnl_addr* addr = (struct rtnl_addr*)0;
	if ((addr = rtnl_addr_alloc()) == NULL) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::add_addr_in4() rtnl_addr_alloc()");
	}

	struct nl_addr* local = (struct nl_addr*)0;
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



void
cnetlink::drop_addr_in4(int ifindex, const rofl::caddress_in4& laddr, int prefixlen)
{
	int rc = 0;

	struct nl_sock *sk = (struct nl_sock*)0;
	if ((sk = nl_socket_alloc()) == NULL) {
		throw eNetLinkFailed("cnetlink::drop_addr_in4() nl_socket_alloc()");
	}

	int sd = 0;

	if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::drop_addr_in4() nl_connect()");
	}

	struct rtnl_addr* addr = (struct rtnl_addr*)0;
	if ((addr = rtnl_addr_alloc()) == NULL) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::drop_addr_in4() rtnl_addr_alloc()");
	}

	struct nl_addr* local = (struct nl_addr*)0;
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



void
cnetlink::add_addr_in6(int ifindex, const rofl::caddress_in6& laddr, int prefixlen)
{
	int rc = 0;

	struct nl_sock *sk = (struct nl_sock*)0;
	if ((sk = nl_socket_alloc()) == NULL) {
		throw eNetLinkFailed("cnetlink::add_addr_in6() nl_socket_alloc()");
	}

	int sd = 0;

	if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::add_addr_in6() nl_connect()");
	}

	struct rtnl_addr* addr = (struct rtnl_addr*)0;
	if ((addr = rtnl_addr_alloc()) == NULL) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::add_addr_in6() rtnl_addr_alloc()");
	}

	struct nl_addr* local = (struct nl_addr*)0;
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



void
cnetlink::drop_addr_in6(int ifindex, const rofl::caddress_in6& laddr, int prefixlen)
{
	int rc = 0;

	struct nl_sock *sk = (struct nl_sock*)0;
	if ((sk = nl_socket_alloc()) == NULL) {
		throw eNetLinkFailed("cnetlink::drop_addr_in6() nl_socket_alloc()");
	}

	int sd = 0;

	if ((sd = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::drop_addr_in6() nl_connect()");
	}

	struct rtnl_addr* addr = (struct rtnl_addr*)0;
	if ((addr = rtnl_addr_alloc()) == NULL) {
		nl_socket_free(sk);
		throw eNetLinkFailed("cnetlink::drop_addr_in6() rtnl_addr_alloc()");
	}

	struct nl_addr* local = (struct nl_addr*)0;
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


