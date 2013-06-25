/*
 * clinkcache.h
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#ifndef CLINKCACHE_H_
#define CLINKCACHE_H_ 1

#ifdef __cplusplus
extern "C" {
#endif
#include <libnl3/netlink/cache.h>
#ifdef __cplusplus
}
#endif

#include <exception>

#include <rofl/common/ciosrv.h>

namespace dptmap
{

class eLinkCacheBase 		: public std::exception {};
class eLinkCacheCritical	: public eLinkCacheBase {};

class clinkcache :
		public rofl::ciosrv
{

	struct nl_cache_mngr *mngr;
	struct nl_cache *cache;


public:


	/**
	 *
	 */
	static clinkcache&
	get_instance();


private:

	static clinkcache	*linkcache;

	/**
	 *
	 */
	clinkcache();


	/**
	 *
	 */
	clinkcache(clinkcache const& linkcache);


	/**
	 *
	 */
	virtual
	~clinkcache();
};

}; // end of namespace dptmap

#endif /* CLINKCACHE_H_ */
