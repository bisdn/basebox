/*
 * crtlink.h
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */

#ifndef CRTLINK_H_
#define CRTLINK_H_ 1

#include <exception>
#include <algorithm>
#include <ostream>
#include <string>
#include <map>
#include <set>

#include <rofl/common/caddress.h>
#include <rofl/common/logging.h>
#include <rofl/common/cmemory.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/object.h>
#include <netlink/route/link.h>
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <crtaddr.h>
#include <crtneigh.h>

namespace rofcore {

class eRtLinkBase		: public std::exception {};
class eRtLinkNotFound	: public eRtLinkBase {};

class crtlink
{
private:

	std::map<uint16_t, crtaddr_in4>  addrs_in4;	// key: index, value: address, index remains constant, as long as addrs endures
	std::map<uint16_t, crtaddr_in6>  addrs_in6;	// key: index, value: address, index remains constant, as long as addrs endures
	std::map<uint16_t, crtneigh_in4> neighs_in4; // key: index, valie: neigh, index remains constant throughout lifetime of neighbor entry
	std::map<uint16_t, crtneigh_in6> neighs_in6; // key: index, valie: neigh, index remains constant throughout lifetime of neighbor entry

public:


	std::string				devname;	// device name (e.g. eth0)
	rofl::cmacaddr			maddr;		// mac address
	rofl::cmacaddr			bcast; 		// broadcast address
	unsigned int			flags;		// link flags
	int						af;			// address family (AF_INET, AF_UNSPEC, ...)
	unsigned int			arptype;	// ARP type (e.g. ARPHDR_ETHER)
	int						ifindex;	// interface index
	unsigned int			mtu;		// maximum transfer unit


public:

	/**
	 *
	 */
	crtlink();


	/**
	 *
	 */
	virtual
	~crtlink();


	/**
	 *
	 */
	crtlink(crtlink const& rtlink);


	/**
	 *
	 */
	crtlink&
	operator= (crtlink const& rtlink);


	/**
	 *
	 */
	crtlink(struct rtnl_link *link);


public:


	/**
	 *
	 */
	crtaddr_in4&
	get_addr_in4(
			uint16_t index);


	/**
	 *
	 */
	uint16_t
	get_addr_in4(
			const crtaddr_in4& rta);


	/**
	 *
	 */
	uint16_t
	set_addr_in4(
			const crtaddr_in4& rta);


	/**
	 *
	 */
	uint16_t
	del_addr_in4(
			uint16_t index = crtaddr::CRTLINK_ADDR_ALL);


	/**
	 *
	 */
	uint16_t
	del_addr_in4(
			const crtaddr_in4& rta);



	/**
	 *
	 */
	crtaddr_in6&
	get_addr_in6(
			uint16_t index);


	/**
	 *
	 */
	uint16_t
	get_addr_in6(
			const crtaddr_in6& rta);


	/**
	 *
	 */
	uint16_t
	set_addr_in6(
			const crtaddr_in6& rta);


	/**
	 *
	 */
	uint16_t
	del_addr_in6(
			uint16_t index = crtaddr::CRTLINK_ADDR_ALL);


	/**
	 *
	 */
	uint16_t
	del_addr_in6(
			const crtaddr_in6& rta);



	/**
	 *
	 */
	crtneigh_in4&
	get_neigh_in4(
			uint16_t index);

	/**
	 *
	 */
	crtneigh_in4&
	get_neigh_in4(
			const rofl::caddress_in4& dst);

	/**
	 *
	 */
	uint16_t
	get_neigh_in4_index(
			const crtneigh_in4& neigh);

	/**
	 *
	 */
	uint16_t
	get_neigh_in4_index(
			const rofl::caddress_in4& dst);

	/**
	 *
	 */
	uint16_t
	set_neigh_in4(
			const crtneigh_in4& neigh);

	/**
	 *
	 */
	uint16_t
	del_neigh_in4(
			uint16_t index = crtneigh::CRTNEIGH_ADDR_ALL);

	/**
	 *
	 */
	uint16_t
	del_neigh_in4(
			const crtneigh_in4& neigh);


	/**
	 *
	 */
	crtneigh_in6&
	get_neigh_in6(
			uint16_t index);

