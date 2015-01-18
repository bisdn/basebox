/*
 * cethcore.hpp
 *
 *  Created on: 03.08.2014
 *      Author: andreas
 */

#ifndef CETHCORE_HPP_
#define CETHCORE_HPP_

#include <sys/types.h>
#include <sys/wait.h>

#include <inttypes.h>
#include <iostream>
#include <map>
#include <rofl/common/crofdpt.h>
#include <rofl/common/thread_helper.h>
#include <rofl/common/protocols/fetherframe.h>

#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/clogging.hpp"
#include "roflibs/ethcore/cvlan.hpp"
#include "roflibs/ethcore/cportdb.hpp"
#include "roflibs/netlink/ctundev.hpp"
#include "roflibs/netlink/ctapdev.hpp"
#include "roflibs/netlink/ccookiebox.hpp"

namespace roflibs {
namespace eth {

class eEthCoreBase : public std::runtime_error {
public:
	eEthCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eEthCoreNotFound : public eEthCoreBase {
public:
	eEthCoreNotFound(const std::string& __arg) : eEthCoreBase(__arg) {};
};
class eEthCoreExists : public eEthCoreBase {
public:
	eEthCoreExists(const std::string& __arg) : eEthCoreBase(__arg) {};
};
class eLinkNoDptAttached		: public eEthCoreBase {
public:
	eLinkNoDptAttached(const std::string& __arg) : eEthCoreBase(__arg) {};
};
class eLinkTapDevNotFound		: public eEthCoreBase {
public:
	eLinkTapDevNotFound(const std::string& __arg) : eEthCoreBase(__arg) {};
};


class cethcore :
		public rofcore::cnetdev_owner,
		public rofcore::cnetlink_common_observer,
		public roflibs::common::openflow::ccookie_owner {
public:

	/**
	 *
	 */
	static std::set<uint64_t>&
	get_eth_cores() {
		return dpids;
	};

	/**
	 *
	 */
	static cethcore&
	add_eth_core(const rofl::cdptid& dptid,
			uint8_t table_id_eth_in, uint8_t table_id_eth_src,
			uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
			uint16_t default_pvid = DEFAULT_PVID) {
		if (cethcore::ethcores.find(dptid) != cethcore::ethcores.end()) {
			delete cethcore::ethcores[dptid];
		}
		cethcore::ethcores[dptid] = new cethcore(dptid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, default_pvid);
		dpids.insert(rofl::crofdpt::get_dpt(dptid).get_dpid().get_uint64_t());
		return *(cethcore::ethcores[dptid]);
	};

	/**
	 *
	 */
	static cethcore&
	set_eth_core(const rofl::cdptid& dptid,
			uint8_t table_id_eth_in, uint8_t table_id_eth_src,
			uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
			uint16_t default_pvid = DEFAULT_PVID) {
		if (cethcore::ethcores.find(dptid) == cethcore::ethcores.end()) {
			cethcore::ethcores[dptid] = new cethcore(dptid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, default_pvid);
			dpids.insert(rofl::crofdpt::get_dpt(dptid).get_dpid().get_uint64_t());
		}
		return *(cethcore::ethcores[dptid]);
	};

	/**
	 *
	 */
	static cethcore&
	set_eth_core(const rofl::cdptid& dptid) {
		if (cethcore::ethcores.find(dptid) == cethcore::ethcores.end()) {
			throw eEthCoreNotFound("cethcore::set_eth_core() dpid not found");
		}
		return *(cethcore::ethcores[dptid]);
	};

	/**
	 *
	 */
	static const cethcore&
	get_eth_core(const rofl::cdptid& dptid) {
		if (cethcore::ethcores.find(dptid) == cethcore::ethcores.end()) {
			throw eEthCoreNotFound("cethcore::get_eth_core() dpid not found");
		}
		return *(cethcore::ethcores.at(dptid));
	};

	/**
	 *
	 */
	static void
	drop_eth_core(const rofl::cdptid& dptid) {
		if (cethcore::ethcores.find(dptid) == cethcore::ethcores.end()) {
			return;
		}
		dpids.erase(rofl::crofdpt::get_dpt(dptid).get_dpid().get_uint64_t());
		delete cethcore::ethcores[dptid];
		cethcore::ethcores.erase(dptid);
	};

	/**
	 *
	 */
	static bool
	has_eth_core(const rofl::cdptid& dptid) {
		return (not (cethcore::ethcores.find(dptid) == cethcore::ethcores.end()));
	};

private:

	/**
	 *
	 */
	cethcore(const rofl::cdptid& dptid,
			uint8_t table_id_eth_in, uint8_t table_id_eth_src,
			uint8_t table_id_eth_local, uint8_t table_id_eth_dst,
			uint16_t default_pvid = DEFAULT_PVID);

	/**
	 *
	 */
	virtual
	~cethcore();

public:

	/**
	 *
	 */
	const rofl::cdpid&
	get_dpid() const { return rofl::crofdpt::get_dpt(dptid).get_dpid(); };

	/**
	 *
	 */
	uint16_t
	get_default_pvid() const { return default_pvid; };

public:

	/**
	 *
	 */
	std::vector<uint16_t>
	get_vlans() const {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		std::vector<uint16_t> vids;
		for (std::map<uint16_t, cvlan>::const_iterator
				it = vlans.begin(); it != vlans.end(); ++it) {
			vids.push_back(it->first);
		}
		return vids;
	};

	/**
	 *
	 */
	void
	clear() { vlans.clear(); };

	/**
	 *
	 */
	cvlan&
	add_vlan(uint16_t vid) {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (vlans.find(vid) != vlans.end()) {
			vlans.erase(vid);
		}
		vlans[vid] = cvlan(dptid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, vid);
		if (STATE_ATTACHED == state) {
			vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
		}
		return vlans[vid];
	};

	/**
	 *
	 */
	cvlan&
	set_vlan(uint16_t vid) {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (vlans.find(vid) == vlans.end()) {
			vlans[vid] = cvlan(dptid, table_id_eth_in, table_id_eth_src, table_id_eth_local, table_id_eth_dst, vid);
			if (STATE_ATTACHED == state) {
				vlans[vid].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		}
		return vlans[vid];
	};

	/**
	 *
	 */
	void
	drop_vlan(uint16_t vid) {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (vlans.find(vid) == vlans.end()) {
			return;
		}
		if (STATE_ATTACHED == state) {
			rofl::crofdpt::get_dpt(dptid).release_group_id(vlans[vid].get_group_id());
		}
		vlans.erase(vid);
	};

	/**
	 *
	 */
	const cvlan&
	get_vlan(uint16_t vid) const {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		if (vlans.find(vid) == vlans.end()) {
			throw eVlanNotFound("cethcore::get_vlan() vid not found");
		}
		return vlans.at(vid);
	};

	/**
	 *
	 */
	bool
	has_vlan(uint16_t vid) const {
		rofl::RwLock rwlock(vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		return (not (vlans.find(vid) == vlans.end()));
	};

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
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg);

	/**
	 *
	 */
	virtual void
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

public:

	/*
	 * from ctapdev
	 */
	virtual void
	enqueue(
			rofcore::cnetdev *netdev, rofl::cpacket* pkt);

	virtual void
	enqueue(
			rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts);


private:

	/**
	 *
	 */
	void
	clear_tap_devs(const rofl::cdptid& dpid) {
		while (not devs[dpid].empty()) {
			std::map<std::string, rofcore::ctapdev*>::iterator it = devs[dpid].begin();
			try {
				hook_port_down(rofl::crofdpt::get_dpt(it->second->get_dptid()), it->second->get_devname());
			} catch (rofl::eRofDptNotFound& e) {}
			drop_tap_dev(dpid, it->first);
		}
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	add_tap_dev(const rofl::cdptid& dpid, const std::string& devname, uint16_t pvid, const rofl::caddress_ll& hwaddr) {
		if (devs[dpid].find(devname) != devs[dpid].end()) {
			delete devs[dpid][devname];
		}
		devs[dpid][devname] = new rofcore::ctapdev(this, dpid, devname, pvid, hwaddr);
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdptid& dpid, const std::string& devname, uint16_t pvid, const rofl::caddress_ll& hwaddr) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			devs[dpid][devname] = new rofcore::ctapdev(this, dpid, devname, pvid, hwaddr);
		}
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdptid& dpid, const std::string& devname) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			throw rofcore::eTunDevNotFound("cbasebox::set_tap_dev() devname not found");
		}
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) {
		std::map<std::string, rofcore::ctapdev*>::iterator it;
		if ((it = find_if(devs[dpid].begin(), devs[dpid].end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs[dpid].end()) {
			throw rofcore::eTunDevNotFound("cbasebox::set_tap_dev() hwaddr not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const rofl::cdptid& dpid, const std::string& devname) const {
		if (devs.find(dpid) == devs.end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() dpid not found");
		}
		if (devs.at(dpid).find(devname) == devs.at(dpid).end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() devname not found");
		}
		return *(devs.at(dpid).at(devname));
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) const {
		if (devs.find(dpid) == devs.end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() dpid not found");
		}
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.at(dpid).begin(), devs.at(dpid).end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs.at(dpid).end()) {
			throw rofcore::eTunDevNotFound("cbasebox::get_tap_dev() hwaddr not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(const rofl::cdptid& dpid, const std::string& devname) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			return;
		}
		delete devs[dpid][devname];
		devs[dpid].erase(devname);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) {
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs[dpid].begin(), devs[dpid].end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs[dpid].end()) {
			return;
		}
		delete it->second;
		devs[dpid].erase(it->first);
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const rofl::cdptid& dpid, const std::string& devname) const {
		if (devs.find(dpid) == devs.end()) {
			return false;
		}
		return (not (devs.at(dpid).find(devname) == devs.at(dpid).end()));
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const rofl::cdptid& dpid, const rofl::caddress_ll& hwaddr) const {
		if (devs.find(dpid) == devs.end()) {
			return false;
		}
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.at(dpid).begin(), devs.at(dpid).end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(dpid, hwaddr))) == devs.at(dpid).end()) {
			return false;
		}
		return true;
	};

private:

