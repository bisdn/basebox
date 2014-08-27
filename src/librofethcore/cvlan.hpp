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

#include "clogging.hpp"
#include "cmemberport.hpp"
#include "cfibentry.hpp"

namespace rofeth {

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
		state(STATE_IDLE), vid(VID_NO_VLAN), group_id(0),
		in_stage_table_id(0), src_stage_table_id(1), dst_stage_table_id(2) {};

	/**
	 *
	 */
	cvlan(const rofl::cdpid& dpid, uint16_t vid = VID_NO_VLAN,
			uint8_t in_stage_table_id = 0, uint8_t src_stage_table_id = 1, uint8_t dst_stage_table_id = 2) :
		state(STATE_IDLE), dpid(dpid), vid(vid), group_id(0),
		in_stage_table_id(in_stage_table_id), src_stage_table_id(src_stage_table_id), dst_stage_table_id(dst_stage_table_id) {};

	/**
	 *
	 */
	~cvlan() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {}
	};

	/**
	 *
	 */
	cvlan(const cvlan& vlan) { *this = vlan; };

	/**
	 *
	 */
	cvlan&
	operator= (const cvlan& vlan) {
		if (this == &vlan)
			return *this;
		state 		= vlan.state;
		dpid 		= vlan.dpid;
		vid 		= vlan.vid;
		group_id 	= vlan.group_id;
		in_stage_table_id  = vlan.in_stage_table_id;
		src_stage_table_id = vlan.src_stage_table_id;
		dst_stage_table_id = vlan.dst_stage_table_id;
		ports.clear();
		for (std::map<uint32_t, cmemberport>::const_iterator
				it = vlan.ports.begin(); it != vlan.ports.end(); ++it) {
			add_port(it->second.get_port_no(), it->second.get_tagged());
		}
		fib.clear();
		for (std::map<rofl::caddress_ll, cfibentry>::const_iterator
				it = vlan.fib.begin(); it != vlan.fib.end(); ++it) {
			add_fib_entry(it->second.get_lladdr(), it->second.get_portno(), it->second.get_entry_timeout());
		}
		return *this;
	};

public:

	/**
	 *
	 */
	cmemberport&
	add_port(uint32_t portno, bool tagged = true) {
		if (ports.find(portno) != ports.end()) {
			ports.erase(portno);
		}
		ports[portno] = cmemberport(dpid, portno, vid, tagged, in_stage_table_id, dst_stage_table_id);
		if (STATE_ATTACHED == state) {
			ports[portno].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		update_group_entry_buckets();
		return ports[portno];
	};

	/**
	 *
	 */
	cmemberport&
	set_port(uint32_t portno) {
		if (ports.find(portno) == ports.end()) {
			throw eMemberPortNotFound("cvlan::set_port() portno not found");
		}
		return ports[portno];
	};

	/**
	 *
	 */
	cmemberport&
	set_port(uint32_t portno, bool tagged) {
		if (ports.find(portno) == ports.end()) {
			ports[portno] = cmemberport(dpid, portno, vid, tagged, in_stage_table_id, dst_stage_table_id);
			update_group_entry_buckets();
			if (STATE_ATTACHED == state) {
				ports[portno].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
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
		std::map<rofl::caddress_ll, cfibentry>::iterator it;
		while ((it = find_if(fib.begin(), fib.end(),
				cfibentry::cfibentry_find_by_portno(portno))) != fib.end()) {
			fib.erase(it);
		}
		ports.erase(portno);
		update_group_entry_buckets();
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
	add_fib_entry(const rofl::caddress_ll& lladdr, uint32_t portno, int entry_timeout = cfibentry::FIB_ENTRY_DEFAULT_TIMEOUT) {
		if (fib.find(lladdr) != fib.end()) {
			fib.erase(lladdr);
		}
		if (not has_port(portno)) {
			throw eFibEntryPortNotMember("cvlan::add_fib_entry() port is not member of this vlan");
		}
		fib[lladdr] = cfibentry(this, dpid, vid, portno, get_port(portno).get_tagged(), lladdr,
							entry_timeout, src_stage_table_id, dst_stage_table_id);
		if (STATE_ATTACHED == state) {
			fib[lladdr].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return fib[lladdr];
	};

	/**
	 *
	 */
	cfibentry&
	set_fib_entry(const rofl::caddress_ll& lladdr) {
		if (fib.find(lladdr) == fib.end()) {
			throw eFibEntryNotFound("cvlan::set_fib_entry() lladdr not found");
		}
		return fib[lladdr];
	};

	/**
	 *
	 */
	cfibentry&
	set_fib_entry(const rofl::caddress_ll& lladdr, uint32_t portno, int entry_timeout = cfibentry::FIB_ENTRY_DEFAULT_TIMEOUT) {
		if (fib.find(lladdr) == fib.end()) {
			if (not has_port(portno)) {
				throw eFibEntryPortNotMember("cvlan::set_fib_entry() port is not member of this vlan");
			}
			fib[lladdr] = cfibentry(this, dpid, vid, portno, get_port(portno).get_tagged(), lladdr,
										entry_timeout, src_stage_table_id, dst_stage_table_id);
			if (STATE_ATTACHED == state) {
				fib[lladdr].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
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
	const rofl::cdpid&
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
	update_group_entry_buckets(uint16_t command = rofl::openflow::OFPGC_MODIFY);

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
		for (std::map<rofl::caddress_ll, cfibentry>::const_iterator
				it = vlan.fib.begin(); it != vlan.fib.end(); ++it) {
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
	fib_expired(cfibentry& entry) {
		drop_fib_entry(entry.get_lladdr());
	};

private:

	enum dpt_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_ATTACHED = 3,
	};

	dpt_state_t			state;
	rofl::cdpid			dpid;
	uint16_t			vid;
	uint32_t			group_id;	// OFP group identifier for broadcasting frames for this vid

	std::map<uint32_t, cmemberport>			ports;
	std::map<rofl::caddress_ll, cfibentry>	fib;
	uint8_t				in_stage_table_id;
	uint8_t 			src_stage_table_id;
	uint8_t				dst_stage_table_id;
};

}; // end of namespace

#endif /* CVLAN_HPP_ */
