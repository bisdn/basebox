/*
 * crtnexthop.h
 *
 *  Created on: 02.07.2013
 *      Author: andreas
 */

#ifndef CRTNEXTHOP_H_
#define CRTNEXTHOP_H_ 1

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#include <netlink/object.h>
#include <netlink/route/route.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/caddress.h>

#include "clogging.hpp"

namespace rofcore {

class crtnexthop {
public:

	class eRtNextHopBase		: public std::runtime_error {
	public:
		eRtNextHopBase(const std::string& __arg) : std::runtime_error(__arg) {};
	};
	class eRtNextHopNotFound	: public eRtNextHopBase {
	public:
		eRtNextHopNotFound(const std::string& __arg) : eRtNextHopBase(__arg) {};
	};
	class eRtNextHopExists		: public eRtNextHopBase {
	public:
		eRtNextHopExists(const std::string& __arg) : eRtNextHopBase(__arg) {};
	};

public:

	/**
	 *
	 */
	crtnexthop() :
		weight(0),
		ifindex(0),
		flags(0),
		realms(0) {};

	/**
	 *
	 */
	virtual
	~crtnexthop() {};

	/**
	 *
	 */
	crtnexthop(
			const crtnexthop& rtnxthop) { *this = rtnxthop; };

	/**
	 *
	 */
	crtnexthop&
	operator= (
			const crtnexthop& rtnxthop) {
		if (this == &rtnxthop)
			return *this;

		weight 	= rtnxthop.weight;
		ifindex	= rtnxthop.ifindex;
		flags	= rtnxthop.flags;
		realms	= rtnxthop.realms;

		return *this;
	};

	/**
	 *
	 */
	crtnexthop(
			struct rtnl_route *route,
			struct rtnl_nexthop *nxthop) :
				weight(0),
				ifindex(0),
				flags(0),
				realms(0)
	{
		rtnl_route_get(route);

		weight		= rtnl_route_nh_get_weight(nxthop);
		ifindex		= rtnl_route_nh_get_ifindex(nxthop);
		flags		= rtnl_route_nh_get_flags(nxthop);
		realms		= rtnl_route_nh_get_realms(nxthop);

		rtnl_route_put(route);
	};

	/**
	 *
	 */
	bool
	operator== (const crtnexthop& rtnexthop) {
		return ((weight 	== rtnexthop.weight) 	&&
				(ifindex 	== rtnexthop.ifindex)	&&
				(flags 		== rtnexthop.flags) 	&&
				(realms 	== rtnexthop.realms));
	};



public:

	/**
	 *
	 */
	uint8_t
	get_weight() const { return weight; };

	/**
	 *
	 */
	int
	get_ifindex() const { return ifindex; };

	/**
	 *
	 */
	unsigned int
	get_flags() const { return flags; };

	/**
	 *
	 */
	uint32_t
	get_realms() const { return realms; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, crtnexthop const& nxthop) {
		os << rofcore::indent(0) << "<crtnexthop: >" << std::endl;
		os << rofcore::indent(2) << "<weight: " 	<< (int)nxthop.weight 			<< " >" << std::endl;
		os << rofcore::indent(2) << "<ifindex: " 	<< (int)nxthop.ifindex 			<< " >" << std::endl;
		os << rofcore::indent(2) << "<flags: " 		<< nxthop.flags 				<< " >" << std::endl;
		os << rofcore::indent(2) << "<realms: " 	<< (unsigned int)nxthop.realms 	<< " >" << std::endl;
		return os;
	};

private:

	uint8_t			weight;
	int				ifindex;
	unsigned int	flags;
	uint32_t		realms;
};



/**
 *
 */
class crtnexthop_find : public std::unary_function<crtnexthop,bool> {
	crtnexthop rtnexthop;
public:
	crtnexthop_find(const crtnexthop& rtnexthop) :
		rtnexthop(rtnexthop) {};
	bool operator() (const crtnexthop& rtn) {
		return (rtnexthop == rtn);
	};
	bool operator() (const std::pair<unsigned int, crtnexthop>& p) {
		return (rtnexthop == p.second);
	};
	bool operator() (const std::pair<unsigned int, crtnexthop*>& p) {
		return (rtnexthop == *(p.second));
	};
};




class crtnexthop_in4 : public crtnexthop {
public:

	/**
	 *
	 */
	crtnexthop_in4() {};

	/**
	 *
	 */
	virtual
	~crtnexthop_in4() {};

	/**
	 *
	 */
	crtnexthop_in4(
			const crtnexthop_in4& nxthop) { *this = nxthop; };

	/**
	 *
	 */
	crtnexthop_in4&
	operator= (
			const crtnexthop_in4& nxthop) {
		if (this == &nxthop)
			return *this;
		crtnexthop::operator= (nxthop);
		gateway = nxthop.gateway;
		return *this;
	};

