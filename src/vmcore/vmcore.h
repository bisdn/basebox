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

#include <cnetdev.h>
#include <ctapdev.h>
#include <cnetlink.h>

namespace dptmap
{

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};
class eVmCoreTapDevNotFound	: public eVmCoreBase {};

class vmcore :
	/*public cnetlink_owner,*/
		public rofl::crofbase,
		public cnetdev_owner,
		public cnetlink_subscriber
{
private:


	rofl::cofdpt *dpt;	// handle for cofdpt instance managed by this vmcore
	std::map<rofl::cofdpt*, std::map<uint32_t, ctapdev*> > tapdevs;	// map of tap interfaces for dpt


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

	virtual void
	enqueue(cnetdev *netdev, rofl::cpacket* pkt);

	virtual void
	enqueue(cnetdev *netdev, std::vector<rofl::cpacket*> pkts);

	virtual void
	linkcache_updated();

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
