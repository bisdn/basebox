/*
 * crofmodel.hpp
 *
 *  Created on: 20.04.2014
 *      Author: andreas
 */

#ifndef CROFMODEL_HPP_
#define CROFMODEL_HPP_

#include <inttypes.h>

#include <string>
#include <iostream>

#include <rofl/common/openflow/cofports.h>
#include <rofl/common/openflow/coftables.h>
#include <rofl/common/openflow/cofdescstats.h>

#include "croflog.hpp"

namespace rofcore {

class crofmodel {

	uint8_t			version; // OFP version

	uint32_t 		n_buffers;
	uint8_t 		n_tables;
	uint8_t 		auxiliary_id;
	uint32_t 		capabilities;

	uint16_t		flags;
	uint16_t		miss_send_len;

	rofl::openflow::cofports			ports;
	rofl::openflow::coftables			tables;
	rofl::openflow::cofdesc_stats_reply	desc_stats;

public:

	/**
	 *
	 */
	crofmodel(
			uint8_t version = rofl::openflow::OFP_VERSION_UNKNOWN);

	/**
	 *
	 */
	virtual
	~crofmodel();

	/**
	 *
	 */
	crofmodel(
			crofmodel const& rofmodel);

	/**
	 *
	 */
	crofmodel&
	operator= (
			crofmodel const& rofmodel);

public:

	/**
	 *
	 */
	void
	clear();

	/**
	 *
	 */
	uint8_t
	get_version() const { return version; };

	/**
	 *
	 */
	void
	set_version(uint8_t version) {
		this->version = version;
		ports.set_version(version);
		tables.set_version(version);
		desc_stats.set_version(version);
	};

	/**
	 *
	 */
	uint32_t
	get_n_buffers() const { return n_buffers; };

	/**
	 *
	 */
	void
	set_n_buffers(uint32_t n_buffers) { this->n_buffers = n_buffers; };

	/**
	 *
	 */
	uint8_t
	get_n_tables() const { return n_tables; };

	/**
	 *
	 */
	void
	set_n_tables(uint8_t n_tables) { this->n_tables = n_tables; };

	/**
	 *
	 */
	uint8_t
	get_auxiliary_id() const { return auxiliary_id; };

	/**
	 *
	 */
	void
	set_auxiliary_id(uint8_t auxiliary_id) { this->auxiliary_id = auxiliary_id; };

	/**
	 *
	 */
	uint32_t
	get_capabilities() const { return capabilities; };

	/**
	 *
	 */
	void
	set_capabilities(uint32_t capabilities) { this->capabilities = capabilities; };

	/**
	 *
	 */
	uint16_t
	get_flags() const { return flags; };

	/**
	 *
	 */
	void
	set_flags(uint16_t flags) { this->flags = flags; };

	/**
	 *
	 */
	uint16_t
	get_miss_send_len() const { return miss_send_len; };

	/**
	 *
	 */
	void
	set_miss_send_len(uint16_t miss_send_len) { this->miss_send_len = miss_send_len; };

	/**
	 *
	 */
	rofl::openflow::cofports const&
	get_ports() const { return ports; };

	/**
	 *
	 */
	rofl::openflow::cofports&
	set_ports() { return ports; };

	/**
	 *
	 */
	rofl::openflow::coftables const&
	get_tables() const { return tables; };

	/**
	 *
	 */
	rofl::openflow::coftables&
	set_tables() { return tables; };

	/**
	 *
	 */
	rofl::openflow::cofdesc_stats_reply const&
	get_desc_stats() const { return desc_stats; };

	/**
	 *
	 */
	rofl::openflow::cofdesc_stats_reply&
	set_desc_stats() { return desc_stats; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, crofmodel const& rm) {
		os << rofcore::indent(0) << "<crofmodel version:" << (int)rm.get_version() << " >" << std::endl;
		os << rofcore::indent(2) << "<n_buffers: " << (int)rm.get_n_buffers() << " >" << std::endl;
		os << rofcore::indent(2) << "<n_tables: " << (int)rm.get_n_tables() << " >" << std::endl;
		os << rofcore::indent(2) << "<auxiliary-id: " << (int)rm.get_auxiliary_id() << " >" << std::endl;
		os << rofcore::indent(2) << "<capabilities: " << (int)rm.get_capabilities() << " >" << std::endl;
		os << rofcore::indent(2) << "<flags: " << (int)rm.get_flags() << " >" << std::endl;
		os << rofcore::indent(2) << "<miss-send-len: " << (int)rm.get_miss_send_len() << " >" << std::endl;
		rofcore::indent i(2);
		os << rm.get_desc_stats();
		os << rm.get_ports();
		os << rm.get_tables();
		return os;
	};
};

}; // end of namespace rofcore

#endif /* CROFMODEL_HPP_ */
