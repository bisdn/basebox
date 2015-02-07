/*
 * cportconf_file.hpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#ifndef CGTPDB_FILE_HPP_
#define CGTPDB_FILE_HPP_

#include <string>
#include <ostream>
#include <libconfig.h++>
#include <roflibs/gtpcore/cgtpcoredb.hpp>
#include <roflibs/netlink/cconfig.hpp>

namespace roflibs {
namespace gtp {

class cgtpcoredb_file : public cgtpcoredb {
public:

	/**
	 *
	 */
	cgtpcoredb_file() :
		config_file(cgtpcoredb_file::DEFAULT_CONFIG_FILE)
	{};

	/**
	 *
	 */
	virtual
	~cgtpcoredb_file()
	{};

public:

	/**
	 *
	 */
	void
	read_config(
			const std::string& config_file,
			const std::string& prefix = std::string(""));

private:

	/**
	 *
	 */
	void
	parse_datapath(
			ethcore::cconfig& config,
			libconfig::Setting& datapath);

	/**
	 *
	 */
	void
	parse_datapath_relay(
			ethcore::cconfig& config,
			libconfig::Setting& relay,
			const rofl::cdpid& dpid);

	/**
	 *
	 */
	void
	parse_datapath_term(
			ethcore::cconfig& config,
			libconfig::Setting& term,
			const rofl::cdpid& dpid);

	/**
	 *
	 */
	void
	parse_datapath_termdev(
			ethcore::cconfig& config,
			libconfig::Setting& termdev,
			const rofl::cdpid& dpid);

public:

	friend std::ostream&
	operator<< (
			std::ostream& os, const cgtpcoredb_file& portdb) {
		os << dynamic_cast<const cgtpcoredb&>( portdb );
		return os;
	};

private:

	static const std::string 		GTPCORE_CONFIG_DPT_LIST;
	static const std::string 		DEFAULT_CONFIG_FILE;

	std::string 					config_file;
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CPORTCONF_FILE_HPP_ */
