/*
 * cbasebox.hpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#ifndef CROFBASE_HPP_
#define CROFBASE_HPP_

#include <string>
#include <iostream>
#include <exception>
#include <rofl/common/crofbase.h>

#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#include <roflibs/ethcore/cethcore.hpp>
#include <roflibs/ipcore/cipcore.hpp>
#include <roflibs/gtpcore/cgtpcore.hpp>
#include <roflibs/gtpcore/cgtprelay.hpp>
#include <roflibs/grecore/cgrecore.hpp>
#include <roflibs/netlink/clogging.hpp>
#include <roflibs/netlink/cnetlink.hpp>
#include <roflibs/netlink/ctundev.hpp>
#include <roflibs/python/cpython.hpp>

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

	/**
	 *
	 */
	cbasebox(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) :
						rofl::crofbase(versionbitmap),
						table_id_eth_port_membership(0),
						table_id_eth_src(1),
						table_id_eth_local(2),
						table_id_ip_local(3),
						table_id_gre_local(4),
						table_id_gtp_local(4),
						table_id_ip_fwd(5),
						table_id_eth_dst(6),
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
	clear_tap_devs() {
		while (not devs.empty()) {
			std::map<std::string, rofcore::ctapdev*>::iterator it = devs.begin();
			hook_port_down(it->second->get_devname());
			drop_tap_dev(it->first);
		}
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	add_tap_dev(const std::string& devname, const rofl::caddress_ll& hwaddr) {
		if (devs.find(devname) != devs.end()) {
			delete devs[devname];
		}
		devs[devname] = new rofcore::ctapdev(this, devname, hwaddr);
		return *(devs[devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const std::string& devname, const rofl::caddress_ll& hwaddr) {
		if (devs.find(devname) == devs.end()) {
			devs[devname] = new rofcore::ctapdev(this, devname, hwaddr);
		}
		return *(devs[devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const std::string& devname) {
		if (devs.find(devname) == devs.end()) {
			throw rofcore::eTunDevNotFound();
		}
		return *(devs[devname]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::caddress_ll& hwaddr) {
		std::map<std::string, rofcore::ctapdev*>::iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(hwaddr))) == devs.end()) {
			throw rofcore::eTunDevNotFound();
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const std::string& devname) const {
		if (devs.find(devname) == devs.end()) {
			throw rofcore::eTunDevNotFound();
		}
		return *(devs.at(devname));
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const rofl::caddress_ll& hwaddr) const {
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(hwaddr))) == devs.end()) {
			throw rofcore::eTunDevNotFound();
		}
		return *(it->second);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(const std::string& devname) {
		if (devs.find(devname) == devs.end()) {
			return;
		}
		delete devs[devname];
		devs.erase(devname);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(const rofl::caddress_ll& hwaddr) {
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(hwaddr))) == devs.end()) {
			return;
		}
		delete it->second;
		devs.erase(it->first);
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const std::string& devname) const {
		return (not (devs.find(devname) == devs.end()));
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const rofl::caddress_ll& hwaddr) const {
		std::map<std::string, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(hwaddr))) == devs.end()) {
			return false;
		}
		return true;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbasebox& box) {
		os << rofcore::indent(0) << "<cbasebox dptid: " << box.dptid << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<std::string, rofcore::ctapdev*>::const_iterator
				it = box.devs.begin(); it != box.devs.end(); ++it) {
			os << *(it->second);
		}
		os << roflibs::ip::cipcore::get_ip_core(rofl::crofdpt::get_dpt(box.dptid).get_dpid());
		try {
			os << roflibs::ethernet::cethcore::get_eth_core(rofl::crofdpt::get_dpt(box.dptid).get_dpid());
		} catch (rofl::eRofDptNotFound& e) {}
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
	hook_dpt_attach();

	void
	hook_dpt_detach();

	void
	hook_port_up(std::string const& devname);

	void
	hook_port_down(std::string const& devname);

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
	test_workflow();

private:

	rofl::cdptid dptid;
	static const std::string BASEBOX_LOG_FILE;
	static const std::string BASEBOX_PID_FILE;
	static const std::string BASEBOX_CONFIG_FILE;

	static std::string script_path_dpt_open;
	static std::string script_path_dpt_close;
	static std::string script_path_port_up;
	static std::string script_path_port_down;

	std::map<std::string, rofcore::ctapdev*>	devs;
	std::string	python_script;

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
