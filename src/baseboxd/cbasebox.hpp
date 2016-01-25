/*
 * cbasebox.hpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#ifndef CROFBASE_HPP_
#define CROFBASE_HPP_

#define OF_DPA

#include <sys/types.h>
#include <sys/wait.h>

#include <string>
#include <iostream>
#include <exception>
#include <rofl/common/crofbase.h>

#ifndef OF_DPA
#include "roflibs/flowcore/cflowcore.hpp"
#include "roflibs/ethcore/cethcore.hpp"
#include "roflibs/ipcore/cipcore.hpp"
#include "roflibs/gtpcore/cgtpcore.hpp"
#include "roflibs/gtpcore/cgtprelay.hpp"
#include "roflibs/grecore/cgrecore.hpp"
#endif
#include "roflibs/netlink/clogging.hpp"
#include "roflibs/netlink/cnetlink.hpp"
#include "roflibs/netlink/ctundev.hpp"
#ifndef OF_DPA
#include "roflibs/python/cpython.hpp"
#include "roflibs/ethcore/cethcoredb_file.hpp"
#include "roflibs/gtpcore/cgtpcoredb_file.hpp"
#endif
#include "roflibs/netlink/ccookiebox.hpp"
#include "roflibs/netlink/cconfig.hpp"

#include "cconfig.hpp"
#include <baseboxd/switch_behavior.hpp>

namespace basebox {

class eBaseBoxBase : public std::runtime_error {
public:
	eBaseBoxBase(const std::string& __arg) : std::runtime_error(__arg) {};
};

static rofl::crofdpt invalid(NULL, rofl::cdptid(0));

class cbasebox : public rofl::crofbase,
	public virtual rofl::cthread_env {

	static bool keep_on_running;
	rofl::cthread thread;

	/**
	 *
	 */
	cbasebox(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) :
						thread(this),
						// FIXME this is configurations, hence move somewhere else
						table_id_svc_flows(0),
						table_id_eth_port_membership(1),
						table_id_eth_src(2),
						table_id_eth_local(3),
						table_id_ip_local(4),
						table_id_gre_local(5),
						table_id_gtp_local(5),
						table_id_ip_fwd(6),
						table_id_eth_dst(7),
						default_pvid(1),
						sa(switch_behavior_fabric::get_behavior(-1, invalid))
	{
		rofl::crofbase::set_versionbitmap(versionbitmap);
		thread.start();
	}

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
		static cbasebox box(versionbitmap);
		return box;
	}

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
	virtual void handle_wakeup(rofl::cthread& thread)
	{
	}

	virtual void
	handle_conn_established(
			rofl::crofdpt& dpt,
			const rofl::cauxid& auxid)
	{
		dpt.set_conn(auxid).set_trace(true);


		crofbase::add_ctl(rofl::cctlid(0)).set_conn(rofl::cauxid(0)).
				set_trace(true).
				set_journal().
				log_on_stderr(true).
				set_max_entries(64);

		crofbase::set_ctl(rofl::cctlid(0)).set_conn(rofl::cauxid(0)).
				set_tcp_journal().
				log_on_stderr(true).
				set_max_entries(16);


	}

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
			const rofl::cdptid& dptid);

	/**
	 *
	 */
	virtual void
	handle_features_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_features_reply& msg);

	virtual void
	handle_desc_stats_reply(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_desc_stats_reply& msg);

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

#if 0
	/**
	 *
	 */
	void
	set_python_script(const std::string& python_script) { this->python_script = python_script; }
#endif

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbasebox& box) {
		os << rofcore::indent(0) << "<cbasebox>" << std::endl;
		return os;
	}

private:

	/*
	 * event specific hooks
	 */
	void
	hook_dpt_attach(const rofl::cdptid& dptid);

	void
	hook_dpt_detach(const rofl::cdptid& dptid);

	void
	set_forwarding(bool forward = true);

	static void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);

private:

	void
	test_gtp(
			rofl::crofdpt& dpt);

	void
	test_workflow(
			rofl::crofdpt& dpt);

private:

	static const std::string BASEBOX_LOG_FILE;
	static const std::string BASEBOX_PID_FILE;
	static const std::string BASEBOX_CONFIG_FILE;

	static std::string script_path_dpt_open;
	static std::string script_path_dpt_close;

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

	enum cbasebox_flag_t {
		FLAG_FLOWCORE		= 1,
		FLAG_ETHCORE		= 2,
		FLAG_IPCORE			= 3,
		FLAG_GRECORE		= 4,
		FLAG_GTPCORE		= 5,
	};

	static std::bitset<64>		flags;

	rofl::cdpid					dpid;
	switch_behavior 			*sa;		// behavior of the switch (currently only a single switch)
};

} // end of namespace ethcore

#endif /* CROFBASE_HPP_ */
