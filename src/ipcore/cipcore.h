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

#include "dptlink.h"
#include "dptroute.h"
#include "cnetlink.h"

namespace ipcore
{

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};
class eVmCoreNotFound		: public eVmCoreBase {};

class cipcore :
		public rofl::crofbase,
		public rofcore::cnetlink_subscriber
{
	#define DEFAULT_DPATH_OPEN_SCRIPT_PATH 	"/var/lib/ipcore/dpath-open.sh"
	#define DEFAULT_DPATH_CLOSE_SCRIPT_PATH "/var/lib/ipcore/dpath-close.sh"
	#define DEFAULT_PORT_UP_SCRIPT_PATH 	"/var/lib/ipcore/port-up.sh"
	#define DEFAULT_PORT_DOWN_SCRIPT_PATH 	"/var/lib/ipcore/port-down.sh"

	static std::string	dpath_open_script_path;
	static std::string	dpath_close_script_path;
	static std::string	port_up_script_path;
	static std::string	port_down_script_path;

	static void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);
private:


	rofl::crofdpt 												*dpt;			// handle for cofdpt instance managed by this vmcore
	std::map<rofl::crofdpt*, std::map<uint32_t, cdptlink*> > 	 dptlinks;		// mapped ports per data path element
	std::map<uint8_t, std::map<unsigned int, dptroute*> >		 dpt4routes;	// active routes => key1:table_id, key2:routing index, value: dptroute instance
	std::map<uint8_t, std::map<unsigned int, dptroute*> >		 dpt6routes;	// active routes => key1:table_id, key2:routing index, value: dptroute instance

	enum ipcore_timer_t {
		IPCORE_TIMER_BASE = 0x6423beed,
		IPCORE_TIMER_DUMP,
	};

	int dump_state_interval;

public:


	/**
	 *
	 */
	cipcore(
			rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap);


	/**
	 *
	 */
	virtual
	~cipcore();


public:


	virtual void
	handle_dpath_open(rofl::crofdpt& dpt);


	virtual void
	handle_dpath_close(rofl::crofdpt& dpt);


	virtual void
	handle_port_status(rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_status& msg, uint8_t aux_id = 0);

	virtual void
	handle_packet_out(rofl::crofctl& ctl, rofl::openflow::cofmsg_packet_out& msg, uint8_t aux_id = 0);

	virtual void
	handle_packet_in(rofl::crofdpt& dpt, rofl::openflow::cofmsg_packet_in& msg, uint8_t aux_id = 0);

	virtual void
	handle_error(rofl::crofdpt& dpt, rofl::openflow::cofmsg_error& msg, uint8_t aux_id = 0);

	virtual void
	handle_flow_removed(rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_removed& msg, uint8_t aux_id = 0);

	virtual void
	handle_timeout(int opaque, void *data = (void*)0);


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

	bool
	link_is_mapped_from_dpt(int ifindex);

	cdptlink&
	get_mapped_link_from_dpt(int ifindex);

	void
	delete_all_ports();

	void
	delete_all_routes();

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


	/*
	 * dump state periodically
	 */
	void
	dump_state();
};

}; // end of namespace vmcore


#endif /* VMCORE_H_ */
