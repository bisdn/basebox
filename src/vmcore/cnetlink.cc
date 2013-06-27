/*
 * clinkcache.cc
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#include <cnetlink.h>

using namespace dptmap;

cnlroute* cnlroute::linkcache = (cnlroute*)0;



cnlroute::cnlroute() :
		mngr(0)
{
	init_caches();
}



cnlroute::cnlroute(cnlroute const& linkcache) :
		mngr(0)
{
	init_caches();
}



cnlroute::~cnlroute()
{
	destroy_caches();
}



void
cnlroute::init_caches()
{
	int rc = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);
	if (rc < 0) {
		fprintf(stderr, "clinkcache::clinkcache() failed to allocate netlink cache manager\n");
		throw eLinkCacheCritical();
	}

	rc = nl_cache_mngr_add(mngr, "route/link", (change_func_t)&link_cache_cb, NULL, &caches[NL_LINK_CACHE]);

	rc = nl_cache_mngr_add(mngr, "route/addr", (change_func_t)&addr_cache_cb, NULL, &caches[NL_ADDR_CACHE]);


	register_filedesc_r(nl_cache_mngr_get_fd(mngr));
}



void
cnlroute::destroy_caches()
{
	nl_cache_mngr_free(mngr);
}





cnlroute&
cnlroute::get_instance()
{
	if (0 == cnlroute::linkcache) {
		cnlroute::linkcache = new cnlroute();
	}
	return *(cnlroute::linkcache);
}



void
cnlroute::subscribe(
		cnlroute_subscriber* subscriber)
{
	subscribers.insert(subscriber);
}



void
cnlroute::unsubscribe(
		cnlroute_subscriber* subscriber)
{
	subscribers.erase(subscriber);
}



void
cnlroute::handle_revent(int fd)
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
		nl_cache_dump(linkcache, &params);
		std::string s_str((const char*)mem.somem(), mem.memlen());
		fprintf(stderr, "XXX: %s\n", s_str.c_str());
#endif
		for (std::set<cnlroute_subscriber*>::iterator
				it = subscribers.begin(); it != subscribers.end(); ++it) {
			(*it)->linkcache_updated();
		}
	}
}



void
cnlroute::link_cache_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
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
}



void
cnlroute::addr_cache_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data)
{
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
}




void
cnlroute::get_link(std::string const& devname)
{
	struct rtnl_link *link = (struct rtnl_link*)0;

	link = rtnl_link_get_by_name(caches[NL_LINK_CACHE], devname.c_str());
}



void
cnlroute::get_addr(std::string const& devname)
{
	struct rtnl_link *link = (struct rtnl_link*)0;

	link = rtnl_link_get_by_name(caches[NL_LINK_CACHE], devname.c_str());

	int ifindex = rtnl_link_get_ifindex(link);


	struct rtnl_addr *addr = (struct rtnl_addr*)0;

	addr = rtnl_addr_get(caches[NL_ADDR_CACHE], ifindex, 0);
}



