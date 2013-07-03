/*
 * vmcore.h
 *
 *  Created on: 29.05.2013
 *      Author: andi
 */

#ifndef VMCORE_H_
#define VMCORE_H_ 1

#include <map>
#include <exception>

//#include "cnetlink.h"
#include <rofl/common/crofbase.h>

#include <dptlink.h>
#include <dptroute.h>
#include <cnetlink.h>

namespace dptmap
{

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};

class vmcore :
		public rofl::crofbase,
		public cnetlink_subscriber
{
private:


	rofl::cofdpt 												*dpt;		// handle for cofdpt instance managed by this vmcore
	std::map<rofl::cofdpt*, std::map<uint32_t, dptlink*> > 		 dptlinks;	// mapped ports per data path element
	std::map<uint8_t, std::map<unsigned int, dptroute*> >		 dptroutes;	// active routes => key1:table_id, key2:routing index, value: dptroute instance


public:


	/**
	 *
	 */
	vmcore();


	/**
	 *
	 */
	virtual
	~vmcore();


public:


	virtual void
	handle_dpath_open(rofl::cofdpt *dpt);


	virtual void
	handle_dpath_close(rofl::cofdpt *dpt);


	virtual void
	handle_port_status(rofl::cofdpt *dpt, rofl::cofmsg_port_status *msg);

	virtual void
	handle_packet_out(rofl::cofctl *ctl, rofl::cofmsg_packet_out *msg);

	virtual void
	handle_packet_in(rofl::cofdpt *dpt, rofl::cofmsg_packet_in *msg);


public:


	virtual void
	route_created(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_updated(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_deleted(uint8_t table_id, unsigned int rtindex);

	virtual void
	neigh_created(unsigned int ifindex, uint16_t nbindex);

	virtual void
	neigh_updated(unsigned int ifindex, uint16_t nbindex);

	virtual void
	neigh_deleted(unsigned int ifindex, uint16_t nbindex);

private:

	bool
	link_is_mapped_from_dpt(int ifindex);

	void
	delete_all_ports();

	void
	delete_all_routes();
};

}; // end of namespace vmcore


#endif /* VMCORE_H_ */
