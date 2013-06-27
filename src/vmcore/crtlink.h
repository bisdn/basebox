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
#include <map>
#include <set>

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

#include <crtaddr.h>

namespace dptmap
{

class crtlink
{
private:


	std::map<int, std::set<crtaddr> >  addrs;	// key: address family, value: set of addresses


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


public:


	/**
	 *
	 */
	void
	addr_add(crtaddr const& rta);


	/**
	 *
	 */
	void
	addr_del(crtaddr const& rta);



public:


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


	/**
	 *
	 */
	class crtlink_find_by_ifindex : public std::unary_function<crtlink,bool> {
		unsigned int ifindex;
	public:
		crtlink_find_by_ifindex(unsigned int ifindex) :
			ifindex(ifindex) {};
		bool operator() (crtlink const& rtl) {
			return (ifindex == rtl.ifindex);
		};
		bool operator() (std::pair<unsigned int, crtlink> const& p) {
			return (ifindex == p.second.ifindex);
		};
	};


	/**
	 *
	 */
	class crtlink_find_by_devname : public std::unary_function<crtlink,bool> {
		std::string devname;
	public:
		crtlink_find_by_devname(std::string const& devname) :
			devname(devname) {};
		bool operator() (crtlink const& rtl) {
			return (devname == rtl.devname);
		};
		bool operator() (std::pair<unsigned int, crtlink> const& p) {
			return (devname == p.second.devname);
		};
	};
};

}; // end of namespace dptmap

#endif /* CRTLINK_H_ */
