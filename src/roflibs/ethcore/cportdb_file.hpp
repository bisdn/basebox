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
	cportdb_file() { read_config(); };

	/**
	 *
	 */
	virtual ~cportdb_file() {};

private:

	/**
	 *
	 */
	void
	read_config();

private:

	static const std::string 		DEFAULT_CONFIG_FILE;
	static const uint16_t			DEFAULT_PORT_VID;

	std::string 					config_file;
	uint16_t						port_vid;
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CPORTCONF_FILE_HPP_ */
