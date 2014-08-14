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

#include "crtnexthops.h"


namespace rofcore {

class crtroute {
public:

	class eRtRouteBase		: public std::runtime_error {
	public:
		eRtRouteBase(const std::string& __arg) : std::runtime_error(__arg) {};
	};
	class eRtRouteNotFound	: public eRtRouteBase {
	public:
		eRtRouteNotFound(const std::string& __arg) : eRtRouteBase(__arg) {};
	};
	class eRtRouteExists		: public eRtRouteBase {
	public:
		eRtRouteExists(const std::string& __arg) : eRtRouteBase(__arg) {};
	};

public:

	/**
	 *
	 */
	crtroute() :
		table_id(0),
		scope(0),
		tos(0),
		protocol(0),
		priority(0),
		family(0),
		prefixlen(0),
		type(0),
		flags(0),
		metric(0),
		iif(0) {};

	/**
	 *
	 */
	virtual
	~crtroute() {};

	/**
	 *
	 */
	crtroute(
			const crtroute& rtroute) { *this = rtroute; };

	/**
	 *
	 */
	crtroute&
	operator= (
			const crtroute& rtroute) {
		if (this == &rtroute)
			return *this;

		table_id 	= rtroute.table_id;
		scope		= rtroute.scope;
		tos			= rtroute.tos;
		protocol	= rtroute.protocol;
		priority	= rtroute.priority;
		family		= rtroute.family;
		prefixlen	= rtroute.prefixlen;
		type		= rtroute.type;
		flags		= rtroute.flags;
		metric		= rtroute.metric;
		iif			= rtroute.iif;

		return *this;
	};

	/**
	 *
	 */
	crtroute(struct rtnl_route *route) :
			table_id(0),
			scope(0),
			tos(0),
			protocol(0),
			priority(0),
			family(0),
			prefixlen(0),
			type(0),
			flags(0),
			metric(0),
			iif(0)
	{
		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		rtnl_route_get(route); // increment reference count by one

		table_id 	= rtnl_route_get_table(route);
		scope		= rtnl_route_get_scope(route);
		tos			= rtnl_route_get_tos(route);
		protocol	= rtnl_route_get_protocol(route);
		priority	= rtnl_route_get_priority(route);
		family		= rtnl_route_get_family(route);
		type 		= rtnl_route_get_type(route);
		flags		= rtnl_route_get_flags(route);
		metric		= rtnl_route_get_metric(route, 0, NULL); // FIXME: check the integer value
		iif		= rtnl_route_get_iif(route);
		prefixlen	= nl_addr_get_prefixlen(rtnl_route_get_dst(route));

		rtnl_route_put(route); // decrement reference count by one
	}


	/**
	 *
	 */
	bool
	operator== (
			const crtroute& rtroute) {
		// FIXME: anything else beyond this?
		return ((table_id 		== rtroute.table_id) &&
				(scope 			== rtroute.scope) &&
				(iif			== rtroute.iif));
	};



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
	get_table_id_s() const {
		std::string str;

		switch (table_id) {
		/*255*/case RT_TABLE_LOCAL:		str = std::string("local");		break;
		/*254*/case RT_TABLE_MAIN:		str = std::string("main");		break;
		/*253*/case RT_TABLE_DEFAULT:	str = std::string("default");	break;
		/*252*/case RT_TABLE_COMPAT:	str = std::string("compat");	break;
		default:						str = std::string("unknown");	break;
		}

		return str;
	};




	/**
	 *
	 */
	uint8_t
	get_scope() const { return scope; };

	/**
	 *
	 */
	std::string
	get_scope_s() const {
		std::string str;

		switch (scope) {
		/*255*/case RT_SCOPE_NOWHERE:	str = std::string("nowhere");	break;
		/*254*/case RT_SCOPE_HOST:		str = std::string("host");		break;
		/*253*/case RT_SCOPE_LINK:		str = std::string("link");		break;
		/*200*/case RT_SCOPE_SITE:		str = std::string("site");		break;
		/*000*/case RT_SCOPE_UNIVERSE:	str = std::string("universe");	break;
		default:						str = std::string("unknown");	break;
		}

		return str;
	};



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
	unsigned int
	get_metric() const { return metric; };

