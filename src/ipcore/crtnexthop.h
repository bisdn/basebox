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

namespace dptmap
{

class crtnexthop
{
private:

	uint8_t			family;
	uint8_t			weight;
	int				ifindex;
	rofl::caddress	gateway;
	unsigned int	flags;
	uint32_t		realms;

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
			crtnexthop const& nxthop);


	/**
	 *
	 */
	crtnexthop&
	operator= (crtnexthop const& nxthop);


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
	uint8_t get_weight() const { return weight; };


	/**
	 *
	 */
	int get_ifindex() const { return ifindex; };


	/**
	 *
	 */
	rofl::caddress get_gateway() const { return gateway; };


	/**
	 *
	 */
	unsigned int get_flags() const { return flags; };


	/**
	 *
	 */
	uint32_t get_realms() const { return realms; };


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtnexthop const& nxthop) {
		os << rofl::indent(0) << "<crtnexthop: >" << std::endl;
		os << rofl::indent(2) << "<family: " 	<< (int)nxthop.family 			<< " >" << std::endl;
		os << rofl::indent(2) << "<weight: " 	<< (int)nxthop.weight 			<< " >" << std::endl;
		os << rofl::indent(2) << "<ifindex: " 	<< (int)nxthop.ifindex 			<< " >" << std::endl;
		os << rofl::indent(2) << "<gateway: " 	<< nxthop.gateway 				<< " >" << std::endl;
		os << rofl::indent(2) << "<flags: " 	<< nxthop.flags 				<< " >" << std::endl;
		os << rofl::indent(2) << "<realms: " 	<< (unsigned int)nxthop.realms 	<< " >" << std::endl;
		return os;
	};
};

};

#endif /* CRTNEXTHOP_H_ */
