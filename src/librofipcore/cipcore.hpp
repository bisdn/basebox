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
#include "clink.hpp"
#include "cnetlink.h"
#include "croutetable.hpp"

namespace ipcore {

class eVmCoreBase 			: public std::exception {};
class eVmCoreCritical 		: public eVmCoreBase {};
class eVmCoreNoDptAttached	: public eVmCoreBase {};
class eVmCoreNotFound		: public eVmCoreBase {};

class eIpCoreBase			: public std::runtime_error {
public:
	eIpCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};

class cipcore : public rofcore::cnetlink_common_observer {
public:

	/**
	 *
	 */
	static cipcore&
	get_instance(const rofl::cdptid& dptid = rofl::cdptid(), uint8_t in_ofp_table_id = 0);

public:

	/**
	 *
	 */
	void
	clear_links() {
		for (std::map<uint32_t, clink*>::iterator
				it = links.begin(); it != links.end(); ++it) {
			delete it->second;
		}
		links.clear();
	};

	/**
	 *
	 */
	clink&
	add_link(uint32_t ofp_port_no, const std::string& devname, const rofl::caddress_ll& hwaddr) {
		if (links.find(ofp_port_no) != links.end()) {
			links.erase(ofp_port_no);
		}
		links[ofp_port_no] = new clink(dptid, ofp_port_no, devname, hwaddr, in_ofp_table_id);
		links[ofp_port_no]->tap_open();
		if (STATE_ATTACHED == state) {
			links[ofp_port_no]->handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
		}
		hook_port_up(devname);
		return *(links[ofp_port_no]);
	};

	/**
	 *
	 */
	clink&
	set_link(uint32_t ofp_port_no, const std::string& devname, const rofl::caddress_ll& hwaddr) {
		if (links.find(ofp_port_no) == links.end()) {
			links[ofp_port_no] = new clink(dptid, ofp_port_no, devname, hwaddr, in_ofp_table_id);
			links[ofp_port_no]->tap_open();
			if (STATE_ATTACHED == state) {
				links[ofp_port_no]->handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
			hook_port_up(devname);
		}
		return *(links[ofp_port_no]);
	};

	/**
	 *
	 */
	clink&
	set_link(uint32_t ofp_port_no) {
		if (links.find(ofp_port_no) == links.end()) {
			throw eLinkNotFound("cipcore::set_link() ofp-port-no not found");
		}
		return *(links[ofp_port_no]);
	};

	/**
	 *
	 */
	clink&
	set_link_by_ifindex(int ifindex) {
		std::map<uint32_t, clink*>::iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_ifindex(ifindex))) == links.end()) {
			throw eLinkNotFound("cipcore::set_link_by_ifindex() ifindex not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const clink&
	get_link(uint32_t ofp_port_no) const {
		if (links.find(ofp_port_no) == links.end()) {
			throw eLinkNotFound("cipcore::get_link() ofp-port-no not found");
		}
		return *(links.at(ofp_port_no));
	};

	/**
	 *
	 */
	void
	drop_link(uint32_t ofp_port_no) {
		if (links.find(ofp_port_no) == links.end()) {
			return;
		}
		hook_port_down(get_link(ofp_port_no).get_devname());
		if (STATE_ATTACHED == state) {
			links[ofp_port_no]->handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
		}
		delete links[ofp_port_no];
		links.erase(ofp_port_no);
	};

	/**
	 *
	 */
	bool
	has_link(uint32_t ofp_port_no) const {
		return (not (links.find(ofp_port_no) == links.end()));
	};

	/**
	 *
	 */
	const clink&
	get_link_by_ofp_port_no(uint32_t ofp_port_no) const {
		std::map<uint32_t, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_ofp_port_no(ofp_port_no))) == links.end()) {
			throw eLinkNotFound("cipcore::get_link_by_ofp_port_no() ofp-port-no not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	bool
	has_link_by_ofp_port_no(uint32_t ofp_port_no) const {
		std::map<uint32_t, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_ofp_port_no(ofp_port_no))) == links.end()) {
			return false;
		}
		return true;
	};

	/**
	 *
	 */
	const clink&
	get_link_by_ifindex(int ifindex) const {
		std::map<uint32_t, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_ifindex(ifindex))) == links.end()) {
			throw eLinkNotFound("cipcore::get_link_by_ifindex() ifindex not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	bool
	has_link_by_ifindex(int ifindex) const {
		std::map<uint32_t, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_ifindex(ifindex))) == links.end()) {
			return false;
		}
		return true;
	};

	/**
	 *
	 */
	const clink&
	get_link_by_devname(const std::string& devname) const {
		std::map<uint32_t, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_devname(devname))) == links.end()) {
			throw eLinkNotFound("cipcore::get_link_by_devname() devname not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	bool
	has_link_by_devname(const std::string& devname) const {
		std::map<uint32_t, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_devname(devname))) == links.end()) {
			return false;
		}
		return true;
	};

