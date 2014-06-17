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

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/cache.h>
#ifdef __cplusplus
}
#endif

#include <exception>

#include <rofl/common/ciosrv.h>

#include "crtlink.h"
#include "crtroute.h"

namespace rofcore {

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
	std::map<uint8_t, std::map<unsigned int, crtroute_in4> >	routes_in4;	// all routes in system => key1:table_id, key2:routeindex, value:crtroute instance
	std::map<uint8_t, std::map<unsigned int, crtroute_in6> >	routes_in6;	// all routes in system => key1:table_id, key2:routeindex, value:crtroute instance

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
	crtroute_in4&
	get_route_in4(
			uint8_t table_id,
			unsigned int rtindex);


	/**
	 *
	 * @param rtr
	 * @return
	 */
	unsigned int
	get_route_in4(
			crtroute_in4 const& rtr);


	/**
	 *
	 */
	unsigned int
	set_route_in4(
			crtroute_in4 const& rtr);


	/**
	 *
	 */
	void
	del_route_in4(
			uint8_t table_id,
			unsigned int rtindex);



	/**
	 *
	 */
	crtroute_in6&
	get_route_in6(
			uint8_t table_id,
			unsigned int rtindex);


	/**
	 *
	 * @param rtr
	 * @return
	 */
	unsigned int
	get_route_in6(
			crtroute_in6 const& rtr);


	/**
	 *
	 */
	unsigned int
	set_route_in6(
			crtroute_in6 const& rtr);


	/**
	 *
	 */
	void
	del_route_in6(
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
	virtual void addr_in4_created(unsigned int ifindex, uint16_t adindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void addr_in4_updated(unsigned int ifindex, uint16_t adindex) {};

	/**
	 *
	 * @param ifindex
	 */
	virtual void addr_in4_deleted(unsigned int ifindex, uint16_t adindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void addr_in6_created(unsigned int ifindex, uint16_t adindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void addr_in6_updated(unsigned int ifindex, uint16_t adindex) {};

	/**
	 *
	 * @param ifindex
	 */
	virtual void addr_in6_deleted(unsigned int ifindex, uint16_t adindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void route_in4_created(uint8_t table_id, unsigned int rtindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void route_in4_updated(uint8_t table_id, unsigned int rtindex) {};


	/**
	 *
	 * @param ifindex
	 */
	virtual void route_in4_deleted(uint8_t table_id, unsigned int rtindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void route_in6_created(uint8_t table_id, unsigned int rtindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void route_in6_updated(uint8_t table_id, unsigned int rtindex) {};

	/**
	 *
	 * @param ifindex
	 */
	virtual void route_in6_deleted(uint8_t table_id, unsigned int rtindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void neigh_in4_created(unsigned int ifindex, uint16_t nbindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void neigh_in4_updated(unsigned int ifindex, uint16_t nbindex) {};

	/**
	 *
	 * @param ifindex
	 */
	virtual void neigh_in4_deleted(unsigned int ifindex, uint16_t nbindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void neigh_in6_created(unsigned int ifindex, uint16_t nbindex) {};

	/**
	 *
	 * @param rtl
	 */
	virtual void neigh_in6_updated(unsigned int ifindex, uint16_t nbindex) {};

	/**
	 *
	 * @param ifindex
	 */
	virtual void neigh_in6_deleted(unsigned int ifindex, uint16_t nbindex) {};
};




}; // end of namespace dptmap

#endif /* CLINKCACHE_H_ */
