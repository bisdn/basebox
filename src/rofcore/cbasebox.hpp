/*
 * cbasebox.hpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#ifndef CROFBASE_HPP_
#define CROFBASE_HPP_

#include <iostream>
#include <exception>
#include <rofl/common/crofbase.h>

#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#include "cethcore.hpp"
#include "cipcore.hpp"
#include "cgtpcore.hpp"
#include "clogging.h"
#include "cconfig.hpp"
#include "cnetlink.h"

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

class cbasebox : public rofl::crofbase, public rofcore::cnetdev_owner {

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
						rofl::crofbase(versionbitmap) {};

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
			rofl::crofdpt& dpt) {
		dptid = dpt.get_dptid();
		ipcore::cipcore::get_instance(dpt.get_dptid(), /*local-stage=*/3, /*out-stage=*/4);
		ethcore::cethcore::set_core(dpt.get_dpid(), /*default_vid=*/1, 0, 1, 5);
		rofgtp::cgtpcore::set_gtp_core(dpt.get_dptid(), /*gtp-stage=*/3); // yes, same as local for cipcore
		dpt.flow_mod_reset();
		dpt.group_mod_reset();
		dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);
	};

	/**
	 *
	 */
	virtual void
	handle_dpt_close(
			rofl::crofdpt& dpt) {
		ethcore::cethcore::set_core(dpt.get_dpid()).handle_dpt_close(dpt);
		ipcore::cipcore::get_instance(dpt.get_dptid()).handle_dpt_close(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {

		try {
			uint32_t ofp_port_no = msg.get_match().get_in_port();

			if (not has_tap_dev(ofp_port_no)) {
				// TODO: handle error?
				return;
			}

			rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
			*pkt = msg.get_packet();
			set_tap_dev(ofp_port_no).enqueue(pkt);

			switch (msg.get_table_id()) {
			case 3:
			case 4: {
				ipcore::cipcore::get_instance(dpt.get_dptid()).handle_packet_in(dpt, auxid, msg);
			} break;
			default: {
				if (ethcore::cethcore::has_core(dpt.get_dpid())) {
					ethcore::cethcore::set_core(dpt.get_dpid()).handle_packet_in(dpt, auxid, msg);
				}
			};
			}

		} catch (rofl::openflow::eOxmNotFound& e) {
			// TODO: log error
		}

		// drop buffer on data path, if the packet was stored there, as it is consumed entirely by the underlying kernel
		if (rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()) != msg.get_buffer_id()) {
			dpt.drop_buffer(auxid, msg.get_buffer_id());
		}
	};

	/**
	 *
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {
		switch (msg.get_table_id()) {
		case 3:
		case 4: {
			ipcore::cipcore::get_instance(dpt.get_dptid()).handle_flow_removed(dpt, auxid, msg);
		} break;
		default: {
			if (ethcore::cethcore::has_core(dpt.get_dpid())) {
				ethcore::cethcore::set_core(dpt.get_dpid()).handle_flow_removed(dpt, auxid, msg);
			}
		};
		}
	};

	/**
	 *
	 */
	virtual void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg) {
		try {
			const rofl::openflow::cofport& port = msg.get_port();
			uint32_t ofp_port_no = msg.get_port().get_port_no();

			switch (msg.get_reason()) {
			case rofl::openflow::OFPPR_ADD: {
				if (not has_tap_dev(ofp_port_no)) {
					add_tap_dev(port.get_port_no(), port.get_name(), port.get_hwaddr());
				}

				if (port.link_state_is_link_down() || port.config_is_port_down()) {
					set_tap_dev(ofp_port_no).disable_interface();
				} else {
					set_tap_dev(ofp_port_no).enable_interface();
				}

			} break;
			case rofl::openflow::OFPPR_MODIFY: {
				if (not has_tap_dev(ofp_port_no)) {
					add_tap_dev(port.get_port_no(), port.get_name(), port.get_hwaddr());
				}

				if (port.link_state_is_link_down() || port.config_is_port_down()) {
					set_tap_dev(ofp_port_no).disable_interface();
				} else {
					set_tap_dev(ofp_port_no).enable_interface();
				}

			} break;
			case rofl::openflow::OFPPR_DELETE: {
				drop_tap_dev(port.get_port_no());

			} break;
			default: {
				// invalid/unknown reason
			} return;
			}

			ipcore::cipcore::get_instance(dpt.get_dptid()).handle_port_status(dpt, auxid, msg);
			if (ethcore::cethcore::has_core(dpt.get_dpid())) {
				ethcore::cethcore::set_core(dpt.get_dpid()).handle_port_status(dpt, auxid, msg);
			}

		} catch (rofl::openflow::ePortNotFound& e) {
			// TODO: log error
		}
	};

	/**
	 *
	 */
	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {
		ipcore::cipcore::get_instance(dpt.get_dptid()).handle_error_message(dpt, auxid, msg);
		if (ethcore::cethcore::has_core(dpt.get_dpid())) {
			ethcore::cethcore::set_core(dpt.get_dpid()).handle_error_message(dpt, auxid, msg);
		}
	};

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg) {

		dpt.set_ports() = msg.get_ports();

		for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
				it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {
			const rofl::openflow::cofport& port = *(it->second);

			ethcore::cethcore::set_core(dpt.get_dpid()).set_vlan(/*default_vid=*/1).add_port(port.get_port_no(), /*tagged=*/false);
		}

		for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
				it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {
			const rofl::openflow::cofport& port = *(it->second);
			//ipcore::cipcore::get_instance().set_link(port.get_port_no(), port.get_name(), port.get_hwaddr(), false, 1);
			if (not has_tap_dev(port.get_port_no())) {
				add_tap_dev(port.get_port_no(), port.get_name(), port.get_hwaddr());
			}
		}

		ethcore::cethcore::set_core(dpt.get_dpid()).handle_dpt_open(dpt);
		ipcore::cipcore::get_instance().handle_dpt_open(dpt);
		rofgtp::cgtpcore::set_gtp_core(dpt.get_dptid()).handle_dpt_open(dpt);

		/*
		 * test
		 */
		if (true) {
			rofgtp::clabel_in4 label_in(
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), rofgtp::cport(2152)),
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , rofgtp::cport(2152)),
					rofgtp::cteid(111111));
			rofgtp::clabel_in4 label_out(
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.1") , rofgtp::cport(2152)),
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.20"), rofgtp::cport(2152)),
					rofgtp::cteid(222222));
			rofgtp::cgtpcore::set_gtp_core(dpt.get_dptid()).add_relay_in4(label_in, label_out);
		}
		if (true) {
			rofgtp::clabel_in4 label_in(
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.20"), rofgtp::cport(2152)),
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.2.2.1") , rofgtp::cport(2152)),
					rofgtp::cteid(222222));
			rofgtp::clabel_in4 label_out(
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.1") , rofgtp::cport(2152)),
					rofgtp::caddress_gtp_in4(rofl::caddress_in4("10.1.1.10"), rofgtp::cport(2152)),
					rofgtp::cteid(111111));
			rofgtp::cgtpcore::set_gtp_core(dpt.get_dptid()).add_relay_in4(label_in, label_out);
		}
	};

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid) {
	};

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
	rofcore::ctapdev&
	add_tap_dev(uint32_t ofp_port_no, const std::string& devname, const rofl::caddress_ll& hwaddr) {
		if (devs.find(ofp_port_no) != devs.end()) {
			delete devs[ofp_port_no];
		}
		devs[ofp_port_no] = new rofcore::ctapdev(this, devname, hwaddr, ofp_port_no);
		return *(devs[ofp_port_no]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(uint32_t ofp_port_no, const std::string& devname, const rofl::caddress_ll& hwaddr) {
		if (devs.find(ofp_port_no) == devs.end()) {
			devs[ofp_port_no] = new rofcore::ctapdev(this, devname, hwaddr, ofp_port_no);
		}
		return *(devs[ofp_port_no]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(uint32_t ofp_port_no) {
		if (devs.find(ofp_port_no) == devs.end()) {
			throw rofcore::eTapDevNotFound();
		}
		return *(devs[ofp_port_no]);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const rofl::caddress_ll& hwaddr) {
		std::map<uint32_t, rofcore::ctapdev*>::iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(hwaddr))) == devs.end()) {
			throw rofcore::eTapDevNotFound();
		}
		return *(it->second);
	};

	/**
	 *
	 */
	rofcore::ctapdev&
	set_tap_dev(const std::string& devname) {
		std::map<uint32_t, rofcore::ctapdev*>::iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_devname(devname))) == devs.end()) {
			throw rofcore::eTapDevNotFound();
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(uint32_t ofp_port_no) const {
		if (devs.find(ofp_port_no) == devs.end()) {
			throw rofcore::eTapDevNotFound();
		}
		return *(devs.at(ofp_port_no));
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const rofl::caddress_ll& hwaddr) const {
		std::map<uint32_t, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(hwaddr))) == devs.end()) {
			throw rofcore::eTapDevNotFound();
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const rofcore::ctapdev&
	get_tap_dev(const std::string& devname) const {
		std::map<uint32_t, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_devname(devname))) == devs.end()) {
			throw rofcore::eTapDevNotFound();
		}
		return *(it->second);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(uint32_t ofp_port_no) {
		if (devs.find(ofp_port_no) == devs.end()) {
			return;
		}
		delete devs[ofp_port_no];
		devs.erase(ofp_port_no);
	};

	/**
	 *
	 */
	void
	drop_tap_dev(const rofl::caddress_ll& hwaddr) {
		std::map<uint32_t, rofcore::ctapdev*>::const_iterator it;
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
	void
	drop_tap_dev(const std::string& devname) {
		std::map<uint32_t, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_devname(devname))) == devs.end()) {
			return;
		}
		delete it->second;
		devs.erase(it->first);
	};

	/**
	 *
	 */
	bool
	has_tap_dev(uint32_t ofp_port_no) const {
		return (not (devs.find(ofp_port_no) == devs.end()));
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const rofl::caddress_ll& hwaddr) const {
		std::map<uint32_t, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_hwaddr(hwaddr))) == devs.end()) {
			return false;
		}
		return true;
	};

	/**
	 *
	 */
	bool
	has_tap_dev(const std::string& devname) const {
		std::map<uint32_t, rofcore::ctapdev*>::const_iterator it;
		if ((it = find_if(devs.begin(), devs.end(),
				rofcore::ctapdev::ctapdev_find_by_devname(devname))) == devs.end()) {
			return false;
		}
		return true;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbasebox& box) {
		os << rofcore::indent(0) << "<cbasebox dptid: " << box.dptid << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<uint32_t, rofcore::ctapdev*>::const_iterator
				it = box.devs.begin(); it != box.devs.end(); ++it) {
			os << *(it->second);
		}
		os << ipcore::cipcore::get_instance();
		try {
			os << ethcore::cethcore::get_core(rofl::crofdpt::get_dpt(box.dptid).get_dpid());
		} catch (rofl::eRofDptNotFound& e) {}
		return os;
	};

private:

	rofl::cdptid dptid;
	static const std::string ROFCORE_LOG_FILE;
	static const std::string ROFCORE_PID_FILE;
	static const std::string ROFCORE_CONFIG_FILE;
	static const std::string ROFCORE_CONFIG_DPT_LIST;

	std::map<uint32_t, rofcore::ctapdev*>	devs;
};

}; // end of namespace ethcore

#endif /* CROFBASE_HPP_ */
