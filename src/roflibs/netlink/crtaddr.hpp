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
#include <algorithm>

#include <rofl/common/caddress.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>
#ifdef __cplusplus
}
#endif

#include "clogging.hpp"

namespace rofcore {


class crtaddr {
public:

	class eRtAddrBase		: public std::runtime_error {
	public:
		eRtAddrBase(const std::string& __arg) : std::runtime_error(__arg) {};
	};
	class eRtAddrNotFound	: public eRtAddrBase {
	public:
		eRtAddrNotFound(const std::string& __arg) : eRtAddrBase(__arg) {};
	};
	class eRtAddrExists		: public eRtAddrBase {
	public:
		eRtAddrExists(const std::string& __arg) : eRtAddrBase(__arg) {};
	};

public:

	/**
	 *
	 */
	crtaddr() :
		ifindex(0),
		af(0),
		prefixlen(0),
		scope(0),
		flags(0) {};

	/**
	 *
	 */
	crtaddr(struct rtnl_addr* addr) :
			ifindex(0),
			af(0),
			prefixlen(0),
			scope(0),
			flags(0)
	{
		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		nl_object_get((struct nl_object*)addr); // increment reference counter by one

		if (rtnl_addr_get_label(addr)) label.assign(rtnl_addr_get_label(addr)); // label might be null in case of IPv6 addresses
		ifindex 	= rtnl_addr_get_ifindex(addr);
		af 			= rtnl_addr_get_family(addr);
		prefixlen 	= rtnl_addr_get_prefixlen(addr);
		scope 		= rtnl_addr_get_scope(addr);
		flags 		= rtnl_addr_get_flags(addr);

		nl_object_put((struct nl_object*)addr); // decrement reference counter by one
	};

	/**
	 *
	 */
	virtual
	~crtaddr() {};

	/**
	 *
	 */
	crtaddr(const crtaddr& rtaddr) { *this = rtaddr; };

	/**
	 *
	 */
	crtaddr&
	operator= (const crtaddr& rtaddr) {
		if (this == &rtaddr)
			return *this;

		label 		= rtaddr.label;
		ifindex 	= rtaddr.ifindex;
		af 			= rtaddr.af;
		prefixlen 	= rtaddr.prefixlen;
		scope 		= rtaddr.scope;
		flags 		= rtaddr.flags;

		return *this;
	};

	/**
	 *
	 */
	bool
	operator== (const crtaddr& rtaddr) {
		return ((label 		== rtaddr.label) 		&&
				(ifindex 	== rtaddr.ifindex) 		&&
				(af 		== rtaddr.af) 			&&
				(prefixlen 	== rtaddr.prefixlen) 	&&
				(scope 		== rtaddr.scope) 		&&
				(flags 		== rtaddr.flags));
	};

public:

	/**
	 *
	 */
	std::string
	get_label() const { return label; };

	/**
	 *
	 */
	int
	get_ifindex() const { return ifindex; };

	/**
	 *
	 */
	int
	get_family() const { return af; };

	/**
	 *
	 */
	std::string
	get_family_s() const {
		std::string str;

		switch (af) {
		case AF_INET:	str = std::string("inet"); 		break;
		case AF_INET6:	str = std::string("inet6");		break;
		default:		str = std::string("unknown"); 	break;
		}

		return str;
	};

	/**
	 *
	 */
	int
	get_prefixlen() const { return prefixlen; };

	/**
	 *
	 */
	int
	get_scope() const { return scope; };

	/**
	 *
	 */
	int
	get_flags() const { return flags; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, crtaddr const& rtaddr) {
		os << rofcore::indent(0) << "<crtaddr label: " << rtaddr.label << " ifindex: " << rtaddr.ifindex << " >" << std::endl;
		os << rofcore::indent(2) << "<af: " << rtaddr.get_family_s() << " prefixlen: " << rtaddr.prefixlen << " >" << std::endl;
		return os;
	};

private:

	std::string			label;
	int					ifindex;
	int					af;
	int					prefixlen;
	int					scope;
	int					flags;
};



/**
 *
 */
class crtaddr_find : public std::unary_function<crtaddr,bool> {
	crtaddr rtaddr;
public:
	crtaddr_find(const crtaddr& rtaddr) :
		rtaddr(rtaddr) {};
	bool operator() (const crtaddr& rta) {
		return (rtaddr == rta);
	};
	bool operator() (const std::pair<unsigned int, crtaddr>& p) {
		return (rtaddr == p.second);
	};
	bool operator() (const std::pair<unsigned int, crtaddr*>& p) {
		return (rtaddr == *(p.second));
	};
};




class crtaddr_in4 : public crtaddr {
public:

	/**
	 *
	 */
	static crtaddr_in4&
	get_addr_in4(unsigned int adindex);

public:

