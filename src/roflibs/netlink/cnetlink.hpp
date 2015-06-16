/*
 * clinkcache.h
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#ifndef CNETLINK_H_
#define CNETLINK_H_ 1

#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/route/neighbour.h>

#include <exception>

#include <rofl/common/ciosrv.h>

#include <roflibs/netlink/crtlinks.hpp>
#include <roflibs/netlink/crtroutes.hpp>
#include <roflibs/netlink/cprefix.hpp>
#include <roflibs/netlink/clogging.hpp>

namespace rofcore {

class eNetLinkBase 			: public std::runtime_error {
public:
	eNetLinkBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eNetLinkCritical		: public eNetLinkBase {
public:
	eNetLinkCritical(const std::string& __arg) : eNetLinkBase(__arg) {};
};
class eNetLinkNotFound		: public eNetLinkBase {
public:
	eNetLinkNotFound(const std::string& __arg) : eNetLinkBase(__arg) {};
};
class eNetLinkFailed		: public eNetLinkBase {
public:
	eNetLinkFailed(const std::string& __arg) : eNetLinkBase(__arg) {};
};

class cnetlink_common_observer;
class cnetlink_neighbour_observer;

class cnetlink :
		public rofl::ciosrv
{
	enum nl_cache_t {
		NL_LINK_CACHE = 0,
		NL_ADDR_CACHE = 1,
		NL_ROUTE_CACHE = 2,
		NL_NEIGH_CACHE = 3,
	};

	struct nl_cache_mngr*									mngr;
	std::map<enum nl_cache_t, struct nl_cache*> 			caches;
	std::set<cnetlink_common_observer*> 					observers;

	std::map<int, std::map<rofl::caddress_ll, std::set<cnetlink_neighbour_observer*> > > nbobservers_ll;
	std::map<int, std::map<rofl::caddress_in4, std::set<cnetlink_neighbour_observer*> > > nbobservers_in4;
	std::map<int, std::map<rofl::caddress_in6, std::set<cnetlink_neighbour_observer*> > > nbobservers_in6;

	crtlinks						rtlinks;		// all links in system => key:ifindex, value:crtlink instance
	std::map<int, crtroutes_in4>	rtroutes_in4;	// all routes in system => key:table_id
	std::map<int, crtroutes_in6>	rtroutes_in6;	// all routes in system => key:table_id

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cnetlink& netlink) {
		os << rofcore::indent(0) << "<cnetlink>" << std::endl;
		rofcore::indent i(2);
		os << netlink.rtlinks;
		for (std::map<int, crtroutes_in4>::const_iterator
				it = netlink.rtroutes_in4.begin(); it != netlink.rtroutes_in4.end(); ++it) {
			os << it->second;
		}
		for (std::map<int, crtroutes_in6>::const_iterator
				it = netlink.rtroutes_in6.begin(); it != netlink.rtroutes_in6.end(); ++it) {
			os << it->second;
		}
		return os;
	};

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
			cnetlink_common_observer* subscriber) {
		observers.insert(subscriber);
	};


	/**
	 *
	 */
	void
	unsubscribe(
			cnetlink_common_observer* subscriber) {
		observers.erase(subscriber);
	};

	/**
	 *
	 */
	void
	observe_neighbour(
			cnetlink_neighbour_observer* observer, int ifindex, const rofl::caddress_ll& dst) {
		nbobservers_ll[ifindex][dst].insert(observer);
	};

	/**
	 *
	 */
	void
	ignore_neighbour(
			cnetlink_neighbour_observer* observer, int ifindex, const rofl::caddress_ll& dst) {
		nbobservers_ll[ifindex][dst].erase(observer);
	};
	/**
	 *
	 */
	void
	observe_neighbour(
			cnetlink_neighbour_observer* observer, int ifindex, const rofl::caddress_in4& dst) {
		nbobservers_in4[ifindex][dst].insert(observer);
	};

	/**
	 *
	 */
	void
	ignore_neighbour(
			cnetlink_neighbour_observer* observer, int ifindex, const rofl::caddress_in4& dst) {
		nbobservers_in4[ifindex][dst].erase(observer);
	};

	/**
	 *
	 */
	void
	observe_neighbour(
			cnetlink_neighbour_observer* observer, int ifindex, const rofl::caddress_in6& dst) {
		nbobservers_in6[ifindex][dst].insert(observer);
	};

	/**
	 *
	 */
	void
	ignore_neighbour(
			cnetlink_neighbour_observer* observer, int ifindex, const rofl::caddress_in6& dst) {
		nbobservers_in6[ifindex][dst].erase(observer);
	};

	/**
	 *
	 */
	void
	ignore_neighbours(
			cnetlink_neighbour_observer* observer) {
		/* remove observer for all neigh_in4 instances */
		if (true) {
			std::map<int, std::map<rofl::caddress_in4, std::set<cnetlink_neighbour_observer*> > >::iterator it;
			for (it = nbobservers_in4.begin(); it != nbobservers_in4.end(); ++it) {
				std::map<rofl::caddress_in4, std::set<cnetlink_neighbour_observer*> >::iterator jt;
				for (jt = it->second.begin(); jt != it->second.end(); ++jt) {
					jt->second.erase(observer);
				}
			}
		}
		/* remove observer for all neigh_in6 instances */
		if (true) {
			std::map<int, std::map<rofl::caddress_in6, std::set<cnetlink_neighbour_observer*> > >::iterator it;
			for (it = nbobservers_in6.begin(); it != nbobservers_in6.end(); ++it) {
				std::map<rofl::caddress_in6, std::set<cnetlink_neighbour_observer*> >::iterator jt;
				for (jt = it->second.begin(); jt != it->second.end(); ++jt) {
					jt->second.erase(observer);
				}
			}
		}
	};

