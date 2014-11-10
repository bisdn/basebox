/*
 * cbasebox.hpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#ifndef CROFBASE_HPP_
#define CROFBASE_HPP_

#include <sys/types.h>
#include <sys/wait.h>

#include <string>
#include <iostream>
#include <exception>
#include <rofl/common/crofbase.h>

#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#include <roflibs/flowcore/cflowcore.hpp>
#include <roflibs/ethcore/cethcore.hpp>
#include <roflibs/ipcore/cipcore.hpp>
#include <roflibs/gtpcore/cgtpcore.hpp>
#include <roflibs/gtpcore/cgtprelay.hpp>
#include <roflibs/grecore/cgrecore.hpp>
#include <roflibs/netlink/clogging.hpp>
#include <roflibs/netlink/cnetlink.hpp>
#include <roflibs/netlink/ctundev.hpp>
#include <roflibs/python/cpython.hpp>
#include <roflibs/ethcore/cportdb_file.hpp>

#include "cconfig.hpp"

namespace basebox {

class eBaseBoxBase : public std::runtime_error {
public:
	eBaseBoxBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eLinkNoDptAttached		: public eBaseBoxBase {
public:
	eLinkNoDptAttached(const std::string& __arg) : eBaseBoxBase(__arg) {};
};
class eLinkTapDevNotFound		: public eBaseBoxBase {
public:
	eLinkTapDevNotFound(const std::string& __arg) : eBaseBoxBase(__arg) {};
};

class cbasebox : public rofl::crofbase, public rofcore::cnetdev_owner, public rofcore::cnetlink_common_observer {

	/**
	 * @brief	pointer to singleton
	 */
	static cbasebox*	rofbase;

	static bool keep_on_running;

	/**
	 *
	 */
	cbasebox(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) :
						rofl::crofbase(versionbitmap),
						table_id_svc_flows(0),
						table_id_eth_port_membership(1),
						table_id_eth_src(2),
						table_id_eth_local(3),
						table_id_ip_local(4),
						table_id_gre_local(5),
						table_id_gtp_local(5),
						table_id_ip_fwd(6),
						table_id_eth_dst(7),
						default_pvid(1)
						{};

	/**
	 *
	 */
	virtual
	~cbasebox() {};

	/**
	 *
	 */
	cbasebox(
			const cbasebox& ethbase);

public:

	/**
	 *
	 */
	static cbasebox&
	get_instance(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) {
		if (cbasebox::rofbase == (cbasebox*)0) {
			cbasebox::rofbase = new cbasebox(versionbitmap);
		}
		return *(cbasebox::rofbase);
	};

	/**
	 *
	 */
	static int
	run(int argc, char** argv);

	/**
	 *
	 */
	static void
	stop();

protected:

	/**
	 *
	 */
	virtual void
	handle_dpt_open(
			rofl::crofdpt& dpt);

	/**
	 *
	 */
	virtual void
	handle_dpt_close(
			rofl::crofdpt& dpt);

	/**
	 *
	 */
	virtual void
	handle_features_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_features_reply& msg);

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
	virtual void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	/**
	 *
	 */
	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg);

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg);

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid);

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

	/**
	 *
	 */
	void
	set_python_script(const std::string& python_script) { this->python_script = python_script; };


private:

	/**
	 *
	 */
	void
	clear_tap_devs(const rofl::cdpid& dpid) {
		while (not devs[dpid].empty()) {
			std::map<std::string, rofcore::ctapdev*>::iterator it = devs[dpid].begin();
			try {
				hook_port_down(rofl::crofdpt::get_dpt(it->second->get_dpid()), it->second->get_devname());
			} catch (rofl::eRofDptNotFound& e) {}
			drop_tap_dev(it->second->get_dpid(), it->first);
		}
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	add_tap_dev(const rofl::cdpid& dpid, const std::string& devname, uint16_t pvid, const rofl::caddress_ll& hwaddr) {
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
	set_tap_dev(const rofl::cdpid& dpid, const std::string& devname, uint16_t pvid, const rofl::caddress_ll& hwaddr) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			devs[dpid][devname] = new rofcore::ctapdev(this, dpid, devname, pvid, hwaddr);
		}
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdpid& dpid, const std::string& devname) {
		if (devs[dpid].find(devname) == devs[dpid].end()) {
			throw rofcore::eTunDevNotFound("cbasebox::set_tap_dev() devname not found");
		}
		return *(devs[dpid][devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::cdpid& dpid, const rofl::caddress_ll& hwaddr) {
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
	get_tap_dev(const rofl::cdpid& dpid, const std::string& devname) const {
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
	get_tap_dev(const rofl::cdpid& dpid, const rofl::caddress_ll& hwaddr) const {
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
	drop_tap_dev(const rofl::cdpid& dpid, const std::string& devname) {
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
	drop_tap_dev(const rofl::cdpid& dpid, const rofl::caddress_ll& hwaddr) {
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
	has_tap_dev(const rofl::cdpid& dpid, const std::string& devname) const {
		if (devs.find(dpid) == devs.end()) {
			return false;
		}
		return (not (devs.at(dpid).find(devname) == devs.at(dpid).end()));
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const rofl::cdpid& dpid, const rofl::caddress_ll& hwaddr) const {
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

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbasebox& box) {
		os << rofcore::indent(0) << "<cbasebox>" << std::endl;
		rofcore::indent i(2);
		for (std::map<rofl::cdpid, std::map<std::string, rofcore::ctapdev*> >::const_iterator
				it = box.devs.begin(); it != box.devs.end(); ++it) {
			for (std::map<std::string, rofcore::ctapdev*>::const_iterator
					jt = it->second.begin(); jt != it->second.end(); ++jt) {
				os << *(jt->second);
			}
		}
		return os;
	};

private:

	friend class cnetlink;

	virtual void
	addr_in4_created(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in4_updated(unsigned int ifindex, uint16_t adindex) {};

	virtual void
	addr_in4_deleted(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in6_created(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in6_updated(unsigned int ifindex, uint16_t adindex) {};

	virtual void
	addr_in6_deleted(unsigned int ifindex, uint16_t adindex);

private:

	/*
	 * event specific hooks
	 */
	void
	hook_dpt_attach(const rofl::crofdpt& dpt);

	void
	hook_dpt_detach(const rofl::crofdpt& dpt);

	void
	hook_port_up(const rofl::crofdpt& dpt, std::string const& devname);

	void
	hook_port_down(const rofl::crofdpt& dpt, std::string const& devname);

	void
	set_forwarding(bool forward = true);

	static void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);

private:

	/**
	 *
	 */
	void
	test_workflow(rofl::crofdpt& dpt);

private:

	static const std::string BASEBOX_LOG_FILE;
	static const std::string BASEBOX_PID_FILE;
	static const std::string BASEBOX_CONFIG_FILE;

	static std::string script_path_dpt_open;
	static std::string script_path_dpt_close;
	static std::string script_path_port_up;
	static std::string script_path_port_down;

	std::map<rofl::cdpid, std::map<std::string, rofcore::ctapdev*> > devs;
	std::string	python_script;

	uint8_t table_id_svc_flows;
	uint8_t table_id_eth_port_membership;
	uint8_t table_id_eth_src;
	uint8_t table_id_eth_local;
	uint8_t table_id_ip_local;
	uint8_t table_id_gre_local;
	uint8_t table_id_gtp_local;
	uint8_t table_id_ip_fwd;
	uint8_t table_id_eth_dst;
	uint16_t default_pvid;
};

}; // end of namespace ethcore

#endif /* CROFBASE_HPP_ */
