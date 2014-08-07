/*
 * cvlan.hpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#ifndef CVLAN_HPP_
#define CVLAN_HPP_

#include <inttypes.h>
#include <iostream>
#include <exception>
#include <map>
#include <rofl/common/crofdpt.h>

#include "logging.h"
#include "cmemberport.hpp"
#include "cdpid.hpp"
#include "cfibentry.hpp"

namespace ethcore {

class eVlanBase : public std::runtime_error {
public:
	eVlanBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eVlanNotFound : public eVlanBase {
public:
	eVlanNotFound(const std::string& __arg) : eVlanBase(__arg) {};
};
class eVlanExists : public eVlanBase {
public:
	eVlanExists(const std::string& __arg) : eVlanBase(__arg) {};
};


class cvlan : public cfibentry_owner {
public:

	enum cvlan_vid_type_t {
		VID_NO_VLAN = 0xffff,
	};

	/**
	 *
	 */
	cvlan() :
		state(STATE_IDLE), vid(VID_NO_VLAN), group_id(0) {};

	/**
	 *
	 */
	cvlan(const cdpid& dpid, uint16_t vid = VID_NO_VLAN, uint32_t group_id = 0) :
		state(STATE_IDLE), dpid(dpid), vid(vid), group_id(group_id) {};

	/**
	 *
	 */
	~cvlan() {
		if (STATE_ATTACHED == state) {
			// TODO
		}
	};

	/**
	 *
	 */
	cvlan(const cvlan& vlan)
		{ *this = vlan; };

	/**
	 *
	 */
	cvlan&
	operator= (const cvlan& vlan) {
		if (this == &vlan)
			return *this;
		dpid = vlan.dpid;
		vid = vlan.vid;
		group_id = vlan.group_id;
		return *this;
	};

public:

	/**
	 *
	 */
	cmemberport&
	add_port(uint32_t portno, bool tagged = true) {
		if (STATE_ATTACHED == state) {
			ports[portno].handle_dpt_close(rofl::crofdpt::get_dpt(dpid.get_dpid()));
		}
		if (ports.find(portno) != ports.end()) {
			ports[portno] = cmemberport(dpid, portno, vid, tagged);
		}
		if (STATE_ATTACHED == state) {
			ports[portno].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()));
		}
		return ports[portno];
	};

	/**
	 *
	 */
	cmemberport&
	set_port(uint32_t portno, bool tagged = true) {
		if (ports.find(portno) == ports.end()) {
			ports[portno] = cmemberport(dpid, portno, vid, tagged);
			if (STATE_ATTACHED == state) {
				ports[portno].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()));
			}
		}
		return ports[portno];
	};

	/**
	 *
	 */
	const cmemberport&
	get_port(uint32_t portno) const {
		if (ports.find(portno) == ports.end()) {
			throw eMemberPortNotFound("cvlan::get_port() portno not found");
		}
		return ports.at(portno);
	};

	/**
	 *
	 */
	void
	drop_port(uint32_t portno) {
		if (ports.find(portno) == ports.end()) {
			return;
		}
		ports.erase(portno);
	}

	/**
	 *
	 */
	bool
	has_port(uint32_t portno) const {
		return (not (ports.find(portno) == ports.end()));
	};

public:

	/**
	 *
	 */
	cfibentry&
	add_fib_entry(const rofl::caddress_ll& lladdr, uint32_t portno, bool tagged) {
		if (STATE_ATTACHED == state) {
			fib[lladdr].handle_dpt_close(rofl::crofdpt::get_dpt(dpid.get_dpid()));
		}
		if (fib.find(lladdr) != fib.end()) {
			fib[lladdr] = cfibentry(this, dpid, vid, portno, tagged, lladdr);
		}
		if (STATE_ATTACHED == state) {
			fib[lladdr].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()));
		}
		return fib[lladdr];
	};

	/**
	 *
	 */
	cfibentry&
	set_fib_entry(const rofl::caddress_ll& lladdr, uint32_t portno, bool tagged) {
		if (fib.find(lladdr) == fib.end()) {
			fib[lladdr] = cfibentry(this, dpid, vid, portno, tagged, lladdr);
			if (STATE_ATTACHED == state) {
				fib[lladdr].handle_dpt_open(rofl::crofdpt::get_dpt(dpid.get_dpid()));
			}
		}
		return fib[lladdr];
	};

	/**
	 *
	 */
	const cfibentry&
	get_fib_entry(const rofl::caddress_ll& lladdr) const {
		if (fib.find(lladdr) == fib.end()) {
			throw eFibEntryNotFound("cvlan::get_fib_entry() lladdr not found");
		}
		return fib.at(lladdr);
	};

	/**
	 *
	 */
	void
	drop_fib_entry(const rofl::caddress_ll& lladdr) {
		if (fib.find(lladdr) == fib.end()) {
			return;
		}
		fib.erase(lladdr);
	};

	/**
	 *
	 */
	bool
	has_fib_entry(const rofl::caddress_ll& lladdr) const {
		return (not (fib.find(lladdr) == fib.end()));
	};

public:

	/**
	 *
	 */
	const cdpid&
	get_dpid() const { return dpid; };

	/**
	 *
	 */
	uint16_t
	get_vid() const { return vid; };

	/**
	 *
	 */
	uint32_t
	get_group_id() const { return group_id; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open(
			rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close(
			rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg);

	/**
	 *
	 */
	void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg);

	/**
	 *
	 */
	void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	/**
	 *
	 */
	void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg);

private:

	/**
	 *
	 */
	void
	drop_buffer(
			const rofl::cauxid& auxid, uint32_t buffer_id);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cvlan& vlan) {
		os << rofcore::indent(0) << "<cvlan "
				<< "vid: " << (int)vlan.get_vid() << " "
				<< "group-id: " << (int)vlan.get_group_id() << " "
				<< " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<uint32_t, cmemberport>::const_iterator
				it = vlan.ports.begin(); it != vlan.ports.end(); ++it) {
			os << it->second;
		}
		return os;
	};

protected:

	friend class cfibentry;

	/**
	 *
	 */
	virtual void
	fib_expired(cfibentry& entry) {};

private:

	enum dpt_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_ATTACHED = 3,
	};

	dpt_state_t			state;
	cdpid				dpid;
	uint16_t			vid;
	uint32_t			group_id;	// OFP group identifier for broadcasting frames for this vid

	std::map<uint32_t, cmemberport>			ports;
	std::map<rofl::caddress_ll, cfibentry>	fib;
};

}; // end of namespace

#endif /* CVLAN_HPP_ */
