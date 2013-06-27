/*
 * clinkcache.h
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#ifndef CNETLINK_H_
#define CNETLINK_H_ 1

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#ifdef __cplusplus
}
#endif

#include <exception>

#include <rofl/common/ciosrv.h>

namespace dptmap
{

class eLinkCacheBase 		: public std::exception {};
class eLinkCacheCritical	: public eLinkCacheBase {};


class cnetlink_subscriber
{
public:
	virtual ~cnetlink_subscriber() {};
	virtual void linkcache_updated() = 0;
};


class cnetlink :
		public rofl::ciosrv
{
	enum nl_cache_t {
		NL_LINK_CACHE = 0,
		NL_ADDR_CACHE = 1,
	};

	struct nl_cache_mngr *mngr;
	std::map<enum nl_cache_t, struct nl_cache*> caches;
	std::set<cnetlink_subscriber*> subscribers;


public:


	/**
	 *
	 */
	static void
	route_link_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data);


	/**
	 *
	 */
	static void
	route_addr_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data);


	/**
	 *
	 */
	static cnetlink&
	get_instance();


	/**
	 *
	 */
	void
	subscribe(
			cnetlink_subscriber* subscriber);


	/**
	 *
	 */
	void
	unsubscribe(
			cnetlink_subscriber* subscriber);


	/**
	 *
	 */
	void
	get_link(
			std::string const& devname);


	/**
	 *
	 */
	void
	get_addr(
			std::string const& devname);


private:

	static cnetlink	*netlink;

	/**
	 *
	 */
	cnetlink();


	/**
	 *
	 */
	cnetlink(cnetlink const& linkcache);


	/**
	 *
	 */
	virtual
	~cnetlink();


	/**
	 *
	 */
	void
	init_caches();


	/**
	 *
	 */
	void
	destroy_caches();


	/**
	 *
	 */
	void
	handle_revent(int fd);
};

}; // end of namespace dptmap

#endif /* CLINKCACHE_H_ */