public:

	/**
	 *
	 */
	const crtlinks&
	get_links() const { return rtlinks; };

	/**
	 *
	 */
	crtlinks&
	set_links() { return rtlinks; };

	/**
	 *
	 */
	const crtroutes_in4&
	get_routes_in4(int table_id) const { return rtroutes_in4.at(table_id); };

	/**
	 *
	 */
	crtroutes_in4&
	set_routes_in4(int table_id) { return rtroutes_in4[table_id]; };

	/**
	 *
	 */
	const crtroutes_in6&
	get_routes_in6(int table_id) const { return rtroutes_in6.at(table_id); };

	/**
	 *
	 */
	crtroutes_in6&
	set_routes_in6(int table_id) { return rtroutes_in6[table_id]; };

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

	void
	update_link_cache();

	/**
	 *
	 */
	void
	handle_revent(int fd);

public:

	/**
	 *
	 */
	void
	notify_link_created(unsigned int ifindex);

	/**
	 *
	 */
	void
	notify_link_updated(unsigned int ifindex);

	/**
	 *
	 */
	void
	notify_link_deleted(unsigned int ifindex);

	/**
	 *
	 */
	void
	notify_addr_in4_created(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_addr_in6_created(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_addr_in4_updated(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_addr_in6_updated(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_addr_in4_deleted(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_addr_in6_deleted(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_neigh_ll_created(unsigned int ifindex, unsigned int adindex);

	/**
	 *
	 */
	void
	notify_neigh_in4_created(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_neigh_in6_created(unsigned int ifindex, unsigned int adindex);

	/**
	 *
	 */
	void
	notify_neigh_ll_updated(unsigned int ifindex, unsigned int adindex);

	/**
	 *
	 */
	void
	notify_neigh_in4_updated(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_neigh_in6_updated(unsigned int ifindex, unsigned int adindex);

	/**
	 *
	 */
	void
	notify_neigh_ll_deleted(unsigned int ifindex, unsigned int adindex);

	/**
	 *
	 */
	void
	notify_neigh_in4_deleted(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_neigh_in6_deleted(unsigned int ifindex, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_route_in4_created(uint8_t table_id, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_route_in6_created(uint8_t table_id, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_route_in4_updated(uint8_t table_id, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_route_in6_updated(uint8_t table_id, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_route_in4_deleted(uint8_t table_id, unsigned int adindex);


	/**
	 *
	 */
	void
	notify_route_in6_deleted(uint8_t table_id, unsigned int adindex);

public:

	void
	add_neigh_ll(int ifindex, const rofl::caddress_ll& addr);

	/**
	 *
	 */
	void
	add_addr_in4(int ifindex, const rofl::caddress_in4& local, int prefixlen);

	/**
	 *
	 */
	void
	drop_addr_in4(int ifindex, const rofl::caddress_in4& local, int prefixlen);

	/**
	 *
	 */
	void
	add_addr_in6(int ifindex, const rofl::caddress_in6& local, int prefixlen);

	/**
	 *
	 */
	void
	drop_addr_in6(int ifindex, const rofl::caddress_in6& local, int prefixlen);
};



class cnetlink_common_observer {
public:
	/**
	 *
	 */
	cnetlink_common_observer() {
		nl_subscribe();
	};

	/**
	 *
	 */
	virtual ~cnetlink_common_observer() {
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

	virtual void neigh_ll_created(unsigned int ifindex, uint16_t nbindex) {};

	virtual void neigh_ll_updated(unsigned int ifindex, uint16_t nbindex) {};

	virtual void neigh_ll_deleted(unsigned int ifindex, uint16_t nbindex) {};

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





class cnetlink_neighbour_observer {
public:
	/**
	 *
	 */
	cnetlink_neighbour_observer() {};

	/**
	 *
	 */
	virtual ~cnetlink_neighbour_observer() {
		cnetlink::get_instance().ignore_neighbours(this);
	};

	void watch(int ifindex, const rofl::caddress_ll& dst) {
		cnetlink::get_instance().observe_neighbour(this, ifindex, dst);
	}

	void unwatch(int ifindex, const rofl::caddress_ll& dst) {
		cnetlink::get_instance().ignore_neighbour(this, ifindex, dst);
	}

	/**
	 *
	 * @param ifindex
	 */
	void watch(int ifindex, const rofl::caddress_in4& dst) {
		cnetlink::get_instance().observe_neighbour(this, ifindex, dst);
	};

	/**
	 *
	 * @param ifindex
	 */
	void unwatch(int ifindex, const rofl::caddress_in4& dst) {
		cnetlink::get_instance().ignore_neighbour(this, ifindex, dst);
	};

	/**
	 *
	 * @param ifindex
	 */
	void watch(int ifindex, const rofl::caddress_in6& dst) {
		cnetlink::get_instance().observe_neighbour(this, ifindex, dst);
	};

	/**
	 *
	 * @param ifindex
	 */
	void unwatch(int ifindex, const rofl::caddress_in6& dst) {
		cnetlink::get_instance().ignore_neighbour(this, ifindex, dst);
	};

	virtual void neigh_ll_created(unsigned int ifindex, uint16_t nbindex) {};

	virtual void neigh_ll_updated(unsigned int ifindex, uint16_t nbindex) {};

	virtual void neigh_ll_deleted(unsigned int ifindex, uint16_t nbindex) {};

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
