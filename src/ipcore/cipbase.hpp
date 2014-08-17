/*
 * cipbase.hpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#ifndef CIPBASE_HPP_
#define CIPBASE_HPP_

#include <iostream>
#include <exception>
#include <rofl/common/crofbase.h>

#include "cipcore.hpp"
#include "clogging.h"
#include "cconfig.hpp"

namespace ipcore {

class cipbase : public rofl::crofbase {

	/**
	 * @brief	pointer to singleton
	 */
	static cipbase*	ipbase;

	/**
	 *
	 */
	cipbase(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) :
						rofl::crofbase(versionbitmap) {};

	/**
	 *
	 */
	virtual
	~cipbase() {};

	/**
	 *
	 */
	cipbase(
			const cipbase& ethbase);

public:

	/**
	 *
	 */
	static cipbase&
	get_instance(
			const rofl::openflow::cofhello_elem_versionbitmap& versionbitmap =
					rofl::openflow::cofhello_elem_versionbitmap()) {
		if (cipbase::ipbase == (cipbase*)0) {
			cipbase::ipbase = new cipbase(versionbitmap);
		}
		return *(cipbase::ipbase);
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
		cipcore::get_instance(dpt.get_dptid(), 0, 1, 1);
		dpt.flow_mod_reset();
		dpt.group_mod_reset();
		dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);
	};

	/**
	 *
	 */
	virtual void
	handle_port_desc_stats_reply(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_desc_stats_reply& msg) {
		dpt.set_ports() = msg.get_ports();
		for (std::map<uint32_t, rofl::openflow::cofport*>::const_iterator
				it = dpt.get_ports().get_ports().begin(); it != dpt.get_ports().get_ports().end(); ++it) {
			const rofl::openflow::cofport& port = *(it->second);
			cipcore::get_instance().set_link(port.get_port_no(), port.get_name(), port.get_hwaddr());
		}
		cipcore::get_instance().handle_dpt_open(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_dpt_close(
			rofl::crofdpt& dpt) {
		cipcore::get_instance().handle_dpt_close(dpt);
	};

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {
		cipcore::get_instance().handle_packet_in(dpt, auxid, msg);
	};

	/**
	 *
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {
		cipcore::get_instance().handle_flow_removed(dpt, auxid, msg);
	};

	/**
	 *
	 */
	virtual void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg) {
		const rofl::openflow::cofport& port = msg.get_port();

		rofcore::logging::debug << "[cipbase] Port-Status message rcvd:" << std::endl << msg;

		try {
			switch (msg.get_reason()) {
			case rofl::openflow::OFPPR_ADD: {
				cipcore::get_instance().add_link(port.get_port_no(), port.get_name(), port.get_hwaddr());
			} break;
			case rofl::openflow::OFPPR_MODIFY: {
				cipcore::get_instance().set_link(port.get_port_no(), port.get_name(), port.get_hwaddr()).
						handle_port_status(dpt, auxid, msg);
			} break;
			case rofl::openflow::OFPPR_DELETE: {
				cipcore::get_instance().drop_link(port.get_port_no());
			} break;
			default: {
				rofcore::logging::debug << "[cipbase] received PortStatus with unknown reason code received, ignoring" << std::endl;
			};
			}

		} catch (rofcore::eNetDevCritical& e) {
			rofcore::logging::debug << "[cipbase] new port created: unable to create tap device: " << msg.get_port().get_name() << std::endl;
		}
	};

	/**
	 *
	 */
	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {
		cipcore::get_instance().handle_error_message(dpt, auxid, msg);
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cipbase& ethbase) {

		return os;
	};
};

}; // end of namespace ethcore

#endif /* CIPBASE_HPP_ */
