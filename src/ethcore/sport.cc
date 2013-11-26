/*
 * port.cc
 *
 *  Created on: 26.11.2013
 *      Author: andreas
 */

#include "sport.h"

using namespace ethercore;

std::map<uint64_t, std::map<uint32_t, sport*> > sport::sports;


sport&
sport::get_sport(uint64_t dpid, uint32_t portno, uint16_t pvid = SPORT_DEFAULT_PVID)
{
	if (sport::sports[dpid].find(portno) == sport::sports[dpid].end()) {
		new sport(dpid, portno, pvid);
	}
	return *(sport::sports[dpid][portno]);
}


sport::sport(uint64_t dpid, uint32_t portno, uint16_t pvid) :
		dpid(dpid),
		portno(portno),
		pvid(pvid)
{
	if (sport::sports[dpid].find(portno) != sport::sports[dpid].end()) {
		throw eSportExists();
	}
	sport::sports[dpid][portno] = this;
	// this is the default membership: on pvid in untagged mode
	add_membership(pvid, /*tagged=*/false);
}


sport::~sport()
{
	if (sport::sports[dpid].find(portno) != sport::sports[dpid].end()) {
		sport::sports[dpid].erase(portno);
	}
}


void
sport::add_membership(uint16_t vid, bool tagged)
{
	// change our default pvid, drop the old default membership, add new one
	if ((not tagged) && (pvid != vid)) {
		drop_membership(pvid);
		memberships.erase(pvid);
		pvid = vid;
		memberships[pvid] = false;
	}
	else if (tagged) {
		if (memberships.find(vid) == memberships.end()) {
			memberships[vid] = true;
		} else {
			rofl::logging::crit << "sport vid membership, critical inconsistency, aborting " << *this << std::endl;
			assert(memberships[vid] == false);
		}
	}
}


void
sport::drop_membership(uint16_t vid)
{

}


