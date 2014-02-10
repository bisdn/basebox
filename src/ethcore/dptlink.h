/*
 * dptport.h
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#ifndef DPTLINK_H_
#define DPTLINK_H_ 1

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/logging.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>

#include "ctapdev.h"
#include "cnetlink.h"
#include "cpacketpool.h"

namespace ethercore
{

class eDptLinkBase 				: public std::exception {};
class eDptLinkCritical 			: public eDptLinkBase {};
class eDptLinkNoDptAttached		: public eDptLinkBase {};
class eDptLinkTapDevNotFound	: public eDptLinkBase {};
class eDptLinkNotFound			: public eDptLinkBase {};

class dptlink :
		public cnetlink_subscriber,
		public cnetdev_owner
{
public:

	static dptlink&
	get_dptlink(unsigned int ifindex);

private:

	static std::map<unsigned int, dptlink*>		dptlinks;

	rofl::crofbase				*rofbase; 		// OpenFlow endpoint
	rofl::crofdpt				*dpt;			// cofdpt handle for attached data path element
	uint32_t			 		 of_port_no;	// OpenFlow portno assigned to port on dpt mapped to this dptport instance
	ctapdev						*tapdev;		// tap device emulating the mapped port on this system
	unsigned int		 		 ifindex;		// ifindex for tapdevice



public:

	/**
	 *
	 */
	dptlink(
			rofl::crofbase *rofbase,
			rofl::crofdpt *dpt,
			uint32_t of_port_no);


	/**
	 *
	 */
	virtual
	~dptlink();


	/**
	 *
	 */
	std::string get_devname() const { return dpt->get_ports()[of_port_no]->get_name(); };


	/**
	 *
	 */
	unsigned int get_ifindex() const { return ifindex; }


	/**
	 *
	 */
	uint32_t get_of_port_no() const { return of_port_no; };


	/**
	 *
	 */
	void
	open();


	/**
	 *
	 */
	void
	close();


public:


	/*
	 * from ctapdev
	 */
	virtual void enqueue(cnetdev *netdev, rofl::cpacket* pkt);

	virtual void enqueue(cnetdev *netdev, std::vector<rofl::cpacket*> pkts);


	/*
	 * from crofbase
	 */
	void handle_packet_in(rofl::cpacket const& pkt);

	void handle_port_status();

public:


	/**
	 *
	 * @param rtl
	 */
	virtual void link_created(unsigned int ifindex);


	/**
	 *
	 * @param rtl
	 */
	virtual void link_updated(unsigned int ifindex);


	/**
	 *
	 * @param ifindex
	 */
	virtual void link_deleted(unsigned int ifindex);



public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, dptlink const& link) {
		os << rofl::indent(0) << "<dptlink: ifindex:" << (int)link.ifindex << " "
				<< "devname:" << link.dpt->get_ports().get_port(link.of_port_no).get_name() << " >" << std::endl;
		os << rofl::indent(2) << "<hwaddr:"  << link.dpt->get_ports().get_port(link.of_port_no).get_hwaddr() << " "
				<< "portno:"  << (int)link.of_port_no << " >" << std::endl;
		try {
			rofl::indent i(2);
			os << cnetlink::get_instance().get_link(link.ifindex);
		} catch (eNetLinkNotFound& e) {
			os << rofl::indent(2) << "<no crtlink found >" << std::endl;
		}

		return os;
	};
};

}; // end of namespace

#endif /* DPTPORT_H_ */