	/**
	 *
	 */
	crtnexthop_in4(
			struct rtnl_route *route,
			struct rtnl_nexthop *nxthop) : crtnexthop(route, nxthop) {

		rtnl_route_get(route);

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_gw(nl_addr2str(rtnl_route_nh_get_gateway(nxthop), s_buf, sizeof(s_buf)));
		if (s_gw != "none") {
			s_gw 		= s_gw.substr(0, s_gw.find_first_of("/", 0));
			gateway 	= rofl::caddress_in4(s_gw.c_str());
		}

		rtnl_route_put(route);
	};


	/**
	 *
	 */
	bool
	operator== (const crtnexthop_in4& rtnexthop) {
		return ((crtnexthop::operator== (rtnexthop)) && (gateway == rtnexthop.gateway));
	};


public:

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_gateway() const { return gateway; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, crtnexthop_in4 const& nxthop) {
		os << rofcore::indent(0) << "<crtnexthop_in4 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const crtnexthop&>( nxthop ); };
		os << rofcore::indent(4) << "<gateway: >" << std::endl;
		os << rofcore::indent(6) << nxthop.gateway;
		return os;
	};

	std::string
	str() const {
		std::stringstream ss;
		ss << "nexthop/inet " << gateway.str() << " weight " << (int)get_weight() << " ";
		return ss.str();
	};

private:

	rofl::caddress_in4	gateway;
};


/**
 *
 */
class crtnexthop_in4_find : public std::unary_function<crtnexthop_in4,bool> {
	crtnexthop_in4 rtnexthop;
public:
	crtnexthop_in4_find(const crtnexthop_in4& rtnexthop) :
		rtnexthop(rtnexthop) {};
	bool operator() (const crtnexthop_in4& rta) {
		return (rtnexthop == rta);
	};
	bool operator() (const std::pair<unsigned int, crtnexthop_in4>& p) {
		return (rtnexthop == p.second);
	};
#if 0
	bool operator() (const std::pair<unsigned int, crtnexthop_in4*>& p) {
		return (rtnexthop == *(p.second));
	};
#endif
};




class crtnexthop_in6 : public crtnexthop {
public:

	/**
	 *
	 */
	crtnexthop_in6() {};

	/**
	 *
	 */
	virtual
	~crtnexthop_in6() {};

	/**
	 *
	 */
	crtnexthop_in6(
			const crtnexthop_in6& nxthop) { *this = nxthop; };

	/**
	 *
	 */
	crtnexthop_in6&
	operator= (
			const crtnexthop_in6& nxthop) {
		if (this == &nxthop)
			return *this;
		crtnexthop::operator= (nxthop);
		gateway = nxthop.gateway;
		return *this;
	};

	/**
	 *
	 */
	crtnexthop_in6(
			struct rtnl_route *route,
			struct rtnl_nexthop *nxthop) : crtnexthop(route, nxthop) {

		rtnl_route_get(route);

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_gw(nl_addr2str(rtnl_route_nh_get_gateway(nxthop), s_buf, sizeof(s_buf)));
		if (s_gw != "none") {
			s_gw 		= s_gw.substr(0, s_gw.find_first_of("/", 0));
			gateway 	= rofl::caddress_in6(s_gw.c_str());
		}

		rtnl_route_put(route);
	};

	/**
	 *
	 */
	bool
	operator== (const crtnexthop_in6& rtnexthop) {
		return ((crtnexthop::operator== (rtnexthop)) && (gateway == rtnexthop.gateway));
	};


public:

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_gateway() const { return gateway; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, crtnexthop_in6 const& nxthop) {
		os << rofcore::indent(0) << "<crtnexthop_in6 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const crtnexthop&>( nxthop ); };
		os << rofcore::indent(4) << "<gateway: >" << std::endl;
		os << rofcore::indent(6) << nxthop.gateway;
		return os;
	};

	std::string
	str() const {
		std::stringstream ss;
		ss << "nexthop/inet6 " << gateway.str() << " weight " << (int)get_weight() << " ";
		return ss.str();
	};

private:

	rofl::caddress_in6	gateway;
};


/**
 *
 */
class crtnexthop_in6_find : public std::unary_function<crtnexthop_in6,bool> {
	crtnexthop_in6 rtnexthop;
public:
	crtnexthop_in6_find(const crtnexthop_in6& rtnexthop) :
		rtnexthop(rtnexthop) {};
	bool operator() (const crtnexthop_in6& rta) {
		return (rtnexthop == rta);
	};
	bool operator() (const std::pair<unsigned int, crtnexthop_in6>& p) {
		return (rtnexthop == p.second);
	};
#if 0
	bool operator() (const std::pair<unsigned int, crtnexthop_in6*>& p) {
		return (rtnexthop == *(p.second));
	};
#endif
};


};

#endif /* CRTNEXTHOP_H_ */
