/*
 * cmemberport.hpp
 *
 *  Created on: 04.08.2014
 *      Author: andreas
 */

#ifndef CMEMBERPORT_HPP_
#define CMEMBERPORT_HPP_

#include <inttypes.h>
#include <exception>
#include <iostream>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/protocols/fvlanframe.h>

#include "roflibs/netlink/clogging.hpp"
#include "roflibs/netlink/ccookiebox.hpp"

namespace roflibs {
namespace eth {

class eMemberPortBase : public std::runtime_error {
public:
	eMemberPortBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eMemberPortNotFound : public eMemberPortBase {
public:
	eMemberPortNotFound(const std::string& __arg) : eMemberPortBase(__arg) {};
};

class cmemberport : public roflibs::common::openflow::ccookie_owner {
public:

	/**
	 *
	 */
	cmemberport() :
		dpt_state(STATE_IDLE),
		cookie(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
		portno(0xffffffff), vid(0xffff),
		tagged(false), table_id_eth_in(0), table_id_eth_local(0), table_id_eth_out(0) {};

	/**
	 *
	 */
	cmemberport(
			const rofl::cdpid& dpid, uint8_t in_stage_table_id, uint8_t local_stage_table_id, uint8_t out_stage_table_id,
			uint32_t portno, const rofl::cmacaddr& hwaddr,
			uint16_t vid = 0xffff, bool tagged = true) :
		dpt_state(STATE_IDLE), dpid(dpid),
		cookie(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
		portno(portno), vid(vid),
		tagged(tagged), table_id_eth_in(in_stage_table_id),
		table_id_eth_local(local_stage_table_id), table_id_eth_out(out_stage_table_id), hwaddr(hwaddr) {};

	/**
	 *
	 */
	virtual
	~cmemberport() {
		try {
			if (STATE_ATTACHED == dpt_state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	cmemberport(const cmemberport& port)
		{ *this = port; };

	/**
	 *
	 */
	cmemberport&
	operator= (const cmemberport& port) {
		if (this == &port)
			return *this;
		dpt_state	= port.dpt_state;
		dpid 	= port.dpid;
		// do not copy cookie here!
		portno 	= port.portno;
		vid 	= port.vid;
		tagged 	= port.tagged;
		table_id_eth_in = port.table_id_eth_in;
		table_id_eth_local = port.table_id_eth_local;
		table_id_eth_out = port.table_id_eth_out;
		hwaddr	= port.hwaddr;
		return *this;
	};

public:

	/**
	 *
	 */
	uint32_t
	get_port_no() const { return portno; };

	/**
	 *
	 */
	const rofl::cmacaddr&
	get_hwaddr() const { return hwaddr; };

	/**
	 *
	 */
	bool
	get_tagged() const { return tagged; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open(
			rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close(
			rofl::crofdpt& dpt);

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
	void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);

	/**
	 *
	 */
	void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cmemberport& port) {
		os << rofcore::indent(0) << "<cmemberport "
				<< "portno: " << (int)port.get_port_no() << " "
				<< "tagged: " << (int)port.get_tagged() << " "
				<< ">" << std::endl;
		return os;
	};

private:

	enum dpt_state_t {
		STATE_IDLE = 1,
		STATE_DETACHED = 2,
		STATE_ATTACHED = 3,
	};

	dpt_state_t		dpt_state;

	rofl::cdpid		dpid;
	uint64_t		cookie;
	uint32_t		portno;
	uint16_t 		vid;
	bool 			tagged;
	uint8_t			table_id_eth_in;
	uint8_t			table_id_eth_local; // MAC addresses assigned to local host
	uint8_t			table_id_eth_out;
	rofl::cmacaddr  hwaddr;
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CMEMBERPORT_HPP_ */
