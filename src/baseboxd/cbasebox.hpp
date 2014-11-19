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
#include <roflibs/netlink/ccookiebox.hpp>

#include "cconfig.hpp"

namespace basebox {

class eBaseBoxBase : public std::runtime_error {
public:
	eBaseBoxBase(const std::string& __arg) : std::runtime_error(__arg) {};
};


class cbasebox : public rofl::crofbase, public rofcore::cnetlink_common_observer {

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

	/**
	 *
	 */
	void
	set_python_script(const std::string& python_script) { this->python_script = python_script; };


public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbasebox& box) {
		os << rofcore::indent(0) << "<cbasebox>" << std::endl;
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
};

}; // end of namespace ethcore

#endif /* CROFBASE_HPP_ */
