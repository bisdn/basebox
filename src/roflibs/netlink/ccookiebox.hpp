/*
 * ccookies.hpp
 *
 *  Created on: 19.11.2014
 *      Author: andreas
 */

#ifndef CCOOKIES_HPP_
#define CCOOKIES_HPP_

#include <inttypes.h>

#include <ostream>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cauxid.h>
#include <rofl/common/crandom.h>
#include <rofl/common/openflow/messages/cofmsg_packet_in.h>
#include <rofl/common/openflow/messages/cofmsg_flow_removed.h>

#include "roflibs/netlink/clogging.hpp"

namespace roflibs {
namespace common {
namespace openflow {

class ccookiebox; // forward declaration, see below

class ccookie_owner {
public:

	/**
	 *
	 */
	virtual
	~ccookie_owner();

public:

	/**
	 *
	 */
	uint64_t
	acquire_cookie();

	/**
	 *
	 */
	void
	release_cookie(uint64_t cookie);

protected:

	friend class ccookiebox;

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) = 0;

	/**
	 *
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) = 0;
};



class ccookie_find_by_owner {
	ccookie_owner* owner;
public:
	ccookie_find_by_owner(ccookie_owner* owner) :
		owner(owner) {};
	bool operator() (const std::pair<uint64_t, ccookie_owner*>& p) {
		return (p.second == owner);
	};
};



class ccookiebox {
private:

	static ccookiebox* 					cookiebox;
	uint64_t 							next_cookie;
	std::map<uint64_t, ccookie_owner*> 	cookiestore;
	ccookiebox() : next_cookie(rofl::crandom(8).uint64()) {};
	~ccookiebox() {};

public:

	/**
	 *
	 */
	static ccookiebox&
	get_instance() {
		if ((ccookiebox*)0 == ccookiebox::cookiebox) {
			ccookiebox::cookiebox = new ccookiebox();
		}
		return *(ccookiebox::cookiebox);
	};

	/**
	 *
	 */
	void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {
		if (cookiestore.find(msg.get_cookie()) == cookiestore.end()) {
			rofcore::logging::debug << "[ccookiebox][handle_packet_in] cookie: 0x"
					<< std::hex << (unsigned long long)msg.get_cookie() << std::dec
					<< " not found, dropping packet" << std::endl << *this;
			return;
		}
		cookiestore[msg.get_cookie()]->handle_packet_in(dpt, auxid, msg);
	};

	/**
	 *
	 */
	void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {
		if (cookiestore.find(msg.get_cookie()) == cookiestore.end()) {
			rofcore::logging::debug << "[ccookiebox][handle_flow_removed] cookie: 0x"
					<< std::hex << (unsigned long long)msg.get_cookie() << std::dec
					<< " not found, dropping packet" << std::endl << *this;
			return;
		}
		cookiestore[msg.get_cookie()]->handle_flow_removed(dpt, auxid, msg);
	};

private:

	friend class ccookie_owner;

	/**
	 *
	 */
	void
	deregister_cookie_owner(ccookie_owner* owner) {
		std::map<uint64_t, ccookie_owner*>::iterator it;
		while ((it = find_if(cookiestore.begin(), cookiestore.end(),
				ccookie_find_by_owner(owner))) != cookiestore.end()) {
			cookiestore.erase(it);
		}
	};

	/**
	 *
	 */
	uint64_t
	acquire_cookie(ccookie_owner* owner) {
		do {
			next_cookie++;
		} while (cookiestore.find(next_cookie) != cookiestore.end());
		cookiestore[next_cookie] = owner;
		return next_cookie;
	};

	/**
	 *
	 */
	void
	release_cookie(ccookie_owner* owner, uint64_t cookie) {
		if (cookiestore.find(cookie) == cookiestore.end()) {
			return;
		}
		if (cookiestore[cookie] != owner) {
			return;
		}
		cookiestore.erase(cookie);
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const ccookiebox& box) {
		os << rofcore::indent(0) << "<ccookiebox >" << std::endl;
		for (std::map<uint64_t, ccookie_owner*>::const_iterator
				it = box.cookiestore.begin(); it != box.cookiestore.end(); ++it) {
			os << rofcore::indent(2) << "cookie: 0x"
					<< std::hex << (unsigned long long)it->first << std::dec << " => 0x"
					<< std::hex << (unsigned long)it->second << std::dec << std::endl;
		}
		return os;
	};
};

}; // end of namespace openflow
}; // end of namespace common
}; // end of namespace roflibs

#endif /* CCOOKIES_HPP_ */
