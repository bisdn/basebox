/*
 * croute.h
 *
 *  Created on: 29.05.2013
 *      Author: andi
 */

#ifndef CROUTE_H_
#define CROUTE_H_

#include <string>

#include <rofl/common/caddress.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif


class croute {
public:


	std::string				s_info;

	struct route_type_name_t {
		uint8_t rt_type;
		const char* route_type_name;
	};

	struct route_proto_name_t {
		uint8_t rt_proto;
		const char* route_proto_name;
	};


	enum croute_type_t {
		RT_UNSPEC = 0,
		RT_UNICAST,
		RT_LOCAL,
		RT_BROADCAST,
		RT_ANYCAST,
		RT_MULTICAST,
		RT_BLACKHOLE,
		RT_UNREACHABLE,
		RT_PROHIBIT,
		RT_THROW,
		RT_NAT,
		RT_MAX
	};

	enum croute_proto_t {
		RT_PROTO_REDIRECT = 1,
		RT_PROTO_KERNEL,
		RT_PROTO_BOOT,
		RT_PROTO_STATIC,
		RT_PROTO_MAX
	};

	uint16_t				rt_family;	// AF_INET, AF_INET6, ...
	uint8_t					rt_type;
	rofl::caddress			dst;		// route destination
	rofl::caddress			mask;		// route netmask
	rofl::caddress			src;		// preferred source address of interface to use when sending via this route
	rofl::caddress			via;		// next hop for route
	uint8_t 				tos;
	uint8_t					metric;
	uint32_t 				preference;
	std::string				devname;
	int						ifindex;
	int						mtu;
	uint8_t					table_id;
	uint8_t 				rt_proto;

public:

	/**
	 *
	 */
	croute(uint16_t rt_family = 0,
			uint8_t route_type = RT_UNSPEC,
			rofl::caddress const& dst 	= 	rofl::caddress(AF_INET, "0.0.0.0"),
			rofl::caddress const& mask 	= 	rofl::caddress(AF_INET, "0.0.0.0"),
			rofl::caddress const& src 	= 	rofl::caddress(AF_INET, "0.0.0.0"),
			rofl::caddress const& via 	= 	rofl::caddress(AF_INET, "0.0.0.0"),
			std::string const& devname	= 	std::string(""),
			int ifindex = 0);


	/**
	 *
	 */
	~croute();


	/**
	 *
	 */
	croute(croute const& route);


	/**
	 *
	 */
	croute&
	operator= (croute const& route);


	/**
	 *
	 */
	const char*
	c_str();


private:


	/**
	 *
	 */
	const char*
	rt_type2str(uint8_t rt_type);

	/**
	 *
	 */
	const char*
	rt_proto2str(uint8_t rt_proto);

};



#endif /* CROUTE_H_ */
