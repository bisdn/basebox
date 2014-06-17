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
#include <rofl/common/logging.h>

namespace rofcore {

class crtnexthop {
public:

	/**
	 *
	 */
	crtnexthop();

	/**
	 *
	 */
	virtual
	~crtnexthop();

	/**
	 *
	 */
	crtnexthop(
			const crtnexthop& nxthop);

	/**
	 *
	 */
	crtnexthop&
	operator= (
			const crtnexthop& nxthop);

	/**
	 *
	 */
	crtnexthop(
			struct rtnl_route *route,
			struct rtnl_nexthop *nxthop);

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
		os << rofl::indent(0) << "<crtnexthop: >" << std::endl;
		os << rofl::indent(2) << "<family: " 	<< (int)nxthop.family 			<< " >" << std::endl;
		os << rofl::indent(2) << "<weight: " 	<< (int)nxthop.weight 			<< " >" << std::endl;
		os << rofl::indent(2) << "<ifindex: " 	<< (int)nxthop.ifindex 			<< " >" << std::endl;
		os << rofl::indent(2) << "<flags: " 	<< nxthop.flags 				<< " >" << std::endl;
		os << rofl::indent(2) << "<realms: " 	<< (unsigned int)nxthop.realms 	<< " >" << std::endl;
		return os;
	};

private:

	uint8_t			family;
	uint8_t			weight;
	int				ifindex;
	unsigned int	flags;
	uint32_t		realms;
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
			struct rtnl_nexthop *nxthop) {

		rtnl_route_get(route);

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_gw(nl_addr2str(rtnl_route_nh_get_gateway(nxthop), s_buf, sizeof(s_buf)));
		s_gw 		= s_gw.substr(0, s_gw.find_first_of("/", 0));
		gateway 	= rofl::caddress_in4(s_gw.c_str());

		rtnl_route_put(route);
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
		os << dynamic_cast<const crtnexthop&>( nxthop );
		os << rofl::indent(0) << "<crtnexthop_in4: >" << std::endl;
		os << rofl::indent(2) << "<gateway: >" << std::endl;
		{ rofl::indent i(4); os << nxthop.gateway; }
		return os;
	};

private:

	rofl::caddress_in4	gateway;
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
			struct rtnl_nexthop *nxthop) {

		rtnl_route_get(route);

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_gw(nl_addr2str(rtnl_route_nh_get_gateway(nxthop), s_buf, sizeof(s_buf)));
		s_gw 		= s_gw.substr(0, s_gw.find_first_of("/", 0));
		gateway 	= rofl::caddress_in6(s_gw.c_str());

		rtnl_route_put(route);
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
		os << dynamic_cast<const crtnexthop&>( nxthop );
		os << rofl::indent(0) << "<crtnexthop_in6: >" << std::endl;
		os << rofl::indent(2) << "<gateway: >" << std::endl;
		{ rofl::indent i(4); os << nxthop.gateway; }
		return os;
	};

private:

	rofl::caddress_in6	gateway;
};



};

#endif /* CRTNEXTHOP_H_ */
