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

#include "cipcore.h"
#include "clogging.h"

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

protected:

	/**
	 *
	 */
	virtual void
	handle_dpt_open(
			rofl::crofdpt& dpt) {
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
		cipcore::get_instance().handle_port_status(dpt, auxid, msg);
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
