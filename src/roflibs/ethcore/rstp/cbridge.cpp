/*
 * cbridge.cpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#include "cbridge.hpp"

using namespace roflibs::eth::rstp;

std::map<cbridgeid, cbridge*>	cbridge::bridges;


void
cbridge::handle_timeout(int opaque, void* data)
{
	switch (opaque) {
	case TIMER_HELLO_WHEN: {

	} break;
	}
}



void
cbridge::event_hello_when()
{
	for (std::map<cportid, cport*>::iterator
			it = ports.begin(); it != ports.end(); ++it) {
		(*(it->second)).send_bpdu_conf(); // VORSICHT: VIELLEICHT AUCH RST, je nach Zustand
	}
}
