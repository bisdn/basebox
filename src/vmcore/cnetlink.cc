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
		throw eLinkCacheCritical();
	}

	rc = nl_cache_mngr_add(mngr, "route/link", (change_func_t)&route_link_cb, NULL, &caches[NL_LINK_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/addr", (change_func_t)&route_addr_cb, NULL, &caches[NL_ADDR_CACHE]);


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
#if 0
		/*
		 * debug output
		 */
		rofl::cmemory mem(2048);
		struct nl_dump_params params;
		memset(&params, 0, sizeof(params));
		//params.dp_type = NL_DUMP_DETAILS;
		params.dp_buf = (char*)mem.somem();
		params.dp_buflen = mem.memlen();
		nl_cache_dump(netlink, &params);
		std::string s_str((const char*)mem.somem(), mem.memlen());
		fprintf(stderr, "XXX: %s\n", s_str.c_str());
#endif
		for (std::set<cnetlink_subscriber*>::iterator
				it = subscribers.begin(); it != subscribers.end(); ++it) {
			(*it)->linkcache_updated();
		}
	}
}



void
cnetlink::route_link_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
	if (std::string(nl_object_get_type(obj)) != std::string("route/link")) {
		fprintf(stderr, "cnetlink::route_link_cb() ignoring non link object received\n");
		return;
	}

	nl_object_get(obj); // get reference to object

	switch (action) {
	case NL_ACT_NEW: {
		fprintf(stderr, "route/link: NL-ACT-NEW\n");
	} break;
	case NL_ACT_CHANGE: {
		fprintf(stderr, "route/link: NL-ACT-CHANGE\n");
	} break;
	case NL_ACT_DEL: {
		fprintf(stderr, "route/link: NL-ACT-DEL\n");
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

	switch (action) {
	case NL_ACT_NEW: {
		fprintf(stderr, "route/addr: NL-ACT-NEW\n");
	} break;
	case NL_ACT_CHANGE: {
		fprintf(stderr, "route/addr: NL-ACT-CHANGE\n");
	} break;
	case NL_ACT_DEL: {
		fprintf(stderr, "route/addr: NL-ACT-DEL\n");
	} break;
	default: {
		fprintf(stderr, "route/addr: unknown NL action\n");
	}
	}

	nl_object_put(obj); // release reference to object
}




void
cnetlink::get_link(std::string const& devname)
{
	struct rtnl_link *link = (struct rtnl_link*)0;

	link = rtnl_link_get_by_name(caches[NL_LINK_CACHE], devname.c_str());
}



void
cnetlink::get_addr(std::string const& devname)
{
	struct rtnl_link *link = (struct rtnl_link*)0;

	link = rtnl_link_get_by_name(caches[NL_LINK_CACHE], devname.c_str());

	int ifindex = rtnl_link_get_ifindex(link);


	struct rtnl_addr *addr = (struct rtnl_addr*)0;

	addr = rtnl_addr_get(caches[NL_ADDR_CACHE], ifindex, 0);
}



