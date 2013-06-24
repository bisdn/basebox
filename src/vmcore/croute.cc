/*
 * croute.cc
 *
 *  Created on: 29.05.2013
 *      Author: andi
 */


#include "croute.h"


struct croute::route_type_name_t rtype_names[] = {
		{ croute::RT_UNSPEC, 		"unspecified" 	},
		{ croute::RT_UNICAST, 		"unicast" 		},
		{ croute::RT_LOCAL, 		"local" 		},
		{ croute::RT_BROADCAST, 	"broadcast" 	},
		{ croute::RT_ANYCAST, 		"anycast" 		},
		{ croute::RT_MULTICAST, 	"multicast" 	},
		{ croute::RT_BLACKHOLE, 	"blackhole" 	},
		{ croute::RT_UNREACHABLE, 	"unreachable" 	},
		{ croute::RT_PROHIBIT, 		"prohibit" 		},
		{ croute::RT_THROW, 		"throw" 		},
		{ croute::RT_NAT, 			"nat" 			},
};


struct croute::route_proto_name_t rproto_names[] = {
		{ 0,							""			},
		{ croute::RT_PROTO_REDIRECT, 	"redirect"	},
		{ croute::RT_PROTO_KERNEL, 		"kernel"	},
		{ croute::RT_PROTO_BOOT,	 	"boot"		},
		{ croute::RT_PROTO_STATIC,	 	"static"	},
};


croute::croute(
		uint16_t rt_family,
		uint8_t route_type,
		rofl::caddress const& dst,
		rofl::caddress const& mask,
		rofl::caddress const& src,
		rofl::caddress const& via,
		std::string const& devname,
		int ifindex) :
				rt_family(rt_family),
				rt_type(route_type),
				dst(dst),
				mask(mask),
				src(src),
				via(via),
				devname(devname),
				ifindex(ifindex),
				tos(0),
				metric(1),
				preference(0),
				mtu(0),
				table_id(0),
				rt_proto(0)
{

}


croute::~croute()
{

}


croute::croute(croute const& route) :
		dst(rofl::caddress(AF_INET)),
		mask(rofl::caddress(AF_INET)),
		src(rofl::caddress(AF_INET)),
		via(rofl::caddress(AF_INET))
{
	*this = route;
}


croute&
croute::operator= (croute const& route)
{
	if (this == &route)
		return *this;

	rt_family		= route.rt_family;
	rt_type			= route.rt_type;
	dst				= route.dst;
	mask			= route.mask;
	src				= route.src;
	via				= route.via;
	tos				= route.tos;
	metric			= route.metric;
	preference		= route.preference;
	devname			= route.devname;
	ifindex			= route.ifindex;
	mtu				= route.mtu;
	table_id		= route.table_id;
	rt_proto		= route.rt_proto;

	return *this;
}


const char*
croute::c_str()
{
	size_t size = 256;
	char info[size];
	memset(info, 0, size);

	snprintf(info, size-1, "[%d] to %s %s via %s dev %s(%d) proto %s table-id %d",
			rt_family, rt_type2str(rt_type), dst.addr_c_str(), via.addr_c_str(), devname.c_str(), ifindex, rt_proto2str(rt_proto), table_id);
	s_info.assign(info);

	return s_info.c_str();
}


const char*
croute::rt_type2str(uint8_t rt_type)
{
	if (rt_type >= RT_MAX)
		return (const char*)0;
	return rtype_names[rt_type].route_type_name;
}


const char*
croute::rt_proto2str(uint8_t rt_proto)
{
	if (rt_proto >= RT_MAX)
		return (const char*)0;
	return rproto_names[rt_proto].route_proto_name;
}