	/**
	 *
	 */
	crtaddr_in4() {};

	/**
	 *
	 */
	virtual
	~crtaddr_in4() {};

	/**
	 *
	 */
	crtaddr_in4(
			struct rtnl_addr* addr) :
				crtaddr(addr) {

		nl_object_get((struct nl_object*)addr); // increment reference counter by one

		mask.set_addr_hbo(~((1 << (32 - crtaddr::get_prefixlen())) - 1));

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_local(nl_addr2str(rtnl_addr_get_local(addr), s_buf, sizeof(s_buf)));
		if (s_local != "none") {
			s_local = s_local.substr(0, s_local.find_first_of("/", 0));
			local 		= rofl::caddress_in4(s_local.c_str());
		}

		std::string s_peer (nl_addr2str(rtnl_addr_get_peer(addr), s_buf, sizeof(s_buf)));
		if (s_peer != "none") {
			s_peer  = s_peer .substr(0,  s_peer.find_first_of("/", 0));
			peer 		= rofl::caddress_in4(s_peer.c_str());
		}

		std::string s_bcast(nl_addr2str(rtnl_addr_get_broadcast(addr), s_buf, sizeof(s_buf)));
		if (s_bcast != "none") {
			s_bcast = s_bcast.substr(0, s_bcast.find_first_of("/", 0));
			bcast 		= rofl::caddress_in4(s_bcast.c_str());
		}

		nl_object_put((struct nl_object*)addr); // decrement reference counter by one
	};

	/**
	 *
	 */
	bool
	operator< (
			const crtaddr_in4& addr) const {
		return (local < addr.local);
	};

