/*
 * vmcore.h
 *
 *  Created on: 29.05.2013
 *      Author: andi
 */

#ifndef VMCORE_H_
#define VMCORE_H_ 1

#include <map>

//#include "cnetlink.h"
#include <rofl/common/crofbase.h>

#include <cnetdev.h>
#include <ctapdev.h>

namespace dptmap
{

class vmcore :
	/*public cnetlink_owner,*/ public rofl::crofbase, public cnetdev_owner
{
private:


	std::map<rofl::cofdpt*, std::map<std::string, ctapdev*> > tapdevs;


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
