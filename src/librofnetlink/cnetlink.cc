/*
 * clinkcache.cc
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#include "cnetlink.h"

using namespace rofcore;

cnetlink* cnetlink::netlink = (cnetlink*)0;



cnetlink::cnetlink() :
		mngr(0)
{
	try {
	init_caches();
	} catch (...) {
		std::cerr << "UUU" << std::endl;
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
		rofcore::logging::debug << "clinkcache::clinkcache() failed to allocate netlink cache manager" << std::endl;
		throw eNetLinkCritical();
	}

	rc = nl_cache_mngr_add(mngr, "route/link", (change_func_t)&route_link_cb, NULL, &caches[NL_LINK_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/addr", (change_func_t)&route_addr_cb, NULL, &caches[NL_ADDR_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/route", (change_func_t)&route_route_cb, NULL, &caches[NL_ROUTE_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/neigh", (change_func_t)&route_neigh_cb, NULL, &caches[NL_NEIGH_CACHE]);

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
cnetlink::handle_revent(int fd)
{
	if (fd == nl_cache_mngr_get_fd(mngr)) {
		nl_cache_mngr_data_ready(mngr);
	}
}



/* static C-callback */
void
cnetlink::route_link_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	try {
		if (std::string(nl_object_get_type(obj)) != std::string("route/link")) {
			rofcore::logging::debug << "cnetlink::route_link_cb() ignoring non link object received" << std::endl;
			return;
		}

		unsigned int ifindex = rtnl_link_get_ifindex((struct rtnl_link*)obj);

		nl_object_get(obj); // get reference to object

		crtlink rtlink((struct rtnl_link*)obj);

		switch (action) {
		case NL_ACT_NEW: {
			cnetlink::get_instance().set_links().add_link(rtlink);
			cnetlink::get_instance().notify_link_created(ifindex);
		} break;
		case NL_ACT_CHANGE: {
			cnetlink::get_instance().set_links().set_link(rtlink);
			cnetlink::get_instance().notify_link_updated(ifindex);
		} break;
		case NL_ACT_DEL: {
			//notify_link_deleted(ifindex);
			cnetlink::get_instance().set_links().drop_link(ifindex);
			cnetlink::get_instance().notify_link_deleted(ifindex);
		} break;
		default: {
			rofcore::logging::debug << "route/link: unknown NL action" << std::endl;
		}
		}

		nl_object_put(obj); // release reference to object

	} catch (eNetLinkNotFound& e) {
		// NL_ACT_CHANGE => ifindex not found
	}
}


