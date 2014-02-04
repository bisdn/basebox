/*
 * vmcore.h
 *
 *  Created on: 29.05.2013
 *      Author: andi
 */

#ifndef VMCORE_H_
#define VMCORE_H_ 1

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
#include <exception>

//#include "cnetlink.h"
#include <rofl/common/crofbase.h>

#include <dptlink.h>
#include <dptroute.h>
#include <cnetlink.h>

namespace dptmap
{

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};
class eVmCoreNotFound		: public eVmCoreBase {};

class vmcore :
		public rofl::crofbase,
		public cnetlink_subscriber
{
	#define DEFAULT_DPATH_OPEN_SCRIPT_PATH "/var/lib/vmcore/dpath-open.sh"
	#define DEFAULT_DPATH_CLOSE_SCRIPT_PATH "/var/lib/vmcore/dpath-close.sh"
	#define DEFAULT_PORT_UP_SCRIPT_PATH "/var/lib/vmcore/port-up.sh"
	#define DEFAULT_PORT_DOWN_SCRIPT_PATH "/var/lib/vmcore/port-down.sh"

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


	rofl::crofdpt 												*dpt;		// handle for cofdpt instance managed by this vmcore
	std::map<rofl::crofdpt*, std::map<uint32_t, dptlink*> > 		 dptlinks;	// mapped ports per data path element
	std::map<uint8_t, std::map<unsigned int, dptroute*> >		 dptroutes;	// active routes => key1:table_id, key2:routing index, value: dptroute instance

	enum vmcore_timer_t {
		VMCORE_TIMER_BASE = 0x6423beed,
		VMCORE_TIMER_DUMP,
	};

	int dump_state_interval;

public:


	/**
	 *
	 */
	vmcore(
			cofhello_elem_versionbitmap const& versionbitmap);


	/**
	 *
	 */
	virtual
	~vmcore();


public:


	virtual void
	handle_dpath_open(rofl::crofdpt& dpt);


	virtual void
	handle_dpath_close(rofl::crofdpt& dpt);


	virtual void
	handle_port_status(rofl::crofdpt& dpt, rofl::cofmsg_port_status& msg, uint8_t aux_id = 0);

	virtual void
	handle_packet_out(rofl::crofctl& ctl, rofl::cofmsg_packet_out& msg, uint8_t aux_id = 0);

	virtual void
	handle_packet_in(rofl::crofdpt& dpt, rofl::cofmsg_packet_in& msg, uint8_t aux_id = 0);

	virtual void
	handle_error(rofl::crofdpt& dpt, rofl::cofmsg_error& msg, uint8_t aux_id = 0);

	virtual void
	handle_timeout(int opaque, void *data = (void*)0);


public:


	virtual void
	route_created(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_updated(uint8_t table_id, unsigned int rtindex);

	virtual void
	route_deleted(uint8_t table_id, unsigned int rtindex);


private:

	bool
	link_is_mapped_from_dpt(int ifindex);

	dptlink&
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

	void
	run_dpath_open_script();

	void
	run_dpath_close_script();

	void
	run_port_up_script(std::string const& devname);

	void
	run_port_down_script(std::string const& devname);

	void
	dump_state();
};

}; // end of namespace vmcore


#endif /* VMCORE_H_ */