public:

	/**
	 *
	 */
	void
	clear_routes() { rtables.clear(); };

	/**
	 *
	 */
	croutetable&
	add_table(unsigned int rttblid) {
		if (rtables.find(rttblid) != rtables.end()) {
			rtables.erase(rttblid);
		}
		rtables[rttblid] = croutetable(rttblid, dptid);
		if (STATE_ATTACHED == state) {
			rtables[rttblid].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
		}
		return rtables[rttblid];
	};

	/**
	 *
	 */
	croutetable&
	set_table(unsigned int rttblid) {
		if (rtables.find(rttblid) == rtables.end()) {
			rtables[rttblid] = croutetable(rttblid, dptid);
			if (STATE_ATTACHED == state) {
				rtables[rttblid].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		}
		return rtables[rttblid];
	};

	/**
	 *
	 */
	const croutetable&
	get_table(unsigned int rttblid) const {
		if (rtables.find(rttblid) == rtables.end()) {
			throw eRtTableNotFound("cipcore::get_table() table-id not found");
		}
		return rtables.at(rttblid);
	};

	/**
	 *
	 */
	void
	drop_table(unsigned int rttblid) {
		if (rtables.find(rttblid) == rtables.end()) {
			return;
		}
		if (STATE_ATTACHED == state) {
			rtables[rttblid].handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
		}
		rtables.erase(rttblid);
	};

	/**
	 *
	 */
	bool
	has_table(unsigned int table_id) const {
		return (not (rtables.find(table_id) == rtables.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cipcore& ipcore) {
		try {
			os << rofcore::indent(0) << "<cipcore dpid: "
					<< rofl::crofdpt::get_dpt(ipcore.dptid).get_dpid_s() << " >" << std::endl;
		} catch (rofl::eRofDptNotFound& e) {
			os << rofcore::indent(0) << "<cipcore dptid: >" << std::endl;
			os << rofcore::indent(2) << ipcore.dptid;
		}
		rofcore::indent i(2);
		for (std::map<unsigned int, clink*>::const_iterator
				it = ipcore.links.begin(); it != ipcore.links.end(); ++it) {
			os << *(it->second);
		}
		for (std::map<unsigned int, croutetable>::const_iterator
				it = ipcore.rtables.begin(); it != ipcore.rtables.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	static const unsigned int __ETH_FRAME_LEN = 9018; // including jumbo frames

	static std::string script_path_dpt_open;
	static std::string script_path_dpt_close;
	static std::string script_path_port_up;
	static std::string script_path_port_down;

	enum ofp_core_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	ofp_core_state_t					state;
	rofl::cdptid 						dptid;
	std::map<uint32_t, clink*> 			links;	// key: ofp port-no, value: clink
	std::map<unsigned int, croutetable>	rtables;
	uint8_t								in_ofp_table_id;

	static cipcore* sipcore;	// singleton

private:

	/**
	 *
	 */
	cipcore(const rofl::cdptid& dptid, uint8_t in_ofp_table_id = 0) :
		state(STATE_DETACHED), dptid(dptid), in_ofp_table_id(in_ofp_table_id) {};


	/**
	 *
	 */
	virtual
	~cipcore()
		{};

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

	void
	set_forwarding(bool forward = true);

	static void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);

public:

	virtual void
	handle_dpt_open(rofl::crofdpt& dpt);

	virtual void
	handle_dpt_close(rofl::crofdpt& dpt);

	virtual void
	handle_port_status(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	virtual void
	handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg);

	virtual void
	handle_error_message(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg);

	virtual void
	handle_flow_removed(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg);

public:

	// links
	virtual void
	link_created(unsigned int ifindex);

	virtual void
	link_updated(unsigned int ifindex);

	virtual void
	link_deleted(unsigned int ifindex);

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

};

}; // end of namespace vmcore


#endif /* VMCORE_H_ */
