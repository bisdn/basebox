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

//#include "cconfig.h"
#include "clogging.h"
#include "cdptlink.h"
#include "cnetlink.h"
#include "croutetables.h"
#include "clinktable.h"

namespace ipcore {

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};
class eVmCoreNotFound		: public eVmCoreBase {};

class cipcore : public rofcore::cnetlink_common_observer {
public:

	/**
	 *
	 */
	static cipcore&
	get_instance(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap());


public:

	/**
	 *
	 */
	const clinktable&
	get_link_table() const { return ltable; };

	/**
	 *
	 */
	clinktable&
	set_link_table() { return ltable; };

	/**
	 *
	 */
	const croutetables&
	get_route_tables() const { return rtables; };

	/**
	 *
	 */
	croutetables&
	set_route_tables() { return rtables; };

public:


	virtual void
	handle_dpt_open(rofl::crofdpt& dpt);


	virtual void
	handle_dpt_close(rofl::crofdpt& dpt);

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

	// IPv4 addresses
	virtual void
	addr_in4_created(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in4_updated(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in4_deleted(unsigned int ifindex, uint16_t adindex);

	// IPv6 addresses
	virtual void
	addr_in6_created(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in6_updated(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in6_deleted(unsigned int ifindex, uint16_t adindex);

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


	// IPv4 neighbors
	virtual void
	neigh_in4_created(unsigned int ifindex, uint16_t nbindex);

	virtual void
	neigh_in4_updated(unsigned int ifindex, uint16_t nbindex);

	virtual void
	neigh_in4_deleted(unsigned int ifindex, uint16_t nbindex);

	// IPv6 neighbors
	virtual void
	neigh_in6_created(unsigned int ifindex, uint16_t nbindex);

	virtual void
	neigh_in6_updated(unsigned int ifindex, uint16_t nbindex);

	virtual void
	neigh_in6_deleted(unsigned int ifindex, uint16_t nbindex);


private:

	/**
	 *
	 */
	cipcore(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap);


	/**
	 *
	 */
	virtual
	~cipcore();

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

	/*
	 * data path related methods
	 */
	void
	set_forwarding(bool forward = true);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cipcore& ipcore) {
		os << rofcore::indent(0) << "<cipcore dptid: " << ipcore.dptid << " >" << std::endl;
		rofcore::indent i(2);
		os << ipcore.ltable;
		os << ipcore.rtables;
		return os;
	};

private:

	static const unsigned int __ETH_FRAME_LEN = 9018; // including jumbo frames

	static std::string script_path_dpt_open;
	static std::string script_path_dpt_close;
	static std::string script_path_port_up;
	static std::string script_path_port_down;

	static void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);


	rofl::cdptid 	dptid;
	clinktable 		ltable;	// table of links
	croutetables 	rtables;	// routing tables (v4 and v6)

	static cipcore* __ipcore__;
};

}; // end of namespace vmcore


#endif /* VMCORE_H_ */
