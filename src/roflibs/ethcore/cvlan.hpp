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
#include <rofl/common/thread_helper.h>

#include "roflibs/netlink/clogging.hpp"
#include "roflibs/ethcore/cmemberport.hpp"
#include "roflibs/ethcore/cfibentry.hpp"
#include "roflibs/ethcore/cethendpnt.hpp"

namespace roflibs {
namespace eth {

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
		table_id_eth_in(0), table_id_eth_src(0), table_id_eth_local(0), table_id_eth_dst(0) {};

	/**
	 *
	 */
	cvlan(const rofl::cdpid& dpid,
			uint8_t table_id_eth_in, uint8_t table_id_eth_src,
			uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
			uint16_t vid) :
		state(STATE_IDLE), dpid(dpid), vid(vid), group_id(0),
		table_id_eth_in(table_id_eth_in), table_id_eth_src(table_id_eth_src),
		table_id_eth_local(table_id_eth_local), table_id_eth_dst(table_id_eth_dst) {};

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
		table_id_eth_in  	= vlan.table_id_eth_in;
		table_id_eth_src 	= vlan.table_id_eth_src;
		table_id_eth_local 	= vlan.table_id_eth_local;
		table_id_eth_dst 	= vlan.table_id_eth_dst;
		phyports.clear();
		for (std::map<uint32_t, cmemberport>::const_iterator
				it = vlan.phyports.begin(); it != vlan.phyports.end(); ++it) {
			add_phy_port(it->second.get_port_no(), it->second.get_hwaddr(), it->second.get_tagged());
		}
		fib.clear();
		for (std::map<rofl::caddress_ll, cfibentry>::const_iterator
				it = vlan.fib.begin(); it != vlan.fib.end(); ++it) {
			add_fib_entry(it->second.get_lladdr(), it->second.get_portno(),
					it->second.is_permanent(), it->second.get_entry_timeout());
		}
		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear_phy_ports() {
		std::vector<uint32_t> phy_ports = get_phy_ports();
		for (std::vector<uint32_t>::const_iterator
				it = phy_ports.begin(); it != phy_ports.end(); ++it) {
			drop_phy_port(*it);
		}
	};

	/**
	 *
	 */
	std::vector<uint32_t>
	get_phy_ports() const {
		rofl::RwLock rwlock(phyports_rwlock, rofl::RwLock::RWLOCK_READ);
		std::vector<uint32_t> portnos;
		for (std::map<uint32_t, cmemberport>::const_iterator
				it = phyports.begin(); it != phyports.end(); ++it) {
			portnos.push_back(it->first);
		}
		return portnos;
	};

