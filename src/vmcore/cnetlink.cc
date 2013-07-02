/*
 * clinkcache.cc
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#include <cnetlink.h>

using namespace dptmap;

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
		cnetlink::get_instance().set_link(crtlink((struct rtnl_link*)obj));

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
	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "oops, route_addr_cb() was called with an invalid link\n");
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

	fprintf(stderr, "cnetlink::route_route_cb() called\n");

	try {
		switch (action) {
		case NL_ACT_NEW: {
			unsigned int rtindex = cnetlink::get_instance().set_route(crtroute((struct rtnl_route*)obj));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->route_created(rtindex);
			}

		} break;
		case NL_ACT_CHANGE: {
			unsigned int rtindex = cnetlink::get_instance().set_route(crtroute((struct rtnl_route*)obj));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->route_updated(rtindex);
			}

		} break;
		case NL_ACT_DEL: {
			unsigned int rtindex = cnetlink::get_instance().get_route(crtroute((struct rtnl_route*)obj));

			for (std::set<cnetlink_subscriber*>::iterator
					it = cnetlink::get_instance().subscribers.begin(); it != cnetlink::get_instance().subscribers.end(); ++it) {
				(*it)->route_deleted(rtindex);
			}

			cnetlink::get_instance().del_route(rtindex);

		} break;
		default: {
			fprintf(stderr, "route/addr: unknown NL action\n");
		}
		}
	} catch (eRtLinkNotFound& e) {
		fprintf(stderr, "oops, route_addr_cb() was called with an invalid link\n");
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
		unsigned int rtindex)
{
	if (routes.find(rtindex) == routes.end())
		throw eNetLinkNotFound();
	return routes[rtindex];
}



unsigned int
cnetlink::get_route(crtroute const& rtr)
{
	std::map<unsigned int, crtroute>::iterator it;
	if ((it = find_if(routes.begin(), routes.end(),
			crtroute::crtroute_find(rtr.get_table_id(), rtr.get_scope(), rtr.get_ifindex(), rtr.get_dst()))) == routes.end()) {
		throw eRtLinkNotFound();
	}
	return it->first;
}


unsigned int
cnetlink::set_route(
		crtroute const& rtr)
{
	// route already exists
	std::map<unsigned int, crtroute>::iterator it;
	if ((it = find_if(routes.begin(), routes.end(),
			crtroute::crtroute_find(rtr.get_table_id(), rtr.get_scope(), rtr.get_ifindex(), rtr.get_dst()))) != routes.end()) {

		std::cerr << "cnetlink::set_route() update route => " << it->second << std::endl;

		it->second = rtr;
		return it->first;

	} else {

		unsigned int rtindex = 0;
		while (routes.find(rtindex) != routes.end()) {
			rtindex++;
		}

		std::cerr << "cnetlink::set_route() new route => " << rtr << std::endl;

		routes[rtindex] = rtr;
		return rtindex;
	}
}



void
cnetlink::del_route(
		unsigned int rtindex)
{
	if (routes.find(rtindex) == routes.end())
		throw eNetLinkNotFound();
	routes.erase(rtindex);
}


