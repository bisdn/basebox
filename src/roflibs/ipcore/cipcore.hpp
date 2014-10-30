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

#include <libconfig.h++>

#include <rofl/common/crofbase.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/openflow/cofhelloelemversionbitmap.h>

#include <roflibs/netlink/ctapdev.hpp>
#include <roflibs/netlink/clogging.hpp>
#include <roflibs/netlink/cnetlink.hpp>
#include <roflibs/ipcore/croutetable.hpp>
#include <roflibs/ipcore/clink.hpp>
#include <roflibs/ipcore/cneigh.hpp>
#include <roflibs/ethcore/cportdb.hpp>

namespace roflibs {
namespace ip {

class eIpCoreBase			: public std::runtime_error {
public:
	eIpCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eIpCoreNotFound : public eIpCoreBase {
public:
	eIpCoreNotFound(const std::string& __arg) : eIpCoreBase(__arg) {};
};

/**
 * local-table: default=3
 * out-table: default=4
 */
class cipcore : public rofcore::cnetlink_common_observer {
public:

	/**
	 *
	 */
	static cipcore&
	add_ip_core(const rofl::cdpid& dpid, uint8_t local_ofp_table_id, uint8_t out_ofp_table_id) {
		if (cipcore::ipcores.find(dpid) != cipcore::ipcores.end()) {
			delete cipcore::ipcores[dpid];
			cipcore::ipcores.erase(dpid);
		}
		cipcore::ipcores[dpid] = new cipcore(dpid, local_ofp_table_id, out_ofp_table_id);
		return *(cipcore::ipcores[dpid]);
	};

	/**
	 *
	 */
	static cipcore&
	set_ip_core(const rofl::cdpid& dpid, uint8_t local_ofp_table_id, uint8_t out_ofp_table_id) {
		if (cipcore::ipcores.find(dpid) == cipcore::ipcores.end()) {
			cipcore::ipcores[dpid] = new cipcore(dpid, local_ofp_table_id, out_ofp_table_id);
		}
		return *(cipcore::ipcores[dpid]);
	};


	/**
	 *
	 */
	static cipcore&
	set_ip_core(const rofl::cdpid& dpid) {
		if (cipcore::ipcores.find(dpid) == cipcore::ipcores.end()) {
			throw eIpCoreNotFound("cipcore::set_ip_core() dpt not found");
		}
		return *(cipcore::ipcores[dpid]);
	};

