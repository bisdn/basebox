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


namespace ethercore
{

class crtaddr
{
public:

	enum addr_index_t {
		CRTLINK_ADDR_ALL = 0xffff,	// apply command to all addresses
	};


	std::string			label;
	int					ifindex;
	int					af;
	int					prefixlen;
	rofl::caddress		mask;
	int					scope;
	int					flags;
	rofl::caddress		local;
	rofl::caddress		peer;
	rofl::caddress		bcast;


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
	bool
	operator< (crtaddr const& rtaddr);


public:


	/**
	 *
	 */
	std::string get_label() const { return label; };


	/**
	 *
	 */
	int get_ifindex() const { return ifindex; };


	/**
	 *
	 */
	int get_family() const { return af; };


	/**
	 *
	 */
	std::string get_family_s() const;


	/**
	 *
	 */
	int get_prefixlen() const { return prefixlen; };


	/**
	 *
	 */
	int get_scope() const { return scope; };


	/**
	 *
	 */
	int get_flags() const { return flags; };


	/**
	 *
	 */
	rofl::caddress get_local_addr() const { return local; };


	/**
	 *
	 */
	rofl::caddress get_peer_addr() const { return peer; };


	/**
	 *
	 */
	rofl::caddress get_broadcast_addr() const { return bcast; };


	/**
	 *
	 */
	rofl::caddress get_mask() const { return mask; };


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, crtaddr const& rtaddr) {
		os << rofl::indent(0) << "<crtaddr label: " << rtaddr.label << " ifindex: " << rtaddr.ifindex << " >" << std::endl;
		os << rofl::indent(2) << "<af: " << rtaddr.get_family_s() << " prefixlen: " << rtaddr.prefixlen << " >" << std::endl;
		os << rofl::indent(2) << "<mask: " << rtaddr.mask << " scope: " << rtaddr.scope << " flags: " << rtaddr.flags << " >" << std::endl;
		os << rofl::indent(2) << "<local: " << rtaddr.local << " peer: " << rtaddr.peer << " broadcast: " << rtaddr.bcast << " >" << std::endl;
		return os;
	};


	/**
	 *
	 */
	class crtaddr_find_by_local_addr : public std::unary_function<crtaddr,bool> {
		rofl::caddress local;
	public:
		crtaddr_find_by_local_addr(rofl::caddress const& local) :
			local(local) {};
		bool
		operator() (std::pair<uint16_t, crtaddr> const& p) {
			std::cerr << "crtaddr_find_by_local_addr => local: " << local << " p.local: " << p.second.local << std::endl;
			return (local == p.second.local);
		};
	};
};

}; // end of namespace

#endif /* CRTADDR_H_ */