/* static C-callback */
void
cnetlink::route_addr_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/addr")) {
		rofcore::logging::debug << "cnetlink::route_addr_cb() ignoring non addr object received" << std::endl;
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
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in4().add_addr(crtaddr_in4((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in4_created(ifindex, adindex);
			} break;
			case AF_INET6: {
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in6().add_addr(crtaddr_in6((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in6_created(ifindex, adindex);
			} break;
			}

		} break;
		case NL_ACT_CHANGE: {
			switch (family) {
			case AF_INET: {
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in4().set_addr(crtaddr_in4((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in4_updated(ifindex, adindex);
			} break;
			case AF_INET6: {
				unsigned int adindex = cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in6().set_addr(crtaddr_in6((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in6_updated(ifindex, adindex);
			} break;
			}

		} break;
		case NL_ACT_DEL: {
			switch (family) {
			case AF_INET: {
				unsigned int adindex = cnetlink::get_instance().set_links().get_link(ifindex).get_addrs_in4().get_addr(crtaddr_in4((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in4_deleted(ifindex, adindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in4().drop_addr(adindex);
			} break;
			case AF_INET6: {
				unsigned int adindex = cnetlink::get_instance().set_links().get_link(ifindex).get_addrs_in6().get_addr(crtaddr_in6((struct rtnl_addr*)obj));
				cnetlink::get_instance().notify_addr_in6_deleted(ifindex, adindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_addrs_in6().drop_addr(adindex);
			} break;
			}

		} break;
		default: {
			rofcore::logging::debug << "route/addr: unknown NL action" << std::endl;
		}
		}
	} catch (eNetLinkNotFound& e) {
		rofcore::logging::debug << "cnetlink::route_addr_cb() oops, route_addr_cb() was called with an invalid link [1]" << std::endl;
	} catch (crtaddr::eRtAddrNotFound& e) {
		rofcore::logging::debug << "cnetlink::route_addr_cb() oops, route_addr_cb() was called with an invalid address [2]" << std::endl;
	}

	nl_object_put(obj); // release reference to object
}


/* static C-callback */
void
cnetlink::route_route_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/route")) {
		rofcore::logging::debug << "cnetlink::route_route_cb() ignoring non route object received" << std::endl;
		return;
	}

	nl_object_get(obj); // get reference to object

	//rofcore::logging::debug << "cnetlink::route_route_cb() called" << std::endl;

	int family = rtnl_route_get_family((struct rtnl_route*)obj);
	int table_id = rtnl_route_get_table((struct rtnl_route*)obj);

	try {
		switch (action) {
		case NL_ACT_NEW: {
			switch (family) {
			case AF_INET: {
				unsigned int rtindex = cnetlink::get_instance().set_routes_in4(table_id).add_route(crtroute_in4((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in4_created(table_id, rtindex);
			} break;
			case AF_INET6: {
				unsigned int rtindex = cnetlink::get_instance().set_routes_in6(table_id).add_route(crtroute_in6((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in6_created(table_id, rtindex);
			} break;
			}
		} break;
		case NL_ACT_CHANGE: {
			switch (family) {
			case AF_INET: {
				unsigned int rtindex = cnetlink::get_instance().set_routes_in4(table_id).set_route(crtroute_in4((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in4_updated(table_id, rtindex);
			} break;
			case AF_INET6: {
				unsigned int rtindex = cnetlink::get_instance().set_routes_in6(table_id).set_route(crtroute_in6((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in6_updated(table_id, rtindex);
			} break;
			}
		} break;
		case NL_ACT_DEL: {
			switch (family) {
			case AF_INET: {
				unsigned int rtindex = cnetlink::get_instance().get_routes_in4(table_id).get_route(crtroute_in4((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in4_deleted(table_id, rtindex);
				cnetlink::get_instance().set_routes_in4(table_id).drop_route(rtindex);
			} break;
			case AF_INET6: {
				unsigned int rtindex = cnetlink::get_instance().get_routes_in6(table_id).get_route(crtroute_in6((struct rtnl_route*)obj));
				cnetlink::get_instance().notify_route_in6_deleted(table_id, rtindex);
				cnetlink::get_instance().set_routes_in6(table_id).drop_route(rtindex);
			} break;
			}
		} break;
		default: {
			rofcore::logging::debug << "route/route: unknown NL action" << std::endl;
		}
		}
	} catch (eNetLinkNotFound& e) {
		rofcore::logging::debug << "cnetlink::route_route_cb() oops, route_route_cb() was called with an invalid link" << std::endl;
	} catch (crtroute::eRtRouteNotFound& e) {
		rofcore::logging::debug << "cnetlink::route_route_cb() oops, route_route_cb() was called with an invalid route" << std::endl;
	}

	nl_object_put(obj); // release reference to object
}


/* static C-callback */
void
cnetlink::route_neigh_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/neigh")) {
		rofcore::logging::debug << "cnetlink::route_neigh_cb() ignoring non neighbor object received" << std::endl;
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
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in4().add_neigh(crtneigh_in4((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in4_created(ifindex, nbindex);
			} break;
			case AF_INET6: {
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in6().add_neigh(crtneigh_in6((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in6_created(ifindex, nbindex);
			} break;
			}
		} break;
		case NL_ACT_CHANGE: {
			switch (family) {
			case AF_INET: {
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in4().set_neigh(crtneigh_in4((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in4_updated(ifindex, nbindex);
			} break;
			case AF_INET6: {
				unsigned int nbindex = cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in6().set_neigh(crtneigh_in6((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in6_updated(ifindex, nbindex);
			} break;
			}
		} break;
		case NL_ACT_DEL: {
			switch (family) {
			case AF_INET: {
				unsigned int nbindex = cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in4().get_neigh(crtneigh_in4((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in4_deleted(ifindex, nbindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in4().drop_neigh(nbindex);
			} break;
			case AF_INET6: {
				unsigned int nbindex = cnetlink::get_instance().get_links().get_link(ifindex).get_neighs_in6().get_neigh(crtneigh_in6((struct rtnl_neigh*)obj));
				cnetlink::get_instance().notify_neigh_in6_deleted(ifindex, nbindex);
				cnetlink::get_instance().set_links().set_link(ifindex).set_neighs_in6().drop_neigh(nbindex);
			} break;
			}
		} break;
		default: {
			rofcore::logging::debug << "route/addr: unknown NL action" << std::endl;
		}
		}
	} catch (eNetLinkNotFound& e) {
		rofcore::logging::debug << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was called with an invalid link" << std::endl;
	} catch (crtneigh::eRtNeighNotFound& e) {
		rofcore::logging::debug << "cnetlink::route_neigh_cb() oops, route_neigh_cb() was called with an invalid neighbor" << std::endl;
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
cnetlink::notify_neigh_in4_created(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in4& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in4().get_neigh(nbindex).get_dst();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_in4_created(ifindex, nbindex);
	}

	std::cerr << "[cnetlink][notify_neigh_in4_created] sending notifications: TTT[0]" << std::endl;
	std::cerr << "<ifindex " << ifindex << " >" << std::endl;
	std::cerr << dst;

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_in4[ifindex][dst].begin();
				it != nbobservers_in4[ifindex][dst].end(); ++it) {

		std::cerr << "[cnetlink][notify_neigh_in4_created] sending notifications: TTT[1]" << std::endl;
		std::cerr << "<ifindex " << ifindex << " >" << std::endl;
		std::cerr << dst;

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
cnetlink::notify_neigh_in4_updated(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in4& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in4().get_neigh(nbindex).get_dst();

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
cnetlink::notify_neigh_in4_deleted(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in4& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in4().get_neigh(nbindex).get_dst();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_in4_deleted(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_in4[ifindex][dst].begin();
				it != nbobservers_in4[ifindex][dst].end(); ++it) {
		(*it)->neigh_in4_deleted(ifindex, nbindex);
	}
};



void
cnetlink::notify_neigh_in6_deleted(unsigned int ifindex, unsigned int nbindex) {
	const rofl::caddress_in6& dst = cnetlink::get_instance().get_links().
									get_link(ifindex).get_neighs_in6().get_neigh(nbindex).get_dst();

	for (std::set<cnetlink_common_observer*>::iterator
			it = cnetlink::get_instance().observers.begin();
				it != cnetlink::get_instance().observers.end(); ++it) {
		(*it)->neigh_in6_deleted(ifindex, nbindex);
	}

	for (std::set<cnetlink_neighbour_observer*>::iterator
			it = nbobservers_in6[ifindex][dst].begin();
				it != nbobservers_in6[ifindex][dst].end(); ++it) {
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



