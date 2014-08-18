/*
 * crofbase.hpp
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
#include "cdpid.hpp"
#include "clogging.h"
#include "cconfig.hpp"
#include "cnetlink.h"
#include "cdpid.hpp"

namespace basebox {

class crofbase : public rofl::crofbase, public rofcore::cnetlink_common_observer {

	/**
	 * @brief	pointer to singleton
	 */
	static crofbase*	rofbase;

	/**
	 *
	 */
	crofbase(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) :
						rofl::crofbase(versionbitmap) {};

	/**
	 *
	 */
	virtual
	~crofbase() {};

	/**
	 *
	 */
	crofbase(
			const crofbase& ethbase);

public:

	/**
	 *
	 */
	static crofbase&
	get_instance(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) {
		if (crofbase::rofbase == (crofbase*)0) {
			crofbase::rofbase = new crofbase(versionbitmap);
		}
		return *(crofbase::rofbase);
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
		ipcore::cipcore::get_instance(dpt.get_dptid(), 3, 4, 4);
		dpt.flow_mod_reset();
		dpt.group_mod_reset();
		ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_dpt_open(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_dpt_close(
			rofl::crofdpt& dpt) {
		ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_dpt_close(dpt);
		ipcore::cipcore::get_instance(dpt.get_dptid()).handle_dpt_close(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {
		switch (msg.get_table_id()) {
		case 3:
		case 4: {
			ipcore::cipcore::get_instance(dpt.get_dptid()).handle_packet_in(dpt, auxid, msg);
		} break;
		default: {
			if (ethcore::cethcore::has_core(ethcore::cdpid(dpt.get_dpid()))) {
				ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_packet_in(dpt, auxid, msg);
			}
		};
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
			if (ethcore::cethcore::has_core(ethcore::cdpid(dpt.get_dpid()))) {
				ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_flow_removed(dpt, auxid, msg);
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
		ipcore::cipcore::get_instance(dpt.get_dptid()).handle_port_status(dpt, auxid, msg);
		if (ethcore::cethcore::has_core(ethcore::cdpid(dpt.get_dpid()))) {
			ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_port_status(dpt, auxid, msg);
		}
	};

	/**
	 *
	 */
	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {
		ipcore::cipcore::get_instance(dpt.get_dptid()).handle_error_message(dpt, auxid, msg);
		if (ethcore::cethcore::has_core(ethcore::cdpid(dpt.get_dpid()))) {
			ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_error_message(dpt, auxid, msg);
		}
	};

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg) {
		if (ethcore::cethcore::has_core(ethcore::cdpid(dpt.get_dpid()))) {
			ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_port_desc_stats_reply(dpt, auxid, msg);
		}
		dpt.set_ports() = msg.get_ports();
		for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
				it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {
			const rofl::openflow::cofport& port = *(it->second);
			ipcore::cipcore::get_instance().set_link(port.get_port_no(), port.get_name(), port.get_hwaddr(), false, 1);
		}
		ipcore::cipcore::get_instance().handle_dpt_open(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid) {
		if (ethcore::cethcore::has_core(ethcore::cdpid(dpt.get_dpid()))) {
			ethcore::cethcore::set_core(ethcore::cdpid(dpt.get_dpid())).handle_port_desc_stats_reply_timeout(dpt, xid);
		}
	};

public:

	/**
	 *
	 */
	virtual void
	link_created(unsigned int ifindex);


	/**
	 *
	 */
	virtual void
	link_updated(unsigned int ifindex);


	/**
	 *
	 */
	virtual void
	link_deleted(unsigned int ifindex);


public:

	friend std::ostream&
	operator<< (std::ostream& os, const crofbase& ethbase) {

		return os;
	};

private:

	rofl::cdptid dptid;
	static const std::string ROFCORE_LOG_FILE;
	static const std::string ROFCORE_PID_FILE;
	static const std::string ROFCORE_CONFIG_FILE;
	static const std::string ROFCORE_CONFIG_DPT_LIST;
};

}; // end of namespace ethcore

#endif /* CROFBASE_HPP_ */
