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
	mngr = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE);
	if (NULL == mngr) {
		fprintf(stderr, "clinkcache::clinkcache() failed to allocate netlink cache manager\n");
		throw eLinkCacheCritical();
	}

	cache = nl_cache_mngr_add(mngr, "route/link", NULL);
}



clinkcache::clinkcache(clinkcache const& linkcache) :
		mngr(0),
		cache(0)
{
	mngr = nl_cache_mngr_alloc(NULL, NETLINK_ROUTE, NL_AUTO_PROVIDE);
	if (NULL == mngr) {
		fprintf(stderr, "clinkcache::clinkcache() failed to allocate netlink cache manager\n");
		throw eLinkCacheCritical();
	}

	cache = nl_cache_mngr_add(mngr, "route/link", NULL);
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
