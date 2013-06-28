/*
 * dptport.h
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#ifndef DPTPORT_H_
#define DPTPORT_H_ 1

#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofdpt.h>
#include <rofl/common/openflow/cflowentry.h>

#include <ctapdev.h>
#include <cnetlink.h>
#include <cpacketpool.h>

namespace dptmap
{

class eDptPortBase 				: public std::exception {};
class eDptPortCritical 			: public eDptPortBase {};
class eDptPortNoDptAttached		: public eDptPortBase {};
class eDptPortTapDevNotFound	: public eDptPortBase {};

class dptport :
		public cnetlink_subscriber,
		public cnetdev_owner
{
private:


	rofl::crofbase		*rofbase; 		// OpenFlow endpoint
	rofl::cofdpt		*dpt;			// cofdpt handle for attached data path element
	uint32_t			 of_port_no;	// OpenFlow portno assigned to port on dpt mapped to this dptport instance
	ctapdev				*tapdev;		// tap device emulating the mapped port on this system
	unsigned int		 ifindex;		// ifindex for tapdevice


public:

	/**
	 *
	 */
	dptport(
			rofl::crofbase *rofbase,
			rofl::cofdpt *dpt,
			uint32_t of_port_no);


	/**
	 *
	 */
	virtual
	~dptport();


	/**
	 *
	 */
	std::string get_devname() const { return dpt->get_ports()[of_port_no]->get_name(); };


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


	/**
	 *
	 * @param rtl
	 */
	virtual void addr_created(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 * @param rtl
	 */
	virtual void addr_updated(unsigned int ifindex, uint16_t adindex);


	/**
	 *
	 * @param ifindex
	 */
	virtual void addr_deleted(unsigned int ifindex, uint16_t adindex);


public:


	/**
	 *
	 * @param adindex
	 */
	void
	ip_endpoint_install_flow_mod(uint16_t adindex);


	/**
	 *
	 * @param adindex
	 */
	void
	ip_endpoint_remove_flow_mod(uint16_t adindex);
};

}; // end of namespace

#endif /* DPTPORT_H_ */
