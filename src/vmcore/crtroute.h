/*
 * crtroute.h
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#ifndef CRTROUTE_H_
#define CRTROUTE_H_ 1

#include <vector>
#include <ostream>
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/object.h>
#include <netlink/route/route.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/caddress.h>

#include <crtnexthop.h>


namespace dptmap
{

class crtroute
{
private:


	uint32_t 				table_id;
	uint8_t					scope;
	uint8_t					tos;
	uint8_t					protocol;
	uint32_t				priority;
	uint8_t					family;
	rofl::caddress			dst;
	rofl::caddress			src;
	uint8_t					type;
	uint32_t				flags;
	int						metric;
	rofl::caddress			pref_src;
	unsigned int			ifindex;
	std::vector<crtnexthop>	nexthops;


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
	crtroute(crtroute const& rtr);


	/**
	 *
	 */
	crtroute&
	operator= (crtroute const& rtr);


	/**
	 *
	 */
	crtroute(struct rtnl_route *route);


	/**
	 *
	 */
	bool
	operator== (crtroute const& rtr);


public:


	/**
	 *
	 */
	uint32_t get_table_id() const { return table_id; };


	/**
	 *
	 */
	uint8_t get_scope() const { return scope; };


	/**
	 *
	 */
	uint8_t get_tos() const { return tos; };


	/**
	 *
	 */
	uint8_t get_protocol() const { return protocol; };


	/**
	 *
	 */
	uint32_t get_priority() const { return priority; };


	/**
	 *
	 */
	uint8_t get_family() const { return family; };


	/**
	 *
	 */
	rofl::caddress get_dst() const { return dst; };


	/**
	 *
	 */
	rofl::caddress get_src() const { return src; };


	/**
	 *
	 */
	uint8_t get_type() const { return type; };


	/**
	 *
	 */
	uint32_t get_flags() const { return flags; };


	/**
	 *
	 */
	int get_metric() const { return metric; };


	/**
	 *
	 */
	rofl::caddress get_pref_src() const { return pref_src; };


	/**
	 *
	 */
	int get_ifindex() const { return ifindex; };


	/**
	 *
	 */
	std::vector<crtnexthop> get_nexthops() const { return nexthops; };


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtroute const& rtr)
	{
		os << "crtroute{"
				<< "table_id=" 	<< (unsigned int)rtr.table_id << " "
				<< "scope=" 	<< (unsigned int)rtr.scope << " "
				<< "tos=" 		<< (unsigned int)rtr.tos << " "
				<< "protocol=" 	<< (unsigned int)rtr.protocol << " "
				<< "priority=" 	<< (unsigned int)rtr.priority << " "
				<< "family=" 	<< (unsigned int)rtr.family << " "
				<< "dst=" 		<< rtr.dst << " "
				<< "src=" 		<< rtr.src << " "
				<< "type=" 		<< (unsigned int)rtr.type << " "
				<< "flags=" 	<< (unsigned int)rtr.flags << " "
				<< "metric=" 	<< (int)rtr.metric << " "
				<< "pref_src=" 	<< rtr.pref_src << " "
				<< "ifindex=" 	<< (unsigned int)rtr.ifindex << " ";
		// TODO: next hops
		os << "}";
		return os;
	};


	/**
	 *
	 */
	class crtroute_find : public std::unary_function<crtroute,bool> {
		uint32_t table_id;
		uint8_t scope;
		unsigned int ifindex;
		rofl::caddress dst;
	public:
		crtroute_find(uint32_t table_id, uint8_t scope, unsigned int ifindex, rofl::caddress const& dst) :
			table_id(table_id), scope(scope), ifindex(ifindex), dst(dst) {};
		bool operator() (std::pair<unsigned int, crtroute> const& p) {
			return ((table_id 		== p.second.table_id) &&
					(scope 			== p.second.scope) &&
					(ifindex		== p.second.ifindex) &&
					(dst			== p.second.dst));
		};
	};
};

}; // end of namespace

#endif /* CRTROUTE_H_ */
