/*
 * crtroute.h
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#ifndef CRTROUTE_H_
#define CRTROUTE_H_ 1

#include <map>
#include <ostream>
#include <algorithm>
#include <exception>

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/object.h>
#include <netlink/route/route.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/logging.h>
#include <rofl/common/caddress.h>

#include "crtnexthop.h"


namespace rofcore {

class eRtRouteBase 			: public std::exception {};
class eRtRouteNotFound 		: public eRtRouteBase {};
class eRtRouteInval			: public eRtRouteBase {};


class crtroute {
public:

	/**
	 *
	 */
	crtroute();

	/**
	 *
	 */
	virtual
	~crtroute();

	/**
	 *
	 */
	crtroute(
			const crtroute& route);

	/**
	 *
	 */
	crtroute&
	operator= (
			const crtroute& route);

	/**
	 *
	 */
	crtroute(
			struct rtnl_route *route);

	/**
	 *
	 */
	bool
	operator== (
			const crtroute& route);


public:


	/**
	 *
	 */
	uint32_t
	get_table_id() const { return table_id; };


	/**
	 *
	 */
	std::string
	get_table_id_s() const;


	/**
	 *
	 */
	uint8_t
	get_scope() const { return scope; };

	/**
	 *
	 */
	std::string
	get_scope_s() const;

	/**
	 *
	 */
	uint8_t
	get_tos() const { return tos; };

	/**
	 *
	 */
	uint8_t
	get_protocol() const { return protocol; };

	/**
	 *
	 */
	uint32_t
	get_priority() const { return priority; };

	/**
	 *
	 */
	uint8_t
	get_family() const { return family; };

	/**
	 *
	 */
	unsigned int
	get_prefixlen() const { return prefixlen; };

	/**
	 *
	 */
	uint8_t
	get_type() const { return type; };

	/**
	 *
	 */
	uint32_t
	get_flags() const { return flags; };

	/**
	 *
	 */
	int
	get_metric() const { return metric; };

	/**
	 *
	 */
	int
	get_iif() const { return iif; };



public:

	friend std::ostream&
	operator<< (std::ostream& os, crtroute const& route) {
		os << rofl::indent(0) << "<crtroute: >" << std::endl;
		os << rofl::indent(2) << "<table_id: " 	<< route.get_table_id_s() 		<< " >" << std::endl;
		os << rofl::indent(2) << "<scope: " 	<< route.get_scope_s() 			<< " >" << std::endl;
		os << rofl::indent(2) << "<tos: " 		<< (unsigned int)route.tos 		<< " >" << std::endl;
		os << rofl::indent(2) << "<protocol: " 	<< (unsigned int)route.protocol << " >" << std::endl;
		os << rofl::indent(2) << "<priority: " 	<< (unsigned int)route.priority << " >" << std::endl;
		os << rofl::indent(2) << "<family: " 	<< (unsigned int)route.family 	<< " >" << std::endl;
		os << rofl::indent(2) << "<prefixlen: " << route.prefixlen 				<< " >" << std::endl;
		os << rofl::indent(2) << "<type: " 		<< (unsigned int)route.type 	<< " >" << std::endl;
		os << rofl::indent(2) << "<flags: " 	<< (unsigned int)route.flags 	<< " >" << std::endl;
		os << rofl::indent(2) << "<metric: " 	<< (int)route.metric 			<< " >" << std::endl;
		os << rofl::indent(2) << "<ifindex: " 	<< (unsigned int)route.iif 		<< " >" << std::endl;

		return os;
	};

private:

	uint32_t 				table_id;
	uint8_t					scope;
	uint8_t					tos;
	uint8_t					protocol;
	uint32_t				priority;
	uint8_t					family;
	unsigned int			prefixlen;
	uint8_t					type;
	uint32_t				flags;
	int						metric;
	unsigned int			iif;
};




class crtroute_in4 : public crtroute {
public:

	/**
	 *
	 */
	crtroute_in4() {};

	/**
	 *
	 */
	virtual
	~crtroute_in4() {};

	/**
	 *
	 */
	crtroute_in4(
			const crtroute_in4& route) { *this = route; };

	/**
	 *
	 */
	crtroute_in4&
	operator= (
			const crtroute_in4& route) {
		if (this == &route)
			return *this;
		crtroute::operator= (route);
		dst 		= route.dst;
		mask		= route.mask;
		src			= route.src;
		pref_src	= route.pref_src;
		return *this;
	}

