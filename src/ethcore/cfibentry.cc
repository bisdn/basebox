/*
 * cfibentry.cc
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#include <cfibentry.h>

using namespace ethercore;

cfibentry::cfibentry(
		cfibentry_owner *fib,
		uint8_t src_stage_table_id,
		uint8_t dst_stage_table_id,
		rofl::cmacaddr lladdr,
		uint64_t dpid,
		uint16_t vid,
		uint32_t out_port_no,
		bool tagged) :
		fib(fib),
		dpid(dpid),
		vid(vid),
		portno(out_port_no),
		tagged(tagged),
		lladdr(lladdr),
		src_stage_table_id(src_stage_table_id),
		dst_stage_table_id(dst_stage_table_id),
		entry_timeout(CFIBENTRY_DEFAULT_TIMEOUT)
{
	flow_mod_configure(FLOW_MOD_ADD);

	register_timer(CFIBENTRY_ENTRY_EXPIRED, entry_timeout);
}


cfibentry::~cfibentry()
{
	flow_mod_configure(FLOW_MOD_DELETE);
#if 0
	rofl::crofbase *rofbase = fib->get_rofbase();
	rofl::cofdpt *dpt = rofbase->dpt_find(dpid);

	rofbase->send_barrier_request(dpt);
#endif
}



void
cfibentry::handle_timeout(int opaque)
{
	switch (opaque) {
	case CFIBENTRY_ENTRY_EXPIRED: {
		flow_mod_configure(FLOW_MOD_DELETE);
		fib->fib_timer_expired(this);
	} break;
	}
}



void
cfibentry::set_portno(uint32_t portno)
{
	flow_mod_configure(FLOW_MOD_DELETE);

	this->portno = portno;

	reset_timer(CFIBENTRY_ENTRY_EXPIRED, entry_timeout);

	flow_mod_configure(FLOW_MOD_ADD);
}



void
cfibentry::flow_mod_configure(enum flow_mod_cmd_t flow_mod_cmd)
{
	try {
		rofl::crofbase *rofbase = fib->get_rofbase();
		rofl::cofdpt *dpt = rofbase->dpt_find(dpid);

		rofl::cflowentry fe(dpt->get_version());

		/* table 'src_stage_table_id':
		 *
		 * if src mac address is already known, move on to next table, otherwise, send packet to controller
		 *
		 * this allows the controller to learn yet unknown src macs when being received on a port
		 */
		if (true) {
			fe.reset();
			switch (flow_mod_cmd) {
			case FLOW_MOD_ADD:		fe.set_command(OFPFC_ADD);				break;
			case FLOW_MOD_MODIFY:	fe.set_command(OFPFC_MODIFY_STRICT); 	break;
			case FLOW_MOD_DELETE:	fe.set_command(OFPFC_DELETE_STRICT);	break;
			}
			fe.set_table_id(src_stage_table_id);
			fe.set_priority(0x8000);
			fe.set_hard_timeout(entry_timeout);
			fe.match.set_in_port(portno);
			fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_NORMAL, vid);
			fe.match.set_eth_src(lladdr); // yes, indeed: set_eth_src for dst
			fe.instructions.next() = rofl::cofinst_goto_table(dpt->get_version(), dst_stage_table_id);

			rofbase->send_flow_mod_message(dpt, fe);
		}

		rofbase->send_barrier_request(dpt);

		/* table 'dst_stage_table_id':
		 *
		 * add a second rule to dst-stage: checks for destination
		 */
		if (true) {
			fe.reset();
			switch (flow_mod_cmd) {
			case FLOW_MOD_ADD:		fe.set_command(OFPFC_ADD);				break;
			case FLOW_MOD_MODIFY:	fe.set_command(OFPFC_MODIFY_STRICT); 	break;
			case FLOW_MOD_DELETE:	fe.set_command(OFPFC_DELETE_STRICT);	break;
			}
			fe.set_table_id(dst_stage_table_id);
			fe.set_priority(0x8000);
			fe.set_hard_timeout(entry_timeout);
			fe.match.set_vlan_vid(rofl::coxmatch_ofb_vlan_vid::VLAN_TAG_MODE_NORMAL, vid);
			fe.match.set_eth_dst(lladdr);
			fe.instructions.next() = rofl::cofinst_apply_actions(dpt->get_version());
			if (not tagged)
				fe.instructions.back().actions.next() = rofl::cofaction_pop_vlan(dpt->get_version());
			fe.instructions.back().actions.next() = rofl::cofaction_output(dpt->get_version(),portno);

			rofbase->send_flow_mod_message(dpt, fe);
		}

		rofbase->send_barrier_request(dpt);

	} catch (rofl::eRofBaseNotFound& e) {

	}
}


