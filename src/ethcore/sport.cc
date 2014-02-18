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


sport&
sport::get_sport(uint64_t dpid, std::string const& devname)
{
	std::map<uint32_t, sport*>::iterator it;
	if ((it = find_if(sport::sports[dpid].begin(), sport::sports[dpid].end(),
			sport_find_by_devname(devname))) == sport::sports[dpid].end()) {
		throw eSportNotFound();
	}
	return *(it->second);
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
sport::dump_sports()
{
	for (std::map<uint64_t, std::map<uint32_t, sport*> >::iterator
			it = sport::sports.begin(); it != sport::sports.end(); ++it) {
		rofl::logging::info << "[sport][dump] dpid:" << (unsigned long long)it->first << std::endl;
		for (std::map<uint32_t, sport*>::iterator
				jt = it->second.begin(); jt != it->second.end(); ++jt) {
			sport& sp = *(jt->second);
			rofl::indent i(2);
			std::cout << sp;
		}
	}
}


void
sport::destroy_sports()
{
	while (not sport::sports.empty()) {
		sport::destroy_sports((sport::sports.begin())->first);
	}
}


sport::sport(sport_owner *spowner, uint64_t dpid, uint32_t portno, std::string const& devname, uint8_t table_id, uint16_t pvid, unsigned int timer_dump_interval) :
		spowner(spowner),
		dpid(dpid),
		portno(portno),
		devname(devname),
		pvid(0xffff),
		table_id(table_id),
		timer_dump_interval(timer_dump_interval)
{
	if (NULL == spowner) {
		throw eSportInval();
	}
	if (sport::sports[dpid].find(portno) != sport::sports[dpid].end()) {
		throw eSportExists();
	}
	sport::sports[dpid][portno] = this;

	set_pvid(pvid);

	logging::info << "[ethcore][sport] created sport:" << std::endl << *this;
	//register_timer(SPORT_TIMER_DUMP, timer_dump_interval);
}


sport::~sport()
{
	logging::info << "[ethcore][sport] deleting sport:" << std::endl << *this;

	while (not memberships.empty()) {
		std::map<uint16_t, struct vlan_membership_t>::iterator it = memberships.begin();
		drop_membership(it->first);
	}

	if (sport::sports[dpid].find(portno) != sport::sports[dpid].end()) {
		sport::sports[dpid].erase(portno);
	}
}


void
sport::handle_timeout(int opaque)
{
	switch (opaque) {
	case SPORT_TIMER_DUMP: {
		logging::debug << *this << std::endl;
		register_timer(SPORT_TIMER_DUMP, timer_dump_interval);
	} break;
	}
}


uint16_t
sport::get_pvid() const
{
	if (not flags.test(SPORT_FLAG_PVID_VALID)) {
		throw eSportNoPvid();
	}
	return pvid;
}


void
sport::set_pvid(uint16_t pvid)
{
	if (pvid == this->pvid)
		return;

	drop_membership(this->pvid);

	this->pvid = pvid;

	add_membership(this->pvid, false);
}


bool
sport::get_is_tagged(uint16_t vid)
{
	if (memberships.find(vid) == memberships.end()) {
		throw eSportNotMember();
	}
	return memberships[vid].get_tagged();
}


void
sport::add_membership(uint16_t vid, bool tagged)
{
	logging::info << "[ethcore][sport] adding membership for vid:" << (int)vid << "(" << tagged << ")" << std::endl << *this;

	try {
		/* membership already exists in specified mode */
		if (memberships.find(vid) != memberships.end()) {
			if (memberships[vid].tagged == tagged) {
				return;
			} else {
				logging::warn << "[ethcore - sport] unable to add membership for vid:" << (int)vid << ", already member, drop existing membership first" << std::endl;
				throw eSportFailed();
			}
		}

		if (tagged) {
			memberships[vid].vid 		= vid;
			memberships[vid].tagged 	= tagged; // = true
			memberships[vid].group_id 	= spowner->get_idle_group_id();

			flow_mod_add(vid, memberships[vid].tagged);

		} else {

			// check for pvid collision
			if (flags.test(SPORT_FLAG_PVID_VALID)) {

				logging::warn << "[ethcore - sport] unable to set pvid:" << (int)vid << ", as there is a pvid already, drop existing membership first" << std::endl;
				throw eSportFailed();

			} else {

				memberships[vid].vid 		= vid;
				memberships[vid].tagged 	= tagged; // = false
				memberships[vid].group_id 	= spowner->get_idle_group_id();

				flow_mod_add(vid, memberships[vid].tagged);

				flags.set(SPORT_FLAG_PVID_VALID);
				pvid = vid;
			}

		}

	} catch (eSportExhausted& e) {
		memberships.erase(vid);
		logging::warn << "sport::add_membership() no idle group-id available, unable to add membership for VID:" << (int)vid << std::endl;
		throw eSportFailed();

	} catch (rofl::eRofBaseNotFound& e) {
		memberships.erase(vid);
		logging::warn << "sport::add_membership() dpt handle for dpid:" << (unsigned long long)dpid << " not found" << std::endl;
		throw eSportFailed();
	}
}


void
sport::drop_membership(uint16_t vid)
{
	logging::info << "[ethcore][sport] dropping membership for vid:" << (int)vid << std::endl << *this;

	if (memberships.find(vid) == memberships.end()) {
		return;
	}

	flow_mod_delete(vid, memberships[vid].tagged);

	spowner->release_group_id(memberships[vid].group_id);

	if (pvid == vid) {
		flags.reset(SPORT_FLAG_PVID_VALID);
		pvid = 0xffff;
	}

	memberships.erase(vid);
}


struct sport::vlan_membership_t&
sport::get_membership(uint16_t vid)
{
	if (memberships.find(vid) == memberships.end()) {
		throw eSportNotMember();
	}
	return memberships[vid];
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
		rofl::crofdpt *dpt = spowner->get_rofbase()->dpt_find(dpid);

		if ((0 == dpt) || (rofl::openflow::OFP_VERSION_UNKNOWN == dpt->get_version())) {
			return;
		}

		// set incoming rule for switch port
		if (true) {
			rofl::cofflowmod fe(dpt->get_version());

			fe.set_command(OFPFC_ADD);
			fe.set_idle_timeout(0);
			fe.set_hard_timeout(0);
			fe.set_table_id(table_id);
			fe.set_priority(0xa000);

			fe.match.set_in_port(portno);
			if (tagged)
				fe.match.set_vlan_vid(vid);
			else
				fe.match.set_vlan_untagged();

			// push vlan tag, when packet was received untagged
			if (not tagged) {
				fe.instructions.add_inst_apply_actions().get_actions().append_action_push_vlan(rofl::fvlanframe::VLAN_CTAG_ETHER);
				fe.instructions.set_inst_apply_actions().get_actions().append_action_set_field(rofl::coxmatch_ofb_vlan_vid(vid));
			}
			// either tagged or untagged: move packet to next table
			fe.instructions.add_inst_goto_table().set_table_id(table_id + 1);

			dpt->send_flow_mod_message(fe);
		}
#if 0
		// set group table entry for this outgoing switch port
		if (true) {
			rofl::cgroupentry ge(dpt->get_version());

			ge.set_command(OFPGC_ADD);
			ge.set_group_id(memberships[vid].group_id);
			ge.set_type(OFPGT_ALL);

			rofl::cofbucket bucket(dpt->get_version());
			if (not tagged) {
				bucket.get_actions().append_action_pop_vlan();
			}
			bucket.get_actions().append_action_output(portno);

			ge.buckets.append_bucket(bucket);

			dpt->send_group_mod_message(ge);
		}
#endif
	} catch (rofl::eRofBaseNotFound& e) {
		logging::error << "sport::flow_mod_add() unable to find crofbase or cofdpt instance " << *this << std::endl;
		throw;
	}
}