	/**
	 *
	 */
	bool
	operator== (const crtaddr_in4& rtaddr) {
		return ((crtaddr::operator== (rtaddr))	&&
				(local 	== rtaddr.local) 		&&
				(peer 	== rtaddr.peer) 		&&
				(bcast 	== rtaddr.bcast) 		&&
				(mask 	== rtaddr.mask));
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_local_addr() const { return local; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_peer_addr() const { return peer; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_broadcast_addr() const { return bcast; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_mask() const { return mask; };


public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtaddr_in4& rtaddr) {
		os << rofcore::indent(0) << "<crtaddr_in4 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const crtaddr&>(rtaddr); };
		os << rofcore::indent(2) << "<local: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_local_addr();
		os << rofcore::indent(2) << "<peer: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_peer_addr();
		os << rofcore::indent(2) << "<broadcast: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_broadcast_addr();
		os << rofcore::indent(2) << "<mask: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_mask();
		return os;
	};

	std::string
	str() const {
	/*
		90: ep0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN qlen 500
			link/ether 00:ff:ff:11:11:11 brd ff:ff:ff:ff:ff:ff
			inet 10.1.3.30/24 brd 10.1.3.255 scope global ep0
			inet6 fe80::2ff:ffff:fe11:1111/64 scope link
			   valid_lft forever preferred_lft forever
	 */
		std::stringstream ss;
		ss << "addr/inet " << local.str() << "/" << get_prefixlen() << " brd " << bcast.str();
		ss << " scope " << get_scope() << " ";
		return ss.str();
	};

	/**
	 *
	 */
	class crtaddr_in4_find_by_local_addr {
		rofl::caddress_in4 local;
	public:
		crtaddr_in4_find_by_local_addr(const rofl::caddress_in4& local) :
			local(local) {};
		bool
		operator() (const std::pair<uint16_t, crtaddr_in4>& p) {
			return (local == p.second.local);
		};
	};

private:

	rofl::caddress_in4		mask;
	rofl::caddress_in4		local;
	rofl::caddress_in4		peer;
	rofl::caddress_in4		bcast;

};


/**
 *
 */
class crtaddr_in4_find : public std::unary_function<crtaddr_in4,bool> {
	crtaddr_in4 rtaddr;
public:
	crtaddr_in4_find(const crtaddr_in4& rtaddr) :
		rtaddr(rtaddr) {};
	bool operator() (const crtaddr_in4& rta) {
		return (rtaddr == rta);
	};
	bool operator() (const std::pair<unsigned int, crtaddr_in4>& p) {
		return (rtaddr == p.second);
	};
#if 0
	bool operator() (const std::pair<unsigned int, crtaddr_in4*>& p) {
		return (rtaddr == *(p.second));
	};
#endif
};




class crtaddr_in6 : public crtaddr {
public:

	/**
	 *
	 */
	static crtaddr_in6&
	get_addr_in6(unsigned int adindex);

public:

	/**
	 *
	 */
	crtaddr_in6() {};

	/**
	 *
	 */
	virtual
	~crtaddr_in6() {};

	/**
	 *
	 */
	crtaddr_in6(
			struct rtnl_addr* addr) :
				crtaddr(addr) {

		nl_object_get((struct nl_object*)addr); // increment reference counter by one

		int segment 	= crtaddr::get_prefixlen() / 8;
		int t_prefixlen = crtaddr::get_prefixlen() % 8;
		for (int i = 0; i < 16; i++) {
			if (segment == i) {
				mask[i] = ~((1 << (8 - t_prefixlen)) - 1);
			} else if (i > segment) {
				mask[i] = 0;
			} else if (i < segment) {
				mask[i] = 0xff;
			}
		}

		char s_buf[128];
		memset(s_buf, 0, sizeof(s_buf));

		std::string s_local(nl_addr2str(rtnl_addr_get_local(addr), s_buf, sizeof(s_buf)));
		if (s_local != "none") {
			s_local = s_local.substr(0, s_local.find_first_of("/", 0));
			local 	= rofl::caddress_in6(s_local.c_str());
		}

		std::string s_peer (nl_addr2str(rtnl_addr_get_peer(addr), s_buf, sizeof(s_buf)));
		if (s_peer != "none") {
			s_peer  = s_peer .substr(0,  s_peer.find_first_of("/", 0));
			peer	= rofl::caddress_in6(s_peer.c_str());
		}

		std::string s_bcast(nl_addr2str(rtnl_addr_get_broadcast(addr), s_buf, sizeof(s_buf)));
		if (s_bcast != "none") {
			s_bcast = s_bcast.substr(0, s_bcast.find_first_of("/", 0));
			bcast	= rofl::caddress_in6(s_bcast.c_str());
		}

		nl_object_put((struct nl_object*)addr); // decrement reference counter by one
	};

	/**
	 *
	 */
	bool
	operator< (
			const crtaddr_in6& addr) const {
		return (local < addr.local);
	};

	/**
	 *
	 */
	bool
	operator== (const crtaddr_in6& rtaddr) {
		return ((crtaddr::operator== (rtaddr))	&&
				(local 	== rtaddr.local) 		&&
				(peer 	== rtaddr.peer) 		&&
				(bcast 	== rtaddr.bcast) 		&&
				(mask 	== rtaddr.mask));
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_local_addr() const { return local; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_peer_addr() const { return peer; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_broadcast_addr() const { return bcast; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_mask() const { return mask; };


public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtaddr_in6& rtaddr) {
		os << rofcore::indent(0) << "<crtaddr_in6 >" << std::endl;
		{ rofcore::indent i(2); os << dynamic_cast<const crtaddr&>(rtaddr); };
		os << rofcore::indent(2) << "<local: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_local_addr();
		os << rofcore::indent(2) << "<peer: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_peer_addr();
		os << rofcore::indent(2) << "<broadcast: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_broadcast_addr();
		os << rofcore::indent(2) << "<mask: >" << std::endl;
		os << rofcore::indent(4) << rtaddr.get_mask();
		return os;
	};


	std::string
	str() const {
	/*
		90: ep0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN qlen 500
			link/ether 00:ff:ff:11:11:11 brd ff:ff:ff:ff:ff:ff
			inet 10.1.3.30/24 brd 10.1.3.255 scope global ep0
			inet6 fe80::2ff:ffff:fe11:1111/64 scope link
			   valid_lft forever preferred_lft forever
	 */
		std::stringstream ss;
		ss << "addr/inet6 " << local.str() << "/" << get_prefixlen() << " brd " << bcast.str();
		ss << " scope " << get_scope() << " ";
		return ss.str();
	};


	/**
	 *
	 */
	class crtaddr_in6_find_by_local_addr {
		rofl::caddress_in6 local;
	public:
		crtaddr_in6_find_by_local_addr(const rofl::caddress_in6& local) :
			local(local) {};
		bool
		operator() (const std::pair<uint16_t, crtaddr_in6>& p) {
			return (local == p.second.local);
		};
	};

private:

	rofl::caddress_in6		mask;
	rofl::caddress_in6		local;
	rofl::caddress_in6		peer;
	rofl::caddress_in6		bcast;

};


/**
 *
 */
class crtaddr_in6_find : public std::unary_function<crtaddr_in6,bool> {
	crtaddr_in6 rtaddr;
public:
	crtaddr_in6_find(const crtaddr_in6& rtaddr) :
		rtaddr(rtaddr) {};
	bool operator() (const crtaddr_in6& rta) {
		return (rtaddr == rta);
	};
	bool operator() (const std::pair<unsigned int, crtaddr_in6>& p) {
		return (rtaddr == p.second);
	};
#if 0
	bool operator() (const std::pair<unsigned int, crtaddr_in6*>& p) {
		return (rtaddr == *(p.second));
	};
#endif
};

}; // end of namespace

#endif /* CRTADDR_H_ */
