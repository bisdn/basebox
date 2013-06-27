/*
 * crtaddr.h
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */

#ifndef CRTADDR_H_
#define CRTADDR_H_ 1

#include <ostream>
#include <string>

#include <rofl/common/caddress.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/object.h>
#include <netlink/route/addr.h>
#ifdef __cplusplus
}
#endif


namespace dptmap
{

class crtaddr
{
public:

	/**
	 *
	 */
	crtaddr();


	/**
	 *
	 */
	virtual
	~crtaddr();


	/**
	 *
	 */
	crtaddr(crtaddr const& rtaddr);


	/**
	 *
	 */
	crtaddr&
	operator= (crtaddr const& rtaddr);


	/**
	 *
	 */
	crtaddr(struct rtnl_addr* addr);

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtaddr const& rtaddr)
	{
		os << "crtaddr{" <<
				""
				<< "}";
		return os;
	};
};

}; // end of namespace

#endif /* CRTADDR_H_ */