void
sport::flow_mod_delete(uint16_t vid, bool tagged)
{
	try {
		rofl::crofdpt *dpt = spowner->get_rofbase()->dpt_find(dpid);

		if ((0 == dpt) || (rofl::openflow::OFP_VERSION_UNKNOWN == dpt->get_version())) {
			return;
		}

		// set incoming rule for switch port
		if (true) {
			rofl::cofflowmod fe(dpt->get_version());

			fe.set_command(OFPFC_DELETE_STRICT);
			fe.set_idle_timeout(0);
			fe.set_hard_timeout(0);
			fe.set_table_id(table_id);
			fe.set_priority(0xa000);

			fe.match.set_in_port(portno);
			if (tagged)
				fe.match.set_vlan_vid(vid);
			else
				fe.match.set_vlan_untagged();

			dpt->send_flow_mod_message(fe);
		}
#if 0
		// set group table entry for this outgoing switch port
		if (true) {
			rofl::cgroupentry ge(dpt->get_version());

			ge.set_command(OFPGC_ADD); // DELETE?
			ge.set_group_id(memberships[vid].group_id);
			ge.set_type(OFPGT_ALL);

			dpt->send_group_mod_message(ge);
		}
#endif
	} catch (rofl::eRofBaseNotFound& e) {
		logging::error << "sport::flow_mod_delete() unable to find crofbase instance " << *this << std::endl;
	}
}


