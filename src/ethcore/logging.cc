/*
 * logging.cc
 *
 *  Created on: 23.11.2013
 *      Author: andreas
 *
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "logging.h"

using namespace ethercore;

std::ostream logging::emerg	(std::cerr.rdbuf());
std::ostream logging::alert	(std::cerr.rdbuf());
std::ostream logging::crit	(std::cerr.rdbuf());
std::ostream logging::error	(std::clog.rdbuf());
std::ostream logging::warn	(std::clog.rdbuf());
std::ostream logging::notice(std::cout.rdbuf());
std::ostream logging::info	(std::cout.rdbuf());
std::ostream logging::debug	(std::cout.rdbuf());



void
logging::set_logfile(
			enum logging_level level,
			std::string const& filename)
{
	switch (level) {
	case LOGGING_EMERG:		logging::emerg .rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	case LOGGING_ALERT:		logging::alert .rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	case LOGGING_CRIT:		logging::crit  .rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	case LOGGING_ERROR:		logging::error .rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	case LOGGING_WARN:		logging::warn  .rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	case LOGGING_NOTICE:	logging::notice.rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	case LOGGING_INFO:		logging::info  .rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	case LOGGING_DEBUG:		logging::debug .rdbuf((new std::filebuf())->open(filename.c_str(), std::ios::out)); break;
	}
}