	/**
	 *
	 */
	static const cipcore&
	get_ip_core(const rofl::cdpid& dpid) {
		if (cipcore::ipcores.find(dpid) == cipcore::ipcores.end()) {
			throw eIpCoreNotFound("cipcore::get_ip_core() dptid not found");
		}
		return *(cipcore::ipcores.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_ip_core(const rofl::cdpid& dpid) {
		if (cipcore::ipcores.find(dpid) == cipcore::ipcores.end()) {
			return;
		}
		delete cipcore::ipcores[dpid];
		cipcore::ipcores.erase(dpid);
	}

	/**
	 *
	 */
	static bool
	has_ip_core(const rofl::cdpid& dpid) {
		return (not (cipcore::ipcores.find(dpid) == cipcore::ipcores.end()));
	};

private:

	/**
	 *
	 */
	cipcore(const rofl::cdpid& dpid,
			uint8_t local_ofp_table_id = 3,
			uint8_t out_ofp_table_id = 4) :
		state(STATE_DETACHED), dpid(dpid),
		local_ofp_table_id(local_ofp_table_id),
		out_ofp_table_id(out_ofp_table_id),
		config_file(DEFAULT_CONFIG_FILE) {};


	/**
	 *
	 */
	virtual
	~cipcore()
		{};

public:

	/**
	 *
	 */
	void
	clear_links() {
		for (std::map<int, clink*>::iterator
				it = links.begin(); it != links.end(); ++it) {
			delete it->second;
		}
		links.clear();
	};

	/**
	 *
	 */
	clink&
	add_link(int ifindex, const std::string& devname, const rofl::caddress_ll& hwaddr, bool tagged = false, uint16_t vid = 1) {
		if (links.find(ifindex) != links.end()) {
			delete links[ifindex];
			links.erase(ifindex);
		}
		links[ifindex] = new clink(dpid, ifindex, devname, hwaddr, local_ofp_table_id, out_ofp_table_id, tagged, vid);
		if (STATE_ATTACHED == state) {
			links[ifindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return *(links[ifindex]);
	};

	/**
	 *
	 */
	clink&
	set_link(int ifindex, const std::string& devname, const rofl::caddress_ll& hwaddr, bool tagged = false, uint16_t vid = 1) {
		if (links.find(ifindex) == links.end()) {
			links[ifindex] = new clink(dpid, ifindex, devname, hwaddr, local_ofp_table_id, out_ofp_table_id, tagged, vid);
			if (STATE_ATTACHED == state) {
				links[ifindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return *(links[ifindex]);
	};

	/**
	 *
	 */
	clink&
	set_link(int ifindex) {
		if (links.find(ifindex) == links.end()) {
			throw eLinkNotFound("cipcore::set_link() ofp-port-no not found");
		}
		return *(links[ifindex]);
	};

	/**
	 *
	 */
	const clink&
	get_link(int ifindex) const {
		if (links.find(ifindex) == links.end()) {
			throw eLinkNotFound("cipcore::get_link() ofp-port-no not found");
		}
		return *(links.at(ifindex));
	};

	/**
	 *
	 */
	void
	drop_link(int ifindex) {
		if (links.find(ifindex) == links.end()) {
			return;
		}
		if (STATE_ATTACHED == state) {
			links[ifindex]->handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
		}
		delete links[ifindex];
		links.erase(ifindex);
	};

	/**
	 *
	 */
	bool
	has_link(int ifindex) const {
		return (not (links.find(ifindex) == links.end()));
	};

	/**
	 *
	 */
	const clink&
	get_link(const std::string& devname) const {
		std::map<int, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_devname(devname))) == links.end()) {
			throw eLinkNotFound("cipcore::get_link_by_devname() devname not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	void
	drop_link(const std::string& devname) {
		std::map<int, clink*>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(),
				clink::clink_by_devname(devname))) == links.end()) {
			return;
		}
		drop_link(it->first);
	};

	/**
	 *
	 */
	bool
	has_link(const std::string& devname) const {
		std::map<int, clink*>::const_iterator it;
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
		rtables[rttblid] = croutetable(rttblid, dpid, local_ofp_table_id, out_ofp_table_id);
		if (STATE_ATTACHED == state) {
			rtables[rttblid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return rtables[rttblid];
	};

	/**
	 *
	 */
	croutetable&
	set_table(unsigned int rttblid) {
		if (rtables.find(rttblid) == rtables.end()) {
			rtables[rttblid] = croutetable(rttblid, dpid, local_ofp_table_id, out_ofp_table_id);
			if (STATE_ATTACHED == state) {
				rtables[rttblid].handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
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
			rtables[rttblid].handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
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
					<< rofl::crofdpt::get_dpt(ipcore.dpid).get_dpid().str() << " >" << std::endl;
		} catch (rofl::eRofDptNotFound& e) {
			os << rofcore::indent(0) << "<cipcore dptid: >" << std::endl;
			os << rofcore::indent(2) << ipcore.dpid;
		}
		rofcore::indent i(2);
		for (std::map<int, clink*>::const_iterator
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

	enum ofp_core_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	ofp_core_state_t					state;
	rofl::cdpid 						dpid;
	std::map<int, clink*> 				links;	// key: ifindex, value: ptr to clink
	std::map<unsigned int, croutetable>	rtables;
	uint8_t								local_ofp_table_id;
	uint8_t								out_ofp_table_id;

	static std::map<rofl::cdpid, cipcore*>		ipcores;

	static const std::string			DEFAULT_CONFIG_FILE;
	std::string							config_file;

private:

	void
	set_forwarding(bool forward = true);

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

}; // end of namespace ip
}; // end of namespace roflibs

#endif /* VMCORE_H_ */
