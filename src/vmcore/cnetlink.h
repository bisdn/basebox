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
#include <netlink/route/neighbour.h>
#ifdef __cplusplus
}
#endif

#include <exception>

#include <rofl/common/ciosrv.h>

#include <crtlink.h>
#include <crtroute.h>

namespace dptmap
{

class eNetLinkBase 			: public std::exception {};
class eNetLinkCritical		: public eNetLinkBase {};
class eNetLinkNotFound		: public eNetLinkBase {};

class cnetlink_subscriber;

class cnetlink :
		public rofl::ciosrv
{
	enum nl_cache_t {
		NL_LINK_CACHE = 0,
		NL_ADDR_CACHE = 1,
		NL_ROUTE_CACHE = 2,
		NL_NEIGH_CACHE = 3,
	};

	struct nl_cache_mngr *mngr;
	std::map<enum nl_cache_t, struct nl_cache*> caches;
	std::set<cnetlink_subscriber*> subscribers;

	std::map<unsigned int, crtlink>		links;	// all links in system => key:ifindex, value:crtlink instance
	std::map<uint8_t, std::map<unsigned int, crtroute> >	routes;	// all routes in system => key1:table_id, key2:routeindex, value:crtroute instance

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
	static void
	route_route_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data);


	/**
	 *
	 */
	static void
	route_neigh_cb(struct nl_cache* cache, struct nl_object* obj, int action, void* data);


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
	crtlink&
	get_link(
			std::string const& devname);


	/**
	 *
	 */
	crtlink&
	get_link(
			unsigned int ifindex);


	/**
	 *
	 */
	crtlink&
	set_link(
			crtlink const& rtl);


	/**
	 *
	 */
	void
	del_link(
			unsigned int ifindex);


	/**
	 *
	 */
	crtroute&
	get_route(
			uint8_t table_id,
			unsigned int rtindex);


	/**
	 *
	 * @param rtr
	 * @return
	 */
	unsigned int
	get_route(
			crtroute const& rtr);


	/**
	 *
	 */
	unsigned int
	set_route(
			crtroute const& rtr);


	/**
	 *
	 */
	void
	del_route(
			uint8_t table_id,
			unsigned int rtindex);


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




class cnetlink_subscriber
{
public:
	/**
	 *
	 */
	cnetlink_subscriber() {
		nl_subscribe();
	};

	/**
	 *
	 */
	virtual ~cnetlink_subscriber() {
		nl_unsubscribe();
	};

	/**
	 *
	 * @param ifindex
	 */
	void nl_subscribe() {
		cnetlink::get_instance().subscribe(this);
	};

	/**
	 *
	 * @param ifindex
	 */
	void nl_unsubscribe() {
		cnetlink::get_instance().unsubscribe(this);
	};

	/**
	 *
	 * @param rtl
	 */
	virtual void link_created(unsigned int ifindex) {};


	/**
	 *
	 * @param rtl
	 */
	virtual void link_updated(unsigned int ifindex) {};


	/**
	 *
	 * @param ifindex
	 */
	virtual void link_deleted(unsigned int ifindex) {};


	/**
	 *
	 * @param rtl
	 */
	virtual void addr_created(unsigned int ifindex, uint16_t adindex) {};


	/**
	 *
	 * @param rtl
	 */
	virtual void addr_updated(unsigned int ifindex, uint16_t adindex) {};


	/**
	 *
	 * @param ifindex
	 */
	virtual void addr_deleted(unsigned int ifindex, uint16_t adindex) {};


	/**
	 *
	 * @param rtl
	 */
	virtual void route_created(uint8_t table_id, unsigned int rtindex) {};


	/**
	 *
	 * @param rtl
	 */
	virtual void route_updated(uint8_t table_id, unsigned int rtindex) {};


	/**
	 *
	 * @param ifindex
	 */
	virtual void route_deleted(uint8_t table_id, unsigned int rtindex) {};


	/**
	 *
	 * @param rtl
	 */
	virtual void neigh_created(unsigned int ifindex, uint16_t nbindex) {};


	/**
	 *
	 * @param rtl
	 */
	virtual void neigh_updated(unsigned int ifindex, uint16_t nbindex) {};


	/**
	 *
	 * @param ifindex
	 */
	virtual void neigh_deleted(unsigned int ifindex, uint16_t nbindex) {};
};




}; // end of namespace dptmap

#endif /* CLINKCACHE_H_ */
