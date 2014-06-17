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
#include <rofl/common/logging.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>
#ifdef __cplusplus
}
#endif


namespace rofcore {

class crtaddr {
public:

	enum addr_index_t {
		CRTLINK_ADDR_ALL = 0xffff,	// apply command to all addresses
	};

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
	crtaddr(
			const crtaddr& addr);

	/**
	 *
	 */
	crtaddr&
	operator= (
			const crtaddr& addr);

	/**
	 *
	 */
	crtaddr(
			struct rtnl_addr* addr);

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
	get_family_s() const;

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
		os << rofl::indent(0) << "<crtaddr label: " << rtaddr.label << " ifindex: " << rtaddr.ifindex << " >" << std::endl;
		os << rofl::indent(2) << "<af: " << rtaddr.get_family_s() << " prefixlen: " << rtaddr.prefixlen << " >" << std::endl;
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



class crtaddr_in4 : public crtaddr {
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
			const crtaddr_in4& addr) { *this = addr; };

	/**
	 *
	 */
	crtaddr_in4&
	operator= (
			const crtaddr_in4& addr) {
		if (this == &addr)
			return *this;
		crtaddr::operator= (addr);
		mask	= addr.mask;
		local	= addr.local;
		peer	= addr.peer;
		bcast	= addr.bcast;
		return *this;
	};

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
		s_local = s_local.substr(0, s_local.find_first_of("/", 0));
		local 		= rofl::caddress_in4(s_local.c_str());

		std::string s_peer (nl_addr2str(rtnl_addr_get_peer(addr), s_buf, sizeof(s_buf)));
		s_peer  = s_peer .substr(0,  s_peer.find_first_of("/", 0));
		peer 		= rofl::caddress_in4(s_peer.c_str());

		std::string s_bcast(nl_addr2str(rtnl_addr_get_broadcast(addr), s_buf, sizeof(s_buf)));
		s_bcast = s_bcast.substr(0, s_bcast.find_first_of("/", 0));
		bcast 		= rofl::caddress_in4(s_bcast.c_str());

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
		os << rofl::indent(2) << "<crtaddr mask: " << rtaddr.mask << " >" << std::endl;
		os << rofl::indent(2) << "<local: " << rtaddr.local << " peer: " << rtaddr.peer << " broadcast: " << rtaddr.bcast << " >" << std::endl;
		return os;
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
			std::cerr << "crtaddr_find_by_local_addr => local: " << local << " p.local: " << p.second.local << std::endl;
			return (local == p.second.local);
		};
	};

private:

	rofl::caddress_in4		mask;
	rofl::caddress_in4		local;
	rofl::caddress_in4		peer;
	rofl::caddress_in4		bcast;

};




class crtaddr_in6 : public crtaddr {
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
			const crtaddr_in6& addr) { *this = addr; };

	/**
	 *
	 */
	crtaddr_in6&
	operator= (
			const crtaddr_in6& addr) {
		if (this == &addr)
			return *this;
		crtaddr::operator= (addr);
		mask	= addr.mask;
		local	= addr.local;
		peer	= addr.peer;
		bcast	= addr.bcast;
		return *this;
	};

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
		s_local = s_local.substr(0, s_local.find_first_of("/", 0));
		local 		= rofl::caddress_in6(s_local.c_str());

		std::string s_peer (nl_addr2str(rtnl_addr_get_peer(addr), s_buf, sizeof(s_buf)));
		s_peer  = s_peer .substr(0,  s_peer.find_first_of("/", 0));
		peer 		= rofl::caddress_in6(s_peer.c_str());

		std::string s_bcast(nl_addr2str(rtnl_addr_get_broadcast(addr), s_buf, sizeof(s_buf)));
		s_bcast = s_bcast.substr(0, s_bcast.find_first_of("/", 0));
		bcast 		= rofl::caddress_in6(s_bcast.c_str());

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
		os << rofl::indent(2) << "<crtaddr mask: " << rtaddr.mask << " >" << std::endl;
		os << rofl::indent(2) << "<local: " << rtaddr.local << " peer: " << rtaddr.peer << " broadcast: " << rtaddr.bcast << " >" << std::endl;
		return os;
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
			std::cerr << "crtaddr_find_by_local_addr => local: " << local << " p.local: " << p.second.local << std::endl;
			return (local == p.second.local);
		};
	};

private:

	rofl::caddress_in6		mask;
	rofl::caddress_in6		local;
	rofl::caddress_in6		peer;
	rofl::caddress_in6		bcast;

};


}; // end of namespace

#endif /* CRTADDR_H_ */