	/**
	 *
	 */
	crtroute_in4(
			struct rtnl_route *route) :
				crtroute(route) {

		rtnl_route_get(route); // increment reference count by one

		mask.set_addr_hbo(~((1 << (32 - crtroute::get_prefixlen())) - 1));

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_dst(nl_addr2str(rtnl_route_get_dst(route), s_buf, sizeof(s_buf)));
		s_dst 		= s_dst.substr(0, s_dst.find_first_of("/", 0));
		dst 		= rofl::caddress_in4(s_dst);

		std::string s_src(nl_addr2str(rtnl_route_get_src(route), s_buf, sizeof(s_buf)));
		s_src  		= s_src.substr(0,  s_src.find_first_of("/", 0));
		src 		= rofl::caddress_in4(s_src);

		std::string s_pref_src(nl_addr2str(rtnl_route_get_pref_src(route), s_buf, sizeof(s_buf)));
		s_pref_src 	= s_pref_src.substr(0, s_pref_src.find_first_of("/", 0));
		pref_src	= rofl::caddress_in4(s_pref_src);

		for (int i = 0; i < rtnl_route_get_nnexthops(route); i++) {
			set_nexthop_in4(i) = crtnexthop_in4(route, rtnl_route_nexthop_n(route, i));
		}

		rtnl_route_put(route); // decrement reference count by one
	};

	/**
	 *
	 */
	bool
	operator== (
			const crtroute_in4& route) {
		return (crtroute::operator== (route) &&
				(dst == route.dst) &&
				(mask == route.mask) &&
				(src == route.src) &&
				(pref_src == route.pref_src));
	};

public:

