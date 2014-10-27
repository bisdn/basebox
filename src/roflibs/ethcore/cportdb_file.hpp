/*
 * cportconf_file.hpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#ifndef CPORTCONF_FILE_HPP_
#define CPORTCONF_FILE_HPP_

#include <string>
#include <ostream>
#include <libconfig.h++>
#include <roflibs/ethcore/cportdb.hpp>
#include <roflibs/netlink/cconfig.hpp>

namespace roflibs {
namespace ethernet {

class cportdb_file : public cportdb {
public:

	/**
	 *
	 */
	cportdb_file() :
		config_file(cportdb_file::DEFAULT_CONFIG_FILE),
		port_vid(cportdb_file::DEFAULT_PORT_VID) {};

	/**
	 *
	 */
	virtual ~cportdb_file() {};

public:

	/**
	 *
	 */
	void
	read_config(const std::string& config_file);

private:

	/**
	 *
	 */
	void
	parse_datapath(ethcore::cconfig& config, libconfig::Setting& datapath);

	/**
	 *
	 */
	void
	parse_datapath_phy_port(
			ethcore::cconfig& config, libconfig::Setting& port,
			const rofl::cdpid& dpid, uint16_t pvid);

	/**
	 *
	 */
	void
	parse_datapath_eth_endpnt(
			ethcore::cconfig& config, libconfig::Setting& endpnt,
			const rofl::cdpid& dpid, uint16_t pvid);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cportdb_file& portdb) {
		os << dynamic_cast<const cportdb&>( portdb );
		return os;
	};

private:

	static const std::string 		ETHCORE_CONFIG_DPT_LIST;
	static const std::string 		DEFAULT_CONFIG_FILE;
	static const uint16_t			DEFAULT_PORT_VID;

	std::string 					config_file;
	uint16_t						port_vid;
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CPORTCONF_FILE_HPP_ */
