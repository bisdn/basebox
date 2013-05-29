/*
 * vmcore.h
 *
 *  Created on: 29.05.2013
 *      Author: andi
 */

#ifndef VMCORE_H_
#define VMCORE_H_ 1

#include "cnetlink.h"

class vmcore :
	public cnetlink_owner
{
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


public: // overloaded from cnetlink_owner


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

};


#endif /* VMCORE_H_ */
