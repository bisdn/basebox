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
sport::get_sport(uint64_t dpid, uint32_t portno)
{
	if (sport::sports[dpid].find(portno) == sport::sports[dpid].end()) {
		throw eSportNotFound();
	}
	return *(sport::sports[dpid][portno]);
}


void
sport::destroy_sports(uint64_t dpid)
{
	while (not sport::sports[dpid].empty()) {
		delete (sport::sports[dpid].begin()->second);
	}
	sport::sports.erase(dpid);
}


void
sport::destroy_sports()
{
	while (not sport::sports.empty()) {
		sport::destroy_sports((sport::sports.begin())->first);
	}
}


sport::sport(sport_owner *spowner, uint64_t dpid, uint32_t portno, uint16_t pvid, uint8_t table_id) :
		spowner(spowner),
		dpid(dpid),
		portno(portno),
		pvid(0xffff),
		table_id(table_id)
{
	if (NULL == spowner) {
		throw eSportInval();
	}
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
	try {
		if (not tagged) {
			if (pvid == vid) {
				// already active pvid, do nothing
				assert(memberships[vid].tagged == false);
			} else {
				// pvid is changing, drop membership from old one
				if (memberships.find(vid) != memberships.end()) {
					memberships.erase(pvid);
				}
				pvid = vid;
				memberships[pvid].tagged = /*untagged=*/false;
				if (0 == memberships[pvid].group_id) {
					memberships[pvid].group_id = spowner->get_idle_group_id();
				}
				flow_mod_add(pvid, false);
			}
		}
		else if (tagged) {
			if (pvid == vid) {
				// invalid => pvid cannot be dropped in tagged mode
				throw eSportInval();
			}
			else {
				if (memberships.find(vid) == memberships.end()) {
					// add vid to list of active memberships
					memberships[vid].tagged = /*tagged*/true;
					if (0 == memberships[pvid].group_id) {
						memberships[vid].group_id = spowner->get_idle_group_id();
					}
					flow_mod_add(vid, true);
				} else {
					// make sure that vid is still set to tagged mode
					assert(memberships[vid].tagged == true);
				}
			}
		}

	} catch (eSportExhausted& e) {
		memberships.erase(vid);
		logging::error << "sport::add_membership() no idle group-id available, unable to add membership for VID:" << (int)vid << std::endl;
		throw eSportFailed();

	} catch (rofl::eRofBaseNotFound& e) {
		memberships.erase(vid);
		logging::error << "sport::add_membership() dpt handle for dpid:" << (unsigned long long)dpid << " not found" << std::endl;
		throw eSportFailed();
	}
}


void
sport::drop_membership(uint16_t vid)
{
	if (memberships.find(vid) == memberships.end()) {
		return;
	}

	flow_mod_delete(vid, memberships[vid].vid);

	spowner->release_group_id(memberships[vid].group_id);

	memberships.erase(vid);
}


uint32_t
sport::get_groupid(uint16_t vid)
{
	if (memberships.find(vid) == memberships.end())
		throw eSportInval();
	return memberships[vid].group_id;
}


void
sport::flow_mod_add(uint16_t vid, bool tagged)
{
	try {
		rofl::cofdpt *dpt = spowner->get_rofbase()->dpt_find(dpid);

		// set incoming rule for switch port
		if (true) {
			rofl::cflowentry fe(dpt->get_version());

			fe.set_command(OFPFC_ADD);
			fe.set_idle_timeout(0);
			fe.set_hard_timeout(0);
			fe.set_table_id(table_id);
			fe.set_priority(0xa000);

			fe.match.set_in_port(portno);
			if (tagged)
				fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_NORMAL, vid);
			else
				fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_UNTAGGED, OFPVID_NONE);

			// push vlan tag, when packet was received untagged
			if (not tagged) {
				fe.instructions.next() = rofl::cofinst_apply_actions(dpt->get_version());
				fe.instructions.back().actions.next() = rofl::cofaction_push_vlan(dpt->get_version(), rofl::fvlanframe::VLAN_CTAG_ETHER);
				fe.instructions.back().actions.next() = rofl::cofaction_set_field(dpt->get_version(), rofl::coxmatch_ofb_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_NORMAL, vid));
			}
			// either tagged or untagged: move packet to next table
			fe.instructions.next() = rofl::cofinst_goto_table(dpt->get_version(), table_id + 1);

			spowner->get_rofbase()->send_flow_mod_message(dpt, fe);
		}

		// set group table entry for this outgoing switch port
		if (true) {
			rofl::cgroupentry ge(dpt->get_version());

			ge.set_command(OFPGC_ADD);
			ge.set_group_id(memberships[vid].group_id);
			ge.set_type(OFPGT_ALL);

			ge.buckets.next() = rofl::cofbucket(dpt->get_version(), 0, 0, 0);
			if (not tagged) {
				ge.buckets.back().actions.next() = rofl::cofaction_pop_vlan(dpt->get_version());
			}
			ge.buckets.back().actions.next() = rofl::cofaction_output(dpt->get_version(), portno);

			spowner->get_rofbase()->send_group_mod_message(dpt, ge);
		}

	} catch (rofl::eRofBaseNotFound& e) {
		logging::error << "sport::flow_mod_add() unable to find crofbase or cofdpt instance " << *this << std::endl;
		throw;
	}
}


void
sport::flow_mod_delete(uint16_t vid, bool tagged)
{
	try {
		rofl::cofdpt *dpt = spowner->get_rofbase()->dpt_find(dpid);

		// set incoming rule for switch port
		if (true) {
			rofl::cflowentry fe(dpt->get_version());

			fe.set_command(OFPFC_DELETE_STRICT);
			fe.set_idle_timeout(0);
			fe.set_hard_timeout(0);
			fe.set_table_id(table_id);
			fe.set_priority(0x8000);

			fe.match.set_in_port(portno);
			if (tagged)
				fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_NORMAL, vid);
			else
				fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_UNTAGGED, OFPVID_NONE);

			fe.instructions.next() = rofl::cofinst_goto_table(dpt->get_version(), table_id + 1);

			spowner->get_rofbase()->send_flow_mod_message(dpt, fe);
		}

		// set group table entry for this outgoing switch port
		if (true) {
			rofl::cgroupentry ge(dpt->get_version());

			ge.set_command(OFPGC_ADD);
			ge.set_group_id(memberships[vid].group_id);
			ge.set_type(OFPGT_ALL);

			ge.buckets.next() = rofl::cofbucket(dpt->get_version(), 0, 0, 0);
			if (not tagged) {
				ge.buckets.back().actions.next() = rofl::cofaction_pop_vlan(dpt->get_version());
			}
			ge.buckets.back().actions.next() = rofl::cofaction_output(dpt->get_version(), portno);

			spowner->get_rofbase()->send_group_mod_message(dpt, ge);
		}

	} catch (rofl::eRofBaseNotFound& e) {
		logging::error << "sport::flow_mod_delete() unable to find crofbase instance " << *this << std::endl;
	}
}