	/**
	 *
	 */
	int
	get_iif() const { return iif; };



public:

	friend std::ostream&
	operator<< (std::ostream& os, crtroute const& route) {
		os << rofcore::indent(0) << "<crtroute: >" << std::endl;
		os << rofcore::indent(2) << "<table_id: " 	<< route.get_table_id_s() 		<< " >" << std::endl;
		os << rofcore::indent(2) << "<scope: " 	<< route.get_scope_s() 			<< " >" << std::endl;
		os << rofcore::indent(2) << "<tos: " 		<< (unsigned int)route.tos 		<< " >" << std::endl;
		os << rofcore::indent(2) << "<protocol: " 	<< (unsigned int)route.protocol << " >" << std::endl;
		os << rofcore::indent(2) << "<priority: " 	<< (unsigned int)route.priority << " >" << std::endl;
		os << rofcore::indent(2) << "<family: " 	<< (unsigned int)route.family 	<< " >" << std::endl;
		os << rofcore::indent(2) << "<prefixlen: " << route.prefixlen 				<< " >" << std::endl;
		os << rofcore::indent(2) << "<type: " 		<< (unsigned int)route.type 	<< " >" << std::endl;
		os << rofcore::indent(2) << "<flags: " 	<< (unsigned int)route.flags 	<< " >" << std::endl;
		os << rofcore::indent(2) << "<metric: " 	<< (unsigned int)route.metric 			<< " >" << std::endl;
		os << rofcore::indent(2) << "<ifindex: " 	<< (unsigned int)route.iif 		<< " >" << std::endl;

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
	unsigned int			metric;
	unsigned int			iif;
};





/**
 *
 */
class crtroute_find : public std::unary_function<crtroute,bool> {
	crtroute rtroute;
public:
	crtroute_find(const crtroute& rtroute) :
		rtroute(rtroute) {};
	bool operator() (const crtroute& rta) {
		return (rtroute == rta);
	};
	bool operator() (const std::pair<unsigned int, crtroute>& p) {
		return (rtroute == p.second);
	};
	bool operator() (const std::pair<unsigned int, crtroute*>& p) {
		return (rtroute == *(p.second));
	};
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
			const crtroute_in4& rtroute) {
		if (this == &rtroute)
			return *this;
		crtroute::operator= (rtroute);
		dst 		= rtroute.dst;
		mask		= rtroute.mask;
		src			= rtroute.src;
		pref_src	= rtroute.pref_src;
		nxthops		= rtroute.nxthops;
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
		if (s_dst != "none") {
			s_dst		= s_dst.substr(0, s_dst.find_first_of("/", 0));
			dst 		= rofl::caddress_in4(s_dst);
		}

		std::string s_src(nl_addr2str(rtnl_route_get_src(route), s_buf, sizeof(s_buf)));
		if (s_src != "none") {
			s_src	 	= s_src.substr(0,  s_src.find_first_of("/", 0));
			src 		= rofl::caddress_in4(s_src);
		}

		std::string s_pref_src(nl_addr2str(rtnl_route_get_pref_src(route), s_buf, sizeof(s_buf)));
		if (s_pref_src != "none") {
			s_pref_src 	= s_pref_src.substr(0, s_pref_src.find_first_of("/", 0));
			pref_src	= rofl::caddress_in4(s_pref_src);
		}

		for (int i = 0; i < rtnl_route_get_nnexthops(route); i++) {
			nxthops.add_nexthop(crtnexthop_in4(route, rtnl_route_nexthop_n(route, i)));
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
	const crtnexthops_in4&
	get_nexthops_in4() const { return nxthops; };

	/**
	 *
	 */
	crtnexthops_in4&
	set_nexthops_in4() { return nxthops; };

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

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtroute_in4& route) {
		os << rofcore::indent(0) << "<crtroute_in4 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const crtroute&>( route ); }
		os << rofcore::indent(2) << "<dst: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv4_dst();
		os << rofcore::indent(2) << "<mask: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv4_mask();
		os << rofcore::indent(2) << "<src: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv4_src();
		os << rofcore::indent(2) << "<pref-src: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv4_pref_src();
		//{ rofcore::indent i(2); os << route.get_nexthops_in4(); };

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

	crtnexthops_in4			nxthops;
};




/**
 *
 */
class crtroute_in4_find : public std::unary_function<crtroute_in4,bool> {
	crtroute_in4 rtroute;
public:
	crtroute_in4_find(const crtroute_in4& rtroute) :
		rtroute(rtroute) {};
	bool operator() (const crtroute_in4& rta) {
		return (rtroute == rta);
	};
	bool operator() (const std::pair<unsigned int, crtroute_in4>& p) {
		return (rtroute == p.second);
	};
#if 0
	bool operator() (const std::pair<unsigned int, crtroute_in4*>& p) {
		return (rtroute == *(p.second));
	};
#endif
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
			const crtroute_in6& rtroute) {
		if (this == &rtroute)
			return *this;
		crtroute::operator= (rtroute);
		dst 		= rtroute.dst;
		mask		= rtroute.mask;
		src			= rtroute.src;
		pref_src	= rtroute.pref_src;
		nxthops		= rtroute.nxthops;
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
		if (s_dst != "none") {
			s_dst 		= s_dst.substr(0, s_dst.find_first_of("/", 0));
			dst 		= rofl::caddress_in6(s_dst);
		}

		std::string s_src(nl_addr2str(rtnl_route_get_src(route), s_buf, sizeof(s_buf)));
		if (s_src != "none") {
			s_src  		= s_src.substr(0,  s_src.find_first_of("/", 0));
			src 		= rofl::caddress_in6(s_src);
		}

		std::string s_pref_src(nl_addr2str(rtnl_route_get_pref_src(route), s_buf, sizeof(s_buf)));
		if (s_pref_src != "none") {
			s_pref_src 	= s_pref_src.substr(0, s_pref_src.find_first_of("/", 0));
			pref_src	= rofl::caddress_in6(s_pref_src);
		}

		for (int i = 0; i < rtnl_route_get_nnexthops(route); i++) {
			nxthops.add_nexthop(crtnexthop_in6(route, rtnl_route_nexthop_n(route, i)));
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
	const crtnexthops_in6&
	get_nexthops_in6() const { return nxthops; };

	/**
	 *
	 */
	crtnexthops_in6&
	set_nexthops_in6() { return nxthops; };

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

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtroute_in6& route) {
		os << rofcore::indent(0) << "<crtroute_in6 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const crtroute&>( route ); }
		os << rofcore::indent(2) << "<dst: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv6_dst();
		os << rofcore::indent(2) << "<mask: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv6_mask();
		os << rofcore::indent(2) << "<src: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv6_src();
		os << rofcore::indent(2) << "<pref-src: >" << std::endl;
		os << rofcore::indent(4) << route.get_ipv6_pref_src();
		//{ rofcore::indent i(2); os << route.get_nexthops_in6(); };

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

	crtnexthops_in6			nxthops;
};



/**
 *
 */
class crtroute_in6_find : public std::unary_function<crtroute_in6,bool> {
	crtroute_in6 rtroute;
public:
	crtroute_in6_find(const crtroute_in6& rtroute) :
		rtroute(rtroute) {};
	bool operator() (const crtroute_in6& rta) {
		return (rtroute == rta);
	};
	bool operator() (const std::pair<unsigned int, crtroute_in6>& p) {
		return (rtroute == p.second);
	};
#if 0
	bool operator() (const std::pair<unsigned int, crtroute_in6*>& p) {
		return (rtroute == *(p.second));
	};
#endif
};


}; // end of namespace

#endif /* CRTROUTE_H_ */
