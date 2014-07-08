/*
 * cipcore.h
 *
 *  Created on: 29.05.2013
 *      Author: andi
 */

#ifndef CIPCORE_H_
#define CIPCORE_H_ 1

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef __cplusplus
}
#endif

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <exception>

#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofhelloelemversionbitmap.h>

#include "logging.h"
#include "cdptlink.h"
#include "cnetlink.h"
#include "croutetables.h"
#include "clinktable.h"

namespace ipcore {

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};
class eVmCoreNotFound		: public eVmCoreBase {};

class cipcore :
		public rofl::crofbase,
		public rofcore::cnetlink_subscriber
{
public:

	static cipcore&
	get_ipcore(const rofl::cdptid& dptid) {
		if (cipcore::ipcores.find(dptid) == cipcore::ipcores.end()) {
			new cipcore(dptid);
		}
		return cipcore::ipcores[dptid];
	};

	static bool
	has_ipcore(const rofl::cdptid& dptid) const {
		return (not (cipcore::ipcores.find(dptid) == cipcore::ipcores.end()));
	};

public:


	/**
	 *
	 */
	cipcore(
			rofl::cdptid& dptid,
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap);


	/**
	 *
	 */
	virtual
	~cipcore();


public:


	virtual void
	handle_dpt_open(rofl::crofdpt& dpt);


	virtual void
	handle_dpt_close(rofl::crofdpt& dpt);


	virtual void
	handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	virtual void
	handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	virtual void
	handle_packet_out(rofl::crofctl& ctl, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_out& msg);

	virtual void
	handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg);

	virtual void
	handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg);

	virtual void
	handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg);


public:

	// IPv4 routes
	virtual void
	route_in4_created(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_in4_updated(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_in4_deleted(uint8_t table_id, unsigned int rtindex);

	// IPv6 routes
	virtual void
	route_in6_created(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_in6_updated(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_in6_deleted(uint8_t table_id, unsigned int rtindex);



private:

	void
	purge_dpt_entries();

	void
	block_stp_frames();

	void
	unblock_stp_frames();

	void
	redirect_ipv4_multicast();

	void
	redirect_ipv6_multicast();


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

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cipcore& ipcore) {
		os << rofcore::indent(0) << "<cipcore dptid: " << ipcore.dptid << " >" << std::endl;
		rofcore::indent i(2);
		os << ipcore.ltable;
		os << ipcore.rtable;
		return os;
	};

private:

	static std::map<rofl::cdptid, cipcore*> ipcores;

	static const unsigned int ETH_FRAME_LEN = 9018; // including jumbo frames

	static const std::string script_path_dpt_open	("/var/lib/ipcore/dpath-open.sh");
	static const std::string script_path_dpt_close	("/var/lib/ipcore/dpath-close.sh");
	static const std::string script_path_port_up	("/var/lib/ipcore/port-up.sh");
	static const std::string script_path_port_down	("/var/lib/ipcore/port-down.sh");

	static void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);


	rofl::cdptid 	dptid;
	clinktable 		ltable;	// table of links
	croutetables 	rtable;	// routing tables (v4 and v6)

};

}; // end of namespace vmcore


#endif /* VMCORE_H_ */