	/*
	 * event specific hooks
	 */
	void
	hook_port_up(const rofl::crofdpt& dpt, std::string const& devname);

	void
	hook_port_down(const rofl::crofdpt& dpt, std::string const& devname);

	void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cethcore& core) {
		rofl::RwLock rwlock(core.vlans_rwlock, rofl::RwLock::RWLOCK_READ);
		os << rofcore::indent(0) << "<cethcore default-pvid: " << (int)core.get_default_pvid() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<rofl::cdptid, std::map<std::string, rofcore::ctapdev*> >::const_iterator
				it = core.devs.begin(); it != core.devs.end(); ++it) {
			for (std::map<std::string, rofcore::ctapdev*>::const_iterator
					jt = it->second.begin(); jt != it->second.end(); ++jt) {
				os << *(jt->second);
			}
		}
		//rofcore::indent i(2);
		os << core.get_dpid();
		for (std::map<uint16_t, cvlan>::const_iterator
				it = core.vlans.begin(); it != core.vlans.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	enum cethcore_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_QUERYING = 3,
		STATE_ATTACHED = 4,
	};

	std::map<rofl::cdptid, std::map<std::string, rofcore::ctapdev*> > devs;
	static const uint16_t				DEFAULT_PVID = 1;

	static std::string 					script_path_port_up;
	static std::string 					script_path_port_down;

	cethcore_state_t					state;
	rofl::cdptid						dptid;
	uint64_t							cookie_grp_addr;
	uint64_t							cookie_miss_entry_src;
	uint64_t							cookie_miss_entry_local;
	uint64_t							cookie_redirect_inject;
	uint16_t							default_pvid;
	std::map<uint16_t, cvlan>			vlans;
	mutable rofl::PthreadRwLock			vlans_rwlock;
	std::set<uint32_t>					group_ids;
	uint8_t								table_id_eth_in;
	uint8_t								table_id_eth_src;
	uint8_t								table_id_eth_local;
	uint8_t								table_id_eth_dst;

	static std::set<uint64_t> 					dpids;
	static std::map<rofl::cdptid, cethcore*> 	ethcores;


private:

	/**
	 *
	 */
	void
	add_eth_endpnts();

	/**
	 *
	 */
	void
	add_phy_ports();

	/**
	 *
	 */
	void
	clear_phy_ports();
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CETHCORE_HPP_ */
