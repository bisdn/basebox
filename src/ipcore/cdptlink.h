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

namespace ipcore
{

class eDptLinkBase 				: public std::exception {};
class eDptLinkCritical 			: public eDptLinkBase {};
class eDptLinkNoDptAttached		: public eDptLinkBase {};
class eDptLinkTapDevNotFound	: public eDptLinkBase {};
class eDptLinkNotFound			: public eDptLinkBase {};

class cdptlink :
		public rofcore::cnetlink_subscriber,
		public rofcore::cnetdev_owner
{
public:

	static cdptlink&
	get_link(unsigned int ifindex);

public:

	/**
	 *
	 */
	cdptlink(unsigned int ifindex);


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
	set_dptid(const rofl::cdptid& dptid) { this->dptid = dptid; };

	/**
	 *
	 */
	uint32_t
	get_ofp_port_no() const { return of_port_no; };

	/**
	 *
	 */
	void
	set_ofp_port_no(uint32_t of_port_no) { this->of_port_no = of_port_no; };

	/**
	 *
	 */
	const rofl::openflow::cofport&
	get_ofp_port() const { return rofl::crofdpt::get_dpt(dptid).get_ports().get_port(of_port_no); };

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

	void
	clear();

	cdptaddr_in4&
	add_addr_in4(
			uint16_t adindex);

	cdptaddr_in4&
	set_addr_in4(
			uint16_t adindex);

	const cdptaddr_in4&
	get_addr_in4(
			uint16_t adindex) const;

	void
	drop_addr_in4(
			uint16_t adindex);

	bool
	has_addr_in4(
			uint16_t adindex) const;




	cdptaddr_in6&
	add_addr_in6(
			uint16_t adindex);

	cdptaddr_in6&
	set_addr_in6(
			uint16_t adindex);

	const cdptaddr_in6&
	get_addr_in6(
			uint16_t adindex) const;

	void
	drop_addr_in6(
			uint16_t adindex);

	bool
	has_addr_in6(
			uint16_t adindex) const;



	cdptneigh_in4&
	add_neigh_in4(
			uint16_t nbindex);

	cdptneigh_in4&
	set_neigh_in4(
			uint16_t nbindex);

	const cdptneigh_in4&
	get_neigh_in4(
			uint16_t nbindex) const;

	void
	drop_neigh_in4(
			uint16_t nbindex);

	bool
	has_neigh_in4(
			uint16_t nbindex) const;




	cdptneigh_in6&
	add_neigh_in6(
			uint16_t nbindex);

	cdptneigh_in6&
	set_neigh_in6(
			uint16_t nbindex);

	const cdptneigh_in6&
	get_neigh_in6(
			uint16_t nbindex) const;

	void
	drop_neigh_in6(
			uint16_t nbindex);

	bool
	has_neigh_in6(
			uint16_t nbindex) const;



private:


	/**
	 *
	 */
	virtual void
	addr_in4_created(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 */
	virtual void
	addr_in4_updated(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 */
	virtual void
	addr_in4_deleted(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 */
	virtual void
	addr_in6_created(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 */
	virtual void
	addr_in6_updated(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 */
	virtual void
	addr_in6_deleted(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 */
	virtual void
	neigh_in4_created(unsigned int ifindex, uint16_t nbindex);


	/**
	 *
	 */
	virtual void
	neigh_in4_updated(unsigned int ifindex, uint16_t nbindex);


	/**
	 *
	 */
	virtual void
	neigh_in4_deleted(unsigned int ifindex, uint16_t nbindex);



	/**
	 *
	 */
	virtual void
	neigh_in6_created(unsigned int ifindex, uint16_t nbindex);


	/**
	 *
	 */
	virtual void
	neigh_in6_updated(unsigned int ifindex, uint16_t nbindex);


	/**
	 *
	 */
	virtual void
	neigh_in6_deleted(unsigned int ifindex, uint16_t nbindex);


private:


	/**
	 *
	 */
	void
	delete_all_addrs();


	/**
	 *
	 */
	void
	delete_all_neighs();


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, cdptlink const& link) {
		os << rofl::indent(0) << "<dptlink: ifindex:" << (int)link.ifindex << " "
				<< "devname:" << link.get_devname() << " >" << std::endl;
		os << rofl::indent(2) << "<hwaddr:"  << link.get_hwaddr() << " "
				<< "portno:"  << (int)link.of_port_no << " >" << std::endl;
		try {
			rofl::indent i(2);
			os << rofcore::cnetlink::get_instance().get_link(link.ifindex);
		} catch (rofcore::eNetLinkNotFound& e) {
			os << rofl::indent(2) << "<no crtlink found >" << std::endl;
		}
		for (std::map<uint16_t, cdptaddr_in4>::const_iterator
				it = link.dpt4addrs.begin(); it != link.dpt4addrs.end(); ++it) {
			rofl::indent i(2); os << it->second;
		}
		for (std::map<uint16_t, cdptneigh_in4>::const_iterator
				it = link.dpt4neighs.begin(); it != link.dpt4neighs.end(); ++it) {
			rofl::indent i(2); os << it->second << std::endl;
		}
		for (std::map<uint16_t, cdptaddr_in6>::const_iterator
				it = link.dpt6addrs.begin(); it != link.dpt6addrs.end(); ++it) {
			rofl::indent i(2); os << it->second;
		}
		for (std::map<uint16_t, cdptneigh_in6>::const_iterator
				it = link.dpt6neighs.begin(); it != link.dpt6neighs.end(); ++it) {
			rofl::indent i(2); os << it->second << std::endl;
		}
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
			return (ofp_port_no == p.second.of_port_no);
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

	static std::map<unsigned int, cdptlink*>		dptlinks;

	int					 		 		ifindex;		// ifindex for tapdevice
	rofl::cdptid						dptid;
	uint8_t								table_id;
	uint32_t			 		 		of_port_no;		// OpenFlow portno assigned to port on dpt mapped to this dptport instance
	rofcore::ctapdev					*tapdev;		// tap device emulating the mapped port on this system

	enum cdptlink_flag_t {
		FLAG_TAP_DEVICE_ACTIVE = 1,
		FLAG_FLOW_MOD_INSTALLED = 2,
	};

	std::bitset<32>						flags;

	std::map<uint16_t, cdptaddr_in4>  	dpt4addrs;		// all IPv4 addresses assigned to this link
	std::map<uint16_t, cdptaddr_in6>  	dpt6addrs;		// all IPv6 addresses assigned to this link
	std::map<uint16_t, cdptneigh_in4>	dpt4neighs;		// all neighbors seen on this link (for ARP)
	std::map<uint16_t, cdptneigh_in6>	dpt6neighs;		// all neighbors seen on this link (for NDP)
};

}; // end of namespace

#endif /* DPTPORT_H_ */