	/**
	 *
	 */
	crtneigh_in6&
	get_neigh_in6(
			const rofl::caddress_in6& dst);

	/**
	 *
	 */
	uint16_t
	get_neigh_in6_index(
			const crtneigh_in6& neigh);

	/**
	 *
	 */
	uint16_t
	get_neigh_in6_index(
			const rofl::caddress_in6& dst);

	/**
	 *
	 */
	uint16_t
	set_neigh_in6(
			const crtneigh_in6& neigh);

	/**
	 *
	 */
	uint16_t
	del_neigh_in6(
			uint16_t index = crtneigh::CRTNEIGH_ADDR_ALL);

	/**
	 *
	 */
	uint16_t
	del_neigh_in6(
			const crtneigh_in6& neigh);



public:


	/**
	 *
	 */
	std::string get_devname() const { return devname; };


	/**
	 *
	 */
	rofl::cmacaddr get_hwaddr() const { return maddr; };


	/**
	 *
	 */
	rofl::cmacaddr get_broadcast() const { return bcast; };


	/**
	 *
	 */
	unsigned int get_flags() const { return flags; };


	/**
	 *
	 */
	int get_family() const { return af; };


	/**
	 *
	 */
	unsigned int get_arptype() const { return arptype; };


	/**
	 *
	 */
	int get_ifindex() const { return ifindex; };


	/**
	 *
	 */
	unsigned int get_mtu() const { return mtu; };


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtlink const& rtlink)
	{
		os << rofl::indent(0) << "<crtlink: >" << std::endl;
		os << rofl::indent(2) << "<devname: " << rtlink.devname 	<< " >" << std::endl;
		os << rofl::indent(2) << "<maddr: >" << std::endl;
		{ rofl::indent i(4); os << rtlink.maddr; };
		os << rofl::indent(2) << "<bcast: >" << std::endl;
		{ rofl::indent i(4); os << rtlink.bcast; };
		os << rofl::indent(2) << "<flags: " << (std::hex) << rtlink.flags << (std::dec) << " >" << std::endl;
		os << rofl::indent(2) << "<af: " << rtlink.af 				<< " >" << std::endl;
		os << rofl::indent(2) << "<arptype: " << rtlink.arptype 	<< " >" << std::endl;
		os << rofl::indent(2) << "<ifindex: " << rtlink.ifindex 	<< " >" << std::endl;
		os << rofl::indent(2) << "<mtu: " << rtlink.mtu 			<< " >" << std::endl;
		if (rtlink.addrs_in4.size() > 0) {
			rofl::indent i(4);
			for (std::map<uint16_t, crtaddr_in4>::const_iterator
					it = rtlink.addrs_in4.begin(); it != rtlink.addrs_in4.end(); ++it) {
				os << it->second;
			}
		}
		if (rtlink.addrs_in6.size() > 0) {
			rofl::indent i(4);
			for (std::map<uint16_t, crtaddr_in6>::const_iterator
					it = rtlink.addrs_in6.begin(); it != rtlink.addrs_in6.end(); ++it) {
				os << it->second;
			}
		}

		return os;
	};


	/**
	 *
	 */
	class crtlink_find_by_ifindex : public std::unary_function<crtlink,bool> {
		unsigned int ifindex;
	public:
		crtlink_find_by_ifindex(unsigned int ifindex) :
			ifindex(ifindex) {};
		bool operator() (crtlink const& rtl) {
			return (ifindex == rtl.ifindex);
		};
		bool operator() (std::pair<unsigned int, crtlink> const& p) {
			return (ifindex == p.second.ifindex);
		};
	};


	/**
	 *
	 */
	class crtlink_find_by_devname : public std::unary_function<crtlink,bool> {
		std::string devname;
	public:
		crtlink_find_by_devname(std::string const& devname) :
			devname(devname) {};
		bool operator() (crtlink const& rtl) {
			return (devname == rtl.devname);
		};
		bool operator() (std::pair<unsigned int, crtlink> const& p) {
			return (devname == p.second.devname);
		};
	};
};

}; // end of namespace dptmap

#endif /* CRTLINK_H_ */
