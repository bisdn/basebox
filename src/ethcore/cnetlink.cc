/*
 * clinkcache.cc
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#include "cnetlink.h"

using namespace ethercore;

cnetlink* cnetlink::netlink = (cnetlink*)0;



cnetlink::cnetlink() :
		mngr(0)
{
	init_caches();
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
		fprintf(stderr, "clinkcache::clinkcache() failed to allocate netlink cache manager\n");
		throw eNetLinkCritical();
	}

	rc = nl_cache_mngr_add(mngr, "route/link", (change_func_t)&route_link_cb, NULL, &caches[NL_LINK_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/addr", (change_func_t)&route_addr_cb, NULL, &caches[NL_ADDR_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/route", (change_func_t)&route_route_cb, NULL, &caches[NL_ROUTE_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/neigh", (change_func_t)&route_neigh_cb, NULL, &caches[NL_NEIGH_CACHE]);

	struct nl_object* obj = nl_cache_get_first(caches[NL_LINK_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		set_link(crtlink((struct rtnl_link*)obj));
		nl_object_put(obj);
		obj = nl_cache_get_next(obj);
	}

	obj = nl_cache_get_first(caches[NL_ADDR_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		unsigned int ifindex = rtnl_addr_get_ifindex((struct rtnl_addr*)obj);
		get_link(ifindex).set_addr(crtaddr((struct rtnl_addr*)obj));
		nl_object_put(obj);
		obj = nl_cache_get_next(obj);
	}

	obj = nl_cache_get_first(caches[NL_ROUTE_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		set_route(crtroute((struct rtnl_route*)obj));
		nl_object_put(obj);
		obj = nl_cache_get_next(obj);
	}

	obj = nl_cache_get_first(caches[NL_NEIGH_CACHE]);
	while (0 != obj) {
		nl_object_get(obj);
		//set_route(crtroute((struct rtnl_neigh*)obj));
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
cnetlink::subscribe(
		cnetlink_subscriber* subscriber)
{
	subscribers.insert(subscriber);
}



void
cnetlink::unsubscribe(
		cnetlink_subscriber* subscriber)
{
	subscribers.erase(subscriber);
}



void
cnetlink::handle_revent(int fd)
{
	if (fd == nl_cache_mngr_get_fd(mngr)) {
		nl_cache_mngr_data_ready(mngr);
	}
}



void
cnetlink::route_link_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/link")) {
		fprintf(stderr, "cnetlink::route_link_cb() ignoring non link object received\n");
		return;
	}

	unsigned int ifindex = rtnl_link_get_ifindex((struct rtnl_link*)obj);

	nl_object_get(obj); // get reference to object

	switch (action) {
	case NL_ACT_NEW: {
		cnetlink::get_instance().set_link(crtlink((struct rtnl_link*)obj));

		for (std::set<cnetlink_subscriber*>::iterator
				it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
			(*it)->link_created(ifindex);
		}

	} break;
	case NL_ACT_CHANGE: {
		cnetlink::get_instance().get_link(ifindex);

		for (std::set<cnetlink_subscriber*>::iterator
				it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
			(*it)->link_updated(ifindex);
		}

	} break;
	case NL_ACT_DEL: {
		cnetlink::get_instance().del_link(ifindex);

		for (std::set<cnetlink_subscriber*>::iterator
				it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
			(*it)->link_deleted(ifindex);
		}

	} break;
	default: {
		fprintf(stderr, "route/link: unknown NL action\n");
	}
	}

	nl_object_put(obj); // release reference to object
}



void
cnetlink::route_addr_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/addr")) {
		fprintf(stderr, "cnetlink::route_addr_cb() ignoring non addr object received\n");
		return;
	}

	nl_object_get(obj); // get reference to object

	unsigned int ifindex = rtnl_addr_get_ifindex((struct rtnl_addr*)obj);

	try {
		switch (action) {
		case NL_ACT_NEW: {
			uint16_t adindex = cnetlink::get_instance().get_link(ifindex).set_addr(crtaddr((struct rtnl_addr*)obj));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->addr_created(ifindex, adindex);
			}

		} break;
		case NL_ACT_CHANGE: {
			uint16_t adindex = cnetlink::get_instance().get_link(ifindex).set_addr(crtaddr((struct rtnl_addr*)obj));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->addr_updated(ifindex, adindex);
			}

		} break;
		case NL_ACT_DEL: {
			uint16_t adindex = cnetlink::get_instance().get_link(ifindex).get_addr(crtaddr((struct rtnl_addr*)obj));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->addr_deleted(ifindex, adindex);
			}

			cnetlink::get_instance().get_link(ifindex).del_addr(adindex);

		} break;
		default: {
			fprintf(stderr, "route/addr: unknown NL action\n");
		}
		}
	} catch (eNetLinkNotFound& e) {
		fprintf(stderr, "cnetlink::route_addr_cb() oops, route_addr_cb() was called with an invalid link [1]\n");
	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "cnetlink::route_addr_cb() oops, route_addr_cb() was called with an invalid address [2]\n");
	}

	nl_object_put(obj); // release reference to object
}



void
cnetlink::route_route_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/route")) {
		fprintf(stderr, "cnetlink::route_route_cb() ignoring non route object received\n");
		return;
	}

	nl_object_get(obj); // get reference to object

	//fprintf(stderr, "cnetlink::route_route_cb() called\n");

	try {
		switch (action) {
		case NL_ACT_NEW: {
			crtroute rtr((struct rtnl_route*)obj);
			std::cerr << "NEW route_route_cb() => " << rtr << std::endl;
			unsigned int rtindex = cnetlink::get_instance().set_route(rtr);

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->route_created(rtr.get_table_id(), rtindex);
			}

		} break;
		case NL_ACT_CHANGE: {
			crtroute rtr((struct rtnl_route*)obj);
			std::cerr << "CHANGE route_route_cb() => " << rtr << std::endl;
			unsigned int rtindex = cnetlink::get_instance().set_route(rtr);

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->route_updated(rtr.get_table_id(), rtindex);
			}

		} break;
		case NL_ACT_DEL: {
			crtroute rtr((struct rtnl_route*)obj);
			std::cerr << "DELETE route_route_cb() => " << rtr << std::endl;
			unsigned int rtindex = cnetlink::get_instance().get_route(rtr);

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->route_deleted(rtr.get_table_id(), rtindex);
			}

			cnetlink::get_instance().del_route(rtr.get_table_id(), rtindex);

		} break;
		default: {
			fprintf(stderr, "route/route: unknown NL action\n");
		}
		}
	} catch (eNetLinkNotFound& e) {
		fprintf(stderr, "cnetlink::route_route_cb() oops, route_route_cb() was called with an invalid link\n");
	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "cnetlink::route_route_cb() oops, route_route_cb() was called with an invalid route\n");
	}

	nl_object_put(obj); // release reference to object
}



void
cnetlink::route_neigh_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/neigh")) {
		fprintf(stderr, "cnetlink::route_neigh_cb() ignoring non neighbor object received\n");
		return;
	}

	nl_object_get(obj); // get reference to object

	struct rtnl_neigh* neigh = (struct rtnl_neigh*)obj;

	int ifindex = rtnl_neigh_get_ifindex(neigh);

	try {
		switch (action) {
		case NL_ACT_NEW: {
			uint16_t nbindex = cnetlink::get_instance().get_link(ifindex).set_neigh(crtneigh(neigh));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->neigh_created(ifindex, nbindex);
			}

		} break;
		case NL_ACT_CHANGE: {
			uint16_t nbindex = cnetlink::get_instance().get_link(ifindex).set_neigh(crtneigh(neigh));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->neigh_updated(ifindex, nbindex);
			}

		} break;
		case NL_ACT_DEL: {
			uint16_t nbindex = cnetlink::get_instance().get_link(ifindex).get_neigh_index(crtneigh(neigh));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->neigh_deleted(ifindex, nbindex);
			}

			cnetlink::get_instance().get_link(ifindex).del_neigh(nbindex);

		} break;
		default: {
			fprintf(stderr, "route/addr: unknown NL action\n");
		}
		}
	} catch (eNetLinkNotFound& e) {
		fprintf(stderr, "cnetlink::route_neigh_cb() oops, route_neigh_cb() was called with an invalid link\n");
	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "cnetlink::route_neigh_cb() oops, route_neigh_cb() was called with an invalid neighbor\n");
	}

	nl_object_put(obj); // release reference to object
}



crtlink&
cnetlink::get_link(std::string const& devname)
{
	std::map<unsigned int, crtlink>::iterator it;
	if ((it = find_if(links.begin(), links.end(),
			crtlink::crtlink_find_by_devname(devname))) == links.end()) {
		throw eNetLinkNotFound();
	}
	return it->second;
}



crtlink&
cnetlink::get_link(unsigned int ifindex)
{
	std::map<unsigned int, crtlink>::iterator it;
	if ((it = find_if(links.begin(), links.end(),
			crtlink::crtlink_find_by_ifindex(ifindex))) == links.end()) {
		throw eNetLinkNotFound();
	}
	return it->second;
}



crtlink&
cnetlink::set_link(crtlink const& rtl)
{
	links[rtl.ifindex] = rtl;
	return links[rtl.ifindex];
}



void
cnetlink::del_link(
		unsigned int ifindex)
{
	links.erase(ifindex);
}



crtroute&
cnetlink::get_route(
		uint8_t table_id,
		unsigned int rtindex)
{
	if (routes[table_id].find(rtindex) == routes[table_id].end())
		throw eNetLinkNotFound();
	return routes[table_id][rtindex];
}



unsigned int
cnetlink::get_route(crtroute const& rtr)
{
	uint8_t table_id = rtr.get_table_id();
	std::map<unsigned int, crtroute>::iterator it;
	if ((it = find_if(routes[table_id].begin(), routes[table_id].end(),
			crtroute::crtroute_find(rtr.get_table_id(), rtr.get_scope(), rtr.get_iif(), rtr.get_dst()))) == routes[table_id].end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}


unsigned int
cnetlink::set_route(
		crtroute const& rtr)
{
	uint8_t table_id = rtr.get_table_id();
	// route already exists
	std::map<unsigned int, crtroute>::iterator it;
	if ((it = find_if(routes[table_id].begin(), routes[table_id].end(),
			crtroute::crtroute_find(rtr.get_table_id(), rtr.get_scope(), rtr.get_iif(), rtr.get_dst()))) != routes[table_id].end()) {

		//std::cerr << "cnetlink::set_route() update route => " << it->second << std::endl;

		it->second = rtr;
		return it->first;

	} else {

		unsigned int rtindex = 0;
		while (routes[table_id].find(rtindex) != routes[table_id].end()) {
			rtindex++;
		}

		//std::cerr << "cnetlink::set_route() new route => " << rtr << std::endl;

		routes[table_id][rtindex] = rtr;
		return rtindex;
	}
}



void
cnetlink::del_route(
		uint8_t table_id,
		unsigned int rtindex)
{
	if (routes[table_id].find(rtindex) == routes[table_id].end())
		throw eNetLinkNotFound();
	routes[table_id].erase(rtindex);
}


