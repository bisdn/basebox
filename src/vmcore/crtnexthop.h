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
	operator<< (
			std::ostream& os, crtnexthop const& nxthop) {
		os << "<crtnexthop: ";
		os << "family: " 	<< (int)nxthop.family << " ";
		os << "weight: " 	<< (int)nxthop.weight << " ";
		os << "ifindex: " 	<< (int)nxthop.ifindex << " ";
		os << "gateway: " 	<< nxthop.gateway << " ";
		os << "flags: " 	<< nxthop.flags << " ";
		os << "realms: " 	<< (unsigned int)nxthop.realms << " ";
		os << ">";
		return os;
	};
};

};

#endif /* CRTNEXTHOP_H_ */
