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
#include "cdpid.hpp"
#include "clogging.h"
#include "cconfig.hpp"

namespace ethcore {

class crofbase : public rofl::crofbase {

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
		dpt.flow_mod_reset();
		dpt.group_mod_reset();
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_dpt_open(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_dpt_close(
			rofl::crofdpt& dpt) {
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_dpt_close(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {
		if (not cethcore::has_core(cdpid(dpt.get_dpid()))) {
			return;
		}
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_packet_in(dpt, auxid, msg);
	};

	/**
	 *
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {
		if (not cethcore::has_core(cdpid(dpt.get_dpid()))) {
			return;
		}
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_flow_removed(dpt, auxid, msg);
	};

	/**
	 *
	 */
	virtual void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg) {
		if (not cethcore::has_core(cdpid(dpt.get_dpid()))) {
			return;
		}
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_port_status(dpt, auxid, msg);
	};

	/**
	 *
	 */
	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {
		if (not cethcore::has_core(cdpid(dpt.get_dpid()))) {
			return;
		}
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_error_message(dpt, auxid, msg);
	};

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg) {
		if (not cethcore::has_core(cdpid(dpt.get_dpid()))) {
			return;
		}
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_port_desc_stats_reply(dpt, auxid, msg);
	};

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply_timeout(rofl::crofdpt& dpt, uint32_t xid) {
		if (not cethcore::has_core(cdpid(dpt.get_dpid()))) {
			return;
		}
		cethcore::set_core(cdpid(dpt.get_dpid())).handle_port_desc_stats_reply_timeout(dpt, xid);
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crofbase& ethbase) {

		return os;
	};

private:

	static const std::string ROFCORE_LOG_FILE;
	static const std::string ROFCORE_PID_FILE;
	static const std::string ROFCORE_CONFIG_FILE;
	static const std::string ROFCORE_CONFIG_DPT_LIST;
};

}; // end of namespace ethcore

#endif /* CROFBASE_HPP_ */
