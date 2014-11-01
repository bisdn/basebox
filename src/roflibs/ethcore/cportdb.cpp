/*
 * cportdb.cpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#include "cportdb.hpp"
#include "roflibs/ethcore/cportdb_file.hpp"

using namespace roflibs::eth;

/*static*/std::map<std::string, cportdb*> cportdb::portdbs;

/*static*/
cportdb&
cportdb::get_portdb(const std::string& name)
{
	if (cportdb::portdbs.find(name) == cportdb::portdbs.end()) {
		if (name == std::string("file")) {
			cportdb::portdbs[name] = new cportdb_file();
		} else {
			throw ePortDBNotFound("cportdb::get_portdb()");
		}
	}
	return *(cportdb::portdbs[name]);
}
