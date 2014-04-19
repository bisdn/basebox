/*
 * croflog.cpp
 *
 *  Created on: 23.11.2013
 *      Author: andreas
 *
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "croflog.hpp"

using namespace rofcore;

std::filebuf croflog::devnull;
std::ostream croflog::emerg	 (&croflog::devnull);
std::ostream croflog::alert  (&croflog::devnull);
std::ostream croflog::crit   (&croflog::devnull);
std::ostream croflog::error  (&croflog::devnull);
std::ostream croflog::warn   (&croflog::devnull);
std::ostream croflog::notice (&croflog::devnull);
std::ostream croflog::info   (&croflog::devnull);
std::ostream croflog::debug  (&croflog::devnull);
std::ostream croflog::trace  (&croflog::devnull);

std::streamsize croflog::width(70);
unsigned int indent::width(0);


void
croflog::init()
{
	if (not croflog::devnull.is_open()) {
		croflog::devnull.open("/dev/null", std::ios::out);
	}
}


void
croflog::close()
{
	if (croflog::devnull.is_open()) {
		croflog::devnull.close();
	}
}



void
croflog::set_debug_level(
			unsigned int debug_level)
{
	croflog::init();

	// EMERG
	if (debug_level >= EMERG) {
		croflog::emerg .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::emerg .rdbuf(&croflog::devnull);
	}

	// ALERT
	if (debug_level >= ALERT) {
		croflog::alert .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::alert .rdbuf(&croflog::devnull);
	}

	// CRIT
	if (debug_level >= CRIT) {
		croflog::crit  .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::crit  .rdbuf(&croflog::devnull);
	}

	// ERROR
	if (debug_level >= ERROR) {
		croflog::error .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::error .rdbuf(&croflog::devnull);
	}

	// WARN
	if (debug_level >= WARN) {
		croflog::warn  .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::warn  .rdbuf(&croflog::devnull);
	}

	// NOTICE
	if (debug_level >= NOTICE) {
		croflog::notice.rdbuf(std::cerr.rdbuf());
	} else {
		croflog::notice.rdbuf(&croflog::devnull);
	}

	// INFO
	if (debug_level >= INFO) {
		croflog::info  .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::info  .rdbuf(&croflog::devnull);
	}

	// DEBUG
	if (debug_level >= DBG) {
		croflog::debug .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::debug .rdbuf(&croflog::devnull);
	}

	// TRACE 
	if (debug_level >= TRACE) {
		croflog::trace .rdbuf(std::cerr.rdbuf());
	} else {
		croflog::trace .rdbuf(&croflog::devnull);
	}
}


