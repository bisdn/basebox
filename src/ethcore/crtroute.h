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

#include <crtnexthop.h>


namespace ethercore
{


class eRtRouteBase 			: public std::exception {};
class eRtRouteNotFound 		: public eRtRouteBase {};


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
	unsigned int			prefixlen;
	rofl::caddress			mask;
	rofl::caddress			src;
	uint8_t					type;
	uint32_t				flags;
	int						metric;
	rofl::caddress			pref_src;
	unsigned int			iif;
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
	std::string get_table_id_s() const;


	/**
	 *
	 */
	uint8_t get_scope() const { return scope; };


	/**
	 *
	 */
	std::string get_scope_s() const;


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
	unsigned int get_prefixlen() const { return prefixlen; };


	/**
	 *
	 */
	rofl::caddress get_mask() const { return mask; };


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
	int get_iif() const { return iif; };


	/**
	 *
	 */
	std::vector<crtnexthop>& get_nexthops() { return nexthops; };


	/**
	 *
	 */
	crtnexthop& get_nexthop(unsigned int index);


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtroute const& rtr)
	{
		os << rofl::indent(0) << "<crtroute: >" << std::endl;
		os << rofl::indent(2) << "<table_id: " 	<< rtr.get_table_id_s() 		<< " >" << std::endl;
		os << rofl::indent(2) << "<scope: " 	<< rtr.get_scope_s() 			<< " >" << std::endl;
		os << rofl::indent(2) << "<tos: " 		<< (unsigned int)rtr.tos 		<< " >" << std::endl;
		os << rofl::indent(2) << "<protocol: " 	<< (unsigned int)rtr.protocol 	<< " >" << std::endl;
		os << rofl::indent(2) << "<priority: " 	<< (unsigned int)rtr.priority 	<< " >" << std::endl;
		os << rofl::indent(2) << "<family: " 	<< (unsigned int)rtr.family 	<< " >" << std::endl;
		os << rofl::indent(2) << "<dst: " 		<< rtr.dst 						<< " >"	<< std::endl;
		os << rofl::indent(2) << "<prefixlen: " << rtr.prefixlen 				<< " >" << std::endl;
		os << rofl::indent(2) << "<mask: " 		<< rtr.mask 					<< " >" << std::endl;
		os << rofl::indent(2) << "<src: " 		<< rtr.src 						<< " >" << std::endl;
		os << rofl::indent(2) << "<type: " 		<< (unsigned int)rtr.type 		<< " >" << std::endl;
		os << rofl::indent(2) << "<flags: " 	<< (unsigned int)rtr.flags 		<< " >" << std::endl;
		os << rofl::indent(2) << "<metric: " 	<< (int)rtr.metric 				<< " >" << std::endl;
		os << rofl::indent(2) << "<pref_src: " 	<< rtr.pref_src 				<< " >" << std::endl;
		os << rofl::indent(2) << "<ifindex: " 	<< (unsigned int)rtr.iif 		<< " >" << std::endl;

		rofl::indent i(2);
		for (std::vector<crtnexthop>::const_iterator
				it = rtr.nexthops.begin(); it != rtr.nexthops.end(); ++it) {
			os << (*it);
		}

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
					(ifindex		== p.second.iif) &&
					(dst			== p.second.dst));
		};
	};
};

}; // end of namespace

#endif /* CRTROUTE_H_ */
