/*
 * crtroute.h
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#ifndef CRTROUTE_H_
#define CRTROUTE_H_ 1

#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/object.h>
#include <netlink/route/route.h>
#ifdef __cplusplus
}
#endif


namespace dptmap
{

class crtroute
{
public:

	/**
	 *
	 */
	crtroute();


	/**
	 *
	 */
	virtual
	~crtroute();


	/**
	 *
	 */
	crtroute(crtroute const& rtr);


	/**
	 *
	 */
	crtroute&
	operator= (crtroute const& rtr);


	/**
	 *
	 */
	crtroute(struct rtnl_route *route);


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtroute const& rtr)
	{
		// TODO
		return os;
	};
};

}; // end of namespace

#endif /* CRTROUTE_H_ */
