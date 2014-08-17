/*
 * crofconf.cpp
 *
 *  Created on: 17.12.2013
 *      Author: andreas
 */

#include "crofconf.hpp"

using namespace rofcore;

std::string const crofconf::CONFPATH = std::string("./ipcored.conf");

crofconf* crofconf::scrofconf = (crofconf*)0;


crofconf&
crofconf::get_instance() {
	if (0 == crofconf::scrofconf) {
		crofconf::scrofconf = new crofconf();
	}
	return *(crofconf::scrofconf);
}


void
crofconf::open(std::string const& confpath)
{
	try {
		this->confpath = confpath;
		Config::readFile(confpath.c_str());
	} catch (FileIOException& e) {
		std::cerr << "[ipcore][config] unable to find config file \"" << confpath << "\", continuing with defaults." << std::endl;
	}
}


