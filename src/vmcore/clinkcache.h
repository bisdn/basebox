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
#include <libnl3/netlink/route/link.h>
#ifdef __cplusplus
}
#endif

#include <exception>

#include <rofl/common/ciosrv.h>

namespace dptmap
{

class eLinkCacheBase 		: public std::exception {};
class eLinkCacheCritical	: public eLinkCacheBase {};


class clinkcache_subscriber
{
public:
	virtual ~clinkcache_subscriber() {};
	virtual void linkcache_updated() = 0;
};


class clinkcache :
		public rofl::ciosrv
{

	struct nl_cache_mngr *mngr;
	struct nl_cache *cache;
	std::set<clinkcache_subscriber*> subscribers;


public:


	/**
	 *
	 */
	static clinkcache&
	get_instance();


	/**
	 *
	 */
	void
	subscribe(
			clinkcache_subscriber* subscriber);


	/**
	 *
	 */
	void
	unsubscribe(
			clinkcache_subscriber* subscriber);


	/**
	 *
	 */
	void
	get_link(
			std::string const& devname);


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


	/**
	 *
	 */
	void
	handle_revent(int fd);
};

}; // end of namespace dptmap

#endif /* CLINKCACHE_H_ */
