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

#include <dptport.h>
#include <cnetlink.h>

namespace dptmap
{

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};

class vmcore :
	/*public cnetlink_owner,*/
		public rofl::crofbase,
		public cnetlink_subscriber
{
private:


	rofl::cofdpt *dpt;	// handle for cofdpt instance managed by this vmcore
	std::map<rofl::cofdpt*, std::map<uint32_t, dptport*> > dptports;	// mapped ports per data path element


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
	link_created(unsigned int ifindex);

	virtual void
	link_updated(unsigned int ifindex);

	virtual void
	link_deleted(unsigned int ifindex);

	virtual void
	addr_created(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_updated(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_deleted(unsigned int ifindex, uint16_t adindex);

	virtual void
	route_created(unsigned int rtindex);

	virtual void
	route_updated(unsigned int rtindex);

	virtual void
	route_deleted(unsigned int rtindex);


public: // overloaded from cnetlink_owner

#if 0
	/**
	 *
	 */
	void
	nl_route_new(cnetlink const* netlink, croute const& route);


	/**
	 *
	 */
	void
	nl_route_del(cnetlink const* netlink, croute const& route);


	/**
	 *
	 */
	void
	nl_route_change(cnetlink const* netlink, croute const& route);
#endif
};

}; // end of namespace vmcore


#endif /* VMCORE_H_ */