	/**
	 *
	 */
	cmemberport&
	add_phy_port(uint32_t portno, const rofl::cmacaddr& hwaddr, bool tagged = true) {
		rofl::RwLock rwlock(phyports_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (phyports.find(portno) != phyports.end()) {
			phyports.erase(portno);
		}
		phyports[portno] = cmemberport(dpid, table_id_eth_in, table_id_eth_local, table_id_eth_dst, portno, hwaddr, vid, tagged);
		if (STATE_ATTACHED == state) {
			phyports[portno].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		update_group_entry_buckets();
		return phyports[portno];
	};

	/**
	 *
	 */
	cmemberport&
	set_phy_port(uint32_t portno) {
		rofl::RwLock rwlock(phyports_rwlock, rofl::RwLock::RWLOCK_READ);
		if (phyports.find(portno) == phyports.end()) {
			throw eMemberPortNotFound("cvlan::set_port() portno not found");
		}
		return phyports[portno];
	};

	/**
	 *
	 */
	cmemberport&
	set_phy_port(uint32_t portno, const rofl::cmacaddr& hwaddr, bool tagged) {
		rofl::RwLock rwlock(phyports_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (phyports.find(portno) == phyports.end()) {
			phyports[portno] = cmemberport(dpid, table_id_eth_in, table_id_eth_local, table_id_eth_dst, portno, hwaddr, vid, tagged);
			update_group_entry_buckets();
			if (STATE_ATTACHED == state) {
				phyports[portno].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return phyports[portno];
	};

	/**
	 *
	 */
	const cmemberport&
	get_phy_port(uint32_t portno) const {
		rofl::RwLock rwlock(phyports_rwlock, rofl::RwLock::RWLOCK_READ);
		if (phyports.find(portno) == phyports.end()) {
			throw eMemberPortNotFound("cvlan::get_port() portno not found");
		}
		return phyports.at(portno);
	};

	/**
	 *
	 */
	void
	drop_phy_port(uint32_t portno) {
		rofl::RwLock rwlock(phyports_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (phyports.find(portno) == phyports.end()) {
			return;
		}
		std::map<rofl::caddress_ll, cfibentry>::iterator it;
		while ((it = find_if(fib.begin(), fib.end(),
				cfibentry::cfibentry_find_by_portno(portno))) != fib.end()) {
			fib.erase(it);
		}
		phyports.erase(portno);
		update_group_entry_buckets();
	}

	/**
	 *
	 */
	bool
	has_phy_port(uint32_t portno) const {
		rofl::RwLock rwlock(phyports_rwlock, rofl::RwLock::RWLOCK_READ);
		return (not (phyports.find(portno) == phyports.end()));
	};

public:

	/**
	 *
	 */
	std::vector<rofl::caddress_ll>
	get_eth_endpnts() const {
		rofl::RwLock rwlock(ethendpnts_rwlock, rofl::RwLock::RWLOCK_READ);
		std::vector<rofl::caddress_ll> hwaddrs;
		for (std::map<rofl::caddress_ll, cethendpnt>::const_iterator
				it = ethendpnts.begin(); it != ethendpnts.end(); ++it) {
			hwaddrs.push_back(it->first);
		}
		return hwaddrs;
	};

	/**
	 *
	 */
	cethendpnt&
	add_eth_endpnt(const rofl::caddress_ll& hwaddr, bool tagged = true) {
		rofl::RwLock rwlock(ethendpnts_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (ethendpnts.find(hwaddr) != ethendpnts.end()) {
			ethendpnts.erase(hwaddr);
		}
		ethendpnts[hwaddr] = cethendpnt(dpid, table_id_eth_local, hwaddr, vid, tagged);
		if (STATE_ATTACHED == state) {
			ethendpnts[hwaddr].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		//update_group_entry_buckets(); // NO!!!
		return ethendpnts[hwaddr];
	};

	/**
	 *
	 */
	cethendpnt&
	set_eth_endpnt(const rofl::caddress_ll& hwaddr) {
		rofl::RwLock rwlock(ethendpnts_rwlock, rofl::RwLock::RWLOCK_READ);
		if (ethendpnts.find(hwaddr) == ethendpnts.end()) {
			throw eEthEndpntNotFound("cvlan::set_eth_endpnt() hwaddr not found");
		}
		return ethendpnts[hwaddr];
	};

	/**
	 *
	 */
	cethendpnt&
	set_eth_endpnt(const rofl::caddress_ll& hwaddr, bool tagged) {
		rofl::RwLock rwlock(ethendpnts_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (ethendpnts.find(hwaddr) == ethendpnts.end()) {
			ethendpnts[hwaddr] = cethendpnt(dpid, table_id_eth_local, hwaddr, vid, tagged);
			//update_group_entry_buckets(); // NO!!!
			if (STATE_ATTACHED == state) {
				ethendpnts[hwaddr].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return ethendpnts[hwaddr];
	};

	/**
	 *
	 */
	const cethendpnt&
	get_eth_endpnt(const rofl::caddress_ll& hwaddr) const {
		rofl::RwLock rwlock(ethendpnts_rwlock, rofl::RwLock::RWLOCK_READ);
		if (ethendpnts.find(hwaddr) == ethendpnts.end()) {
			throw eEthEndpntNotFound("cvlan::get_eth_endpnt() portno not found");
		}
		return ethendpnts.at(hwaddr);
	};

	/**
	 *
	 */
	void
	drop_eth_endpnt(const rofl::caddress_ll& hwaddr) {
		rofl::RwLock rwlock(ethendpnts_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (ethendpnts.find(hwaddr) == ethendpnts.end()) {
			return;
		}
		ethendpnts.erase(hwaddr);
		//update_group_entry_buckets(); // NO!!!
	}

	/**
	 *
	 */
	bool
	has_eth_endpnt(const rofl::caddress_ll& hwaddr) const {
		rofl::RwLock rwlock(ethendpnts_rwlock, rofl::RwLock::RWLOCK_READ);
		return (not (ethendpnts.find(hwaddr) == ethendpnts.end()));
	};

public:

	/**
	 *
	 */
	cfibentry&
	add_fib_entry(const rofl::caddress_ll& lladdr, uint32_t portno,
			bool permanent = false, int entry_timeout = cfibentry::FIB_ENTRY_DEFAULT_TIMEOUT) {
		if (fib.find(lladdr) != fib.end()) {
			fib.erase(lladdr);
		}
		if (not has_phy_port(portno)) {
			throw eFibEntryPortNotMember("cvlan::add_fib_entry() port is not member of this vlan");
		}
		fib[lladdr] = cfibentry(this, dpid, vid, portno, get_phy_port(portno).get_tagged(), lladdr, permanent,
							entry_timeout, table_id_eth_src, table_id_eth_dst);
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
	set_fib_entry(const rofl::caddress_ll& lladdr, uint32_t portno,
			bool permanent = false, int entry_timeout = cfibentry::FIB_ENTRY_DEFAULT_TIMEOUT) {
		if (fib.find(lladdr) == fib.end()) {
			if (not has_phy_port(portno)) {
				throw eFibEntryPortNotMember("cvlan::set_fib_entry() port is not member of this vlan");
			}
			fib[lladdr] = cfibentry(this, dpid, vid, portno, get_phy_port(portno).get_tagged(), lladdr, permanent,
										entry_timeout, table_id_eth_src, table_id_eth_dst);
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
		rofl::RwLock rwlock(vlan.phyports_rwlock, rofl::RwLock::RWLOCK_READ);
		os << rofcore::indent(0) << "<cvlan "
				<< "vid: " << (int)vlan.get_vid() << " "
				<< "group-id: " << (int)vlan.get_group_id() << " "
				<< " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<rofl::caddress_ll, cethendpnt>::const_iterator
				it = vlan.ethendpnts.begin(); it != vlan.ethendpnts.end(); ++it) {
			os << it->second;
		}
		for (std::map<uint32_t, cmemberport>::const_iterator
				it = vlan.phyports.begin(); it != vlan.phyports.end(); ++it) {
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

	std::map<rofl::caddress_ll, cethendpnt>	ethendpnts;
	mutable rofl::PthreadRwLock				ethendpnts_rwlock;
	std::map<uint32_t, cmemberport>			phyports;
	mutable rofl::PthreadRwLock				phyports_rwlock;
	std::map<rofl::caddress_ll, cfibentry>	fib;
	uint8_t				table_id_eth_in;
	uint8_t 			table_id_eth_src;
	uint8_t				table_id_eth_local;
	uint8_t				table_id_eth_dst;
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CVLAN_HPP_ */
