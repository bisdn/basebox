/*
 * clinkcache.cc
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#include <clinkcache.h>

using namespace dptmap;

clinkcache* clinkcache::linkcache = (clinkcache*)0;

clinkcache::clinkcache() :
		mngr(0),
		cache(0)
{
	int rc = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);
	if (rc < 0) {
		fprintf(stderr, "clinkcache::clinkcache() failed to allocate netlink cache manager\n");
		throw eLinkCacheCritical();
	}

	rc = nl_cache_mngr_add(mngr, "route/link", NULL, NULL, &cache);

	register_filedesc_r(nl_cache_mngr_get_fd(mngr));
}



clinkcache::clinkcache(clinkcache const& linkcache) :
		mngr(0),
		cache(0)
{
	int rc = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE, &mngr);
	if (rc < 0) {
		fprintf(stderr, "clinkcache::clinkcache() failed to allocate netlink cache manager\n");
		throw eLinkCacheCritical();
	}

	rc = nl_cache_mngr_add(mngr, "route/link", NULL, NULL, &cache);
}



clinkcache::~clinkcache()
{
	nl_cache_mngr_free(mngr);
}



clinkcache&
clinkcache::get_instance()
{
	if (0 == clinkcache::linkcache) {
		clinkcache::linkcache = new clinkcache();
	}
	return *(clinkcache::linkcache);
}



void
clinkcache::subscribe(
		clinkcache_subscriber* subscriber)
{
	subscribers.insert(subscriber);
}



void
clinkcache::unsubscribe(
		clinkcache_subscriber* subscriber)
{
	subscribers.erase(subscriber);
}



void
clinkcache::handle_revent(int fd)
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
		nl_cache_dump(cache, &params);
		std::string s_str((const char*)mem.somem(), mem.memlen());
		fprintf(stderr, "XXX: %s\n", s_str.c_str());
#endif
		for (std::set<clinkcache_subscriber*>::iterator
				it = subscribers.begin(); it != subscribers.end(); ++it) {
			(*it)->linkcache_updated();
		}
	}
}



void
clinkcache::get_link(std::string const& devname)
{


	struct rtnl_link *link = (struct rtnl_link*)0;

	link = rtnl_link_get_by_name(cache, devname.c_str());
}


