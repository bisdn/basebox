/*
 * dptport.h
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#ifndef CDPTLINK_H_
#define CDPTLINK_H_ 1

#include <inttypes.h>

#include <bitset>

#include <rofl/common/logging.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>

#include "ctapdev.h"
#include "cnetlink.h"
#include "cpacketpool.h"
#include "cdptaddr.h"
#include "cdptneigh.h"
#include "caddrtable.h"
#include "cneightable.h"

namespace ipcore
{

class eDptLinkBase 				: public std::exception {};
class eDptLinkCritical 			: public eDptLinkBase {};
class eDptLinkNoDptAttached		: public eDptLinkBase {};
class eDptLinkTapDevNotFound	: public eDptLinkBase {};
class eDptLinkNotFound			: public eDptLinkBase {};

class cdptlink :
		public rofcore::cnetlink_common_observer,
		public rofcore::cnetdev_owner
{
public:

	/**
	 *
	 */
	cdptlink();


	/**
	 *
	 */
	virtual
	~cdptlink();

public:

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

	/**
	 *
	 */
	void
	set_dptid(const rofl::cdptid& dptid) {
		this->dptid = dptid;
		addrtable.set_dptid(dptid);
		neightable.set_dptid(dptid);
	};

	/**
	 *
	 */
	uint32_t
	get_ofp_port_no() const { return ofp_port_no; };

	/**
	 *
	 */
	void
	set_ofp_port_no(uint32_t ofp_port_no) { this->ofp_port_no = ofp_port_no; };

	/**
	 *
	 */
	const rofl::openflow::cofport&
	get_ofp_port() const { return rofl::crofdpt::get_dpt(dptid).get_ports().get_port(ofp_port_no); };

	/**
	 *
	 */
	std::string
	get_devname() const { return get_ofp_port().get_name(); };

	/**
	 *
	 */
	rofl::cmacaddr
	get_hwaddr() const { return get_ofp_port().get_hwaddr(); };

	/**
	 *
	 */
	unsigned int
	get_ifindex() const { return ifindex; }

	/**
	 *
	 */
	void
	tap_open();

	/**
	 *
	 */
	void
	tap_close();

	/**
	 *
	 */
	void
	install();

	/**
	 *
	 */
	void
	uninstall();

public:

	/**
	 *
	 */
	void
	clear() {
		addrtable.clear(); neightable.clear();
	};

	/*
	 * from ctapdev
	 */
	virtual void
	enqueue(
			rofcore::cnetdev *netdev, rofl::cpacket* pkt);

	virtual void
	enqueue(
			rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts);


	/*
	 * from crofbase
	 */
	void
	handle_packet_in(
			rofl::cpacket const& pkt);

	void
	handle_port_status();


public:

	/**
	 *
	 */
	const caddrtable&
	get_addr_table() const { return addrtable; };

	/**
	 *
	 */
	caddrtable&
	set_addr_table() { return addrtable; };

	/**
	 *
	 */
	const cneightable&
	get_neigh_table() const { return neightable; };

	/**
	 *
	 */
	cneightable&
	set_neigh_table() { return neightable; };

public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, cdptlink const& link) {
		os << rofcore::indent(0) << "<dptlink: ifindex:" << (int)link.ifindex << " "
				<< "devname:" << link.get_devname() << " >" << std::endl;
		os << rofcore::indent(2) << "<hwaddr:"  << link.get_hwaddr() << " "
				<< "portno:"  << (int)link.ofp_port_no << " >" << std::endl;
		try {
			rofcore::indent i(2);
			os << rofcore::cnetlink::get_instance().get_links().get_link(link.ifindex);
		} catch (rofcore::eNetLinkNotFound& e) {
			os << rofl::indent(2) << "<no crtlink found >" << std::endl;
		}
		{ rofcore::indent i(2); os << link.addrtable; };
		{ rofcore::indent i(2); os << link.neightable; };
		return os;
	};


	/**
	 *
	 */
	class cdptlink_by_ofp_port_no {
		uint32_t ofp_port_no;
	public:
		cdptlink_by_ofp_port_no(uint32_t ofp_port_no) : ofp_port_no(ofp_port_no) {};
		bool operator() (const std::pair<unsigned int, cdptlink>& p) {
			return (ofp_port_no == p.second.ofp_port_no);
		};
	};


	/**
	 *
	 */
	class cdptlink_by_ifindex {
		int ifindex;
	public:
		cdptlink_by_ifindex(int ifindex) : ifindex(ifindex) {};
		bool operator() (const std::pair<unsigned int, cdptlink>& p) {
			return (ifindex == p.second.ifindex);
		};
	};

private:

	uint32_t			 		 		ofp_port_no;		// OpenFlow portno assigned to port on dpt mapped to this dptport instance
	int					 		 		ifindex;		// ifindex for tapdevice
	rofl::cdptid						dptid;
	uint8_t								table_id;
	rofcore::ctapdev					*tapdev;		// tap device emulating the mapped port on this system

	enum cdptlink_flag_t {
		FLAG_TAP_DEVICE_ACTIVE = 1,
		FLAG_FLOW_MOD_INSTALLED = 2,
	};

	std::bitset<32>						flags;

	caddrtable							addrtable;		// all IPv4 and IPv6 addresses assigned to this link
	cneightable							neightable;		// all neighbors seen on this link for ARP and NDP
};

}; // end of namespace

#endif /* DPTPORT_H_ */