	/**
	 *
	 */
	rofl::caddress_in4&
	set_ipv4_dst() { return dst; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_ipv4_dst() const { return dst; };

	/**
	 *
	 */
	rofl::caddress_in4&
	set_ipv4_mask() { return mask; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_ipv4_mask() const { return mask; };

	/**
	 *
	 */
	rofl::caddress_in4&
	set_ipv4_src() { return src; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_ipv4_src() const { return src; };

	/**
	 *
	 */
	rofl::caddress_in4&
	set_ipv4_pref_src() { return pref_src; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_ipv4_pref_src() const { return pref_src; };


	/**
	 *
	 */
	std::map<unsigned int, crtnexthop_in4>&
	get_nexthops() { return nexthops; };

	/**
	 *
	 */
	crtnexthop_in4&
	add_nexthop_in4(
			unsigned int nhindex);

	/**
	 *
	 */
	crtnexthop_in4&
	set_nexthop_in4(
			unsigned int nhindex);

	/**
	 *
	 */
	const crtnexthop_in4&
	get_nexthop_in4(
			unsigned int nhindex) const;

	/**
	 *
	 */
	void
	drop_nexthop_in4(
			unsigned int nhindex);

	/**
	 *
	 */
	bool
	has_nexthop_in4(
			unsigned int nhindex) const;

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtroute_in4& route) {
		os << dynamic_cast<const crtroute&>( route );
		os << rofl::indent(2) << "<dst: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv4_dst(); }
		os << rofl::indent(2) << "<mask: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv4_mask(); }
		os << rofl::indent(2) << "<src: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv4_src(); }
		os << rofl::indent(2) << "<pref-src: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv4_pref_src(); }

		rofl::indent i(2);
		for (std::map<unsigned int, crtnexthop_in4>::const_iterator
				it = route.nexthops.begin(); it != route.nexthops.end(); ++it) {
			os << (it->second);
		}

		return os;
	};


	/**
	 *
	 */
	class crtroute_in4_find {
		uint32_t 			table_id;
		uint8_t 			scope;
		unsigned int 		ifindex;
		rofl::caddress_in4 	dst;
	public:
		crtroute_in4_find(uint32_t table_id, uint8_t scope, unsigned int ifindex, const rofl::caddress_in4& dst) :
			table_id(table_id), scope(scope), ifindex(ifindex), dst(dst) {};
		bool operator() (const std::pair<unsigned int, crtroute_in4>& p) {
			return ((table_id 	== p.second.get_table_id()) &&
					(scope 		== p.second.get_scope()) &&
					(ifindex 	== p.second.get_iif()) &&
					(dst 		== p.second.get_ipv4_dst()));
		};
	};

private:

	rofl::caddress_in4		dst;
	rofl::caddress_in4		mask;
	rofl::caddress_in4		src;
	rofl::caddress_in4		pref_src;

	std::map<unsigned int, crtnexthop_in4>	nexthops;
};



class crtroute_in6 : public crtroute {
public:

	/**
	 *
	 */
	crtroute_in6() {};

	/**
	 *
	 */
	virtual
	~crtroute_in6() {};

	/**
	 *
	 */
	crtroute_in6(
			const crtroute_in6& route) { *this = route; };

	/**
	 *
	 */
	crtroute_in6&
	operator= (
			const crtroute_in6& route) {
		if (this == &route)
			return *this;
		crtroute::operator= (route);
		dst 		= route.dst;
		mask		= route.mask;
		src			= route.src;
		pref_src	= route.pref_src;
		return *this;
	}

	/**
	 *
	 */
	crtroute_in6(
			struct rtnl_route *route) :
				crtroute(route) {

		rtnl_route_get(route); // increment reference count by one

		int segment 	= crtroute::get_prefixlen() / 8;
		int t_prefixlen = crtroute::get_prefixlen() % 8;
		for (int i = 0; i < 16; i++) {
			if (segment == i) {
				mask[i] = ~((1 << (8 - t_prefixlen)) - 1);
			} else if (i > segment) {
				mask[i] = 0;
			} else if (i < segment) {
				mask[i] = 0xff;
			}
		}

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_dst(nl_addr2str(rtnl_route_get_dst(route), s_buf, sizeof(s_buf)));
		s_dst 		= s_dst.substr(0, s_dst.find_first_of("/", 0));
		dst 		= rofl::caddress_in6(s_dst);

		std::string s_src(nl_addr2str(rtnl_route_get_src(route), s_buf, sizeof(s_buf)));
		s_src  		= s_src.substr(0,  s_src.find_first_of("/", 0));
		src 		= rofl::caddress_in6(s_src);

		std::string s_pref_src(nl_addr2str(rtnl_route_get_pref_src(route), s_buf, sizeof(s_buf)));
		s_pref_src 	= s_pref_src.substr(0, s_pref_src.find_first_of("/", 0));
		pref_src	= rofl::caddress_in6(s_pref_src);

		for (int i = 0; i < rtnl_route_get_nnexthops(route); i++) {
			set_nexthop_in6(i) = crtnexthop_in6(route, rtnl_route_nexthop_n(route, i));
		}

		rtnl_route_put(route); // decrement reference count by one
	};

	/**
	 *
	 */
	bool
	operator== (
			const crtroute_in6& route) {
		return (crtroute::operator== (route) &&
				(dst == route.dst) &&
				(mask == route.mask) &&
				(src == route.src) &&
				(pref_src == route.pref_src));
	};

public:

	/**
	 *
	 */
	rofl::caddress_in6&
	set_ipv6_dst() { return dst; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_ipv6_dst() const { return dst; };

	/**
	 *
	 */
	rofl::caddress_in6&
	set_ipv6_mask() { return mask; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_ipv6_mask() const { return mask; };

	/**
	 *
	 */
	rofl::caddress_in6&
	set_ipv6_src() { return src; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_ipv6_src() const { return src; };

	/**
	 *
	 */
	rofl::caddress_in6&
	set_ipv6_pref_src() { return pref_src; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_ipv6_pref_src() const { return pref_src; };

	/**
	 *
	 */
	std::map<unsigned int, crtnexthop_in6>&
	get_nexthops() { return nexthops; };

	/**
	 *
	 */
	crtnexthop_in6&
	add_nexthop_in6(
			unsigned int nhindex);

	/**
	 *
	 */
	crtnexthop_in6&
	set_nexthop_in6(
			unsigned int nhindex);

	/**
	 *
	 */
	const crtnexthop_in6&
	get_nexthop_in6(
			unsigned int nhindex) const;

	/**
	 *
	 */
	void
	drop_nexthop_in6(
			unsigned int nhindex);

	/**
	 *
	 */
	bool
	has_nexthop_in6(
			unsigned int nhindex) const;

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtroute_in6& route) {
		os << dynamic_cast<const crtroute&>( route );
		os << rofl::indent(2) << "<dst: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv6_dst(); }
		os << rofl::indent(2) << "<mask: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv6_mask(); }
		os << rofl::indent(2) << "<src: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv6_src(); }
		os << rofl::indent(2) << "<pref-src: >" << std::endl;
		{ rofl::indent i(4); os << route.get_ipv6_pref_src(); }

		rofl::indent i(2);
		for (std::map<unsigned int, crtnexthop_in6>::const_iterator
				it = route.nexthops.begin(); it != route.nexthops.end(); ++it) {
			os << (it->second);
		}

		return os;
	};


	/**
	 *
	 */
	class crtroute_in6_find {
		uint32_t 			table_id;
		uint8_t 			scope;
		unsigned int 		ifindex;
		rofl::caddress_in 	dst;
	public:
		crtroute_in6_find(uint32_t table_id, uint8_t scope, unsigned int ifindex, const rofl::caddress_in6& dst) :
			table_id(table_id), scope(scope), ifindex(ifindex), dst(dst) {};
		bool operator() (const std::pair<unsigned int, crtroute_in6>& p) {
			return ((table_id 	== p.second.get_table_id()) &&
					(scope 		== p.second.get_scope()) &&
					(ifindex 	== p.second.get_iif()) &&
					(dst 		== p.second.get_ipv6_dst()));
		};
	};

private:

	rofl::caddress_in6		dst;
	rofl::caddress_in6		mask;
	rofl::caddress_in6		src;
	rofl::caddress_in6		pref_src;

	std::map<unsigned int, crtnexthop_in6>	nexthops;
};



}; // end of namespace

#endif /* CRTROUTE_H_ */
