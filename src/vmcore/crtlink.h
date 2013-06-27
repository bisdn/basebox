/*
 * crtlink.h
 *
 *  Created on: 27.06.2013
 *      Author: andreas
 */

#ifndef CRTLINK_H_
#define CRTLINK_H_ 1

#include <ostream>
#include <string>

#include <rofl/common/ciosrv.h>
#include <rofl/common/cmacaddr.h>
#include <rofl/common/cmemory.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <netlink/object.h>
#include <netlink/route/link.h>
#ifdef __cplusplus
}
#endif

namespace dptmap
{

class crtlink :
		public rofl::ciosrv
{
public:


	std::string				devname;	// device name (e.g. eth0)
	rofl::cmacaddr			maddr;		// mac address
	rofl::cmacaddr			bcast; 		// broadcast address
	unsigned int			flags;		// link flags
	int						af;			// address family (AF_INET, AF_UNSPEC, ...)
	unsigned int			arptype;	// ARP type (e.g. ARPHDR_ETHER)
	int						ifindex;	// interface index
	unsigned int			mtu;		// maximum transfer unit


public:

	/**
	 *
	 */
	crtlink();


	/**
	 *
	 */
	virtual
	~crtlink();


	/**
	 *
	 */
	crtlink(crtlink const& rtlink);


	/**
	 *
	 */
	crtlink&
	operator= (crtlink const& rtlink);


	/**
	 *
	 */
	crtlink(struct rtnl_link *link);


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtlink const& rtlink)
	{
		os << "crtlink{"
				<< "devname=" << rtlink.devname << " "
				<< "maddr=" << rtlink.maddr.c_str() << " "
				<< "bcast=" << rtlink.bcast.c_str() << " "
				<< "flags=" << rtlink.flags << " "
				<< "af=" << rtlink.af << " "
				<< "arptype=" << rtlink.arptype << " "
				<< "ifindex=" << rtlink.ifindex << " "
				<< "mtu=" << rtlink.mtu << " "
				<< "}";
		return os;
	};
};

}; // end of namespace dptmap

#endif /* CRTLINK_H_ */
