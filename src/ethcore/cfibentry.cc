/*
 * cfibentry.cc
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#include <cfibentry.h>

using namespace ethercore;

cfibentry::cfibentry(
		cfibtable *fib,
		uint8_t table_id,
		rofl::cmacaddr dst,
		uint64_t dpid,
		uint16_t vid,
		uint32_t out_port_no,
		bool tagged) :
		fib(fib),
		dpid(dpid),
		vid(vid),
		out_port_no(out_port_no),
		tagged(tagged),
		dst(dst),
		table_id(table_id),
		entry_timeout(CFIBENTRY_DEFAULT_TIMEOUT)
{
	register_timer(CFIBENTRY_ENTRY_EXPIRED, entry_timeout);
}


cfibentry::~cfibentry()
{

}



void
cfibentry::handle_timeout(int opaque)
{
	switch (opaque) {
	case CFIBENTRY_ENTRY_EXPIRED: {
		fib->fib_timer_expired(this);
	} break;
	}
}



void
cfibentry::set_out_port_no(uint32_t out_port_no)
{
	flow_mod_delete();

	this->out_port_no = out_port_no;

	reset_timer(CFIBENTRY_ENTRY_EXPIRED, entry_timeout);

	flow_mod_add();
}



void
cfibentry::flow_mod_add()
{
	try {
		rofl::crofbase *rofbase = fib->get_rofbase();
		rofl::cofdpt *dpt = rofbase->dpt_find(dpid);

		rofl::cflowentry fe(dpt->get_version());

		/* table 1:
		 *
		 * if src mac address is already known, move on to next table, otherwise, send packet to controller
		 *
		 * this allows the controller to learn yet unknown src macs when being received on a port
		 */
		if (true) {
			fe.set_command(OFPFC_ADD);
			fe.set_table_id(table_id+1);
			fe.set_hard_timeout(entry_timeout);
			fe.match.set_eth_src(dst); // yes, indeed: set_eth_src for dst
			fe.instructions.next() = rofl::cofinst_goto_table(table_id+2);

			rofbase->send_flow_mod_message(dpt, fe);
		}

		/* table 2:
		 *
		 */
		if (OFPP12_FLOOD != out_port_no) {
			rofl::cflowentry fe(dpt->get_version());

			fe.set_command(OFPFC_ADD);
			fe.set_table_id(table_id+2);
			fe.set_hard_timeout(entry_timeout);
			fe.match.set_eth_dst(dst);

			fe.instructions.next() = rofl::cofinst_apply_actions(dpt->get_version());
			fe.instructions.back().actions.next() = rofl::cofaction_output(dpt->get_version(), out_port_no);

			rofbase->send_flow_mod_message(dpt, fe);

		} else {

			rofl::cflowentry fe(dpt->get_version());

			fe.set_command(OFPFC_ADD);
			fe.set_table_id(table_id);
			fe.set_hard_timeout(entry_timeout);
			fe.match.set_eth_src(dst);

			fe.instructions.next() = rofl::cofinst_apply_actions(dpt->get_version());
			fe.instructions.back().actions.next() = rofl::cofaction_group(dpt->get_version(), fib->get_flood_group_id(vid));

			rofbase->send_flow_mod_message(dpt, fe);

		}
	} catch (rofl::eRofBaseNotFound& e) {

	}
}



void
cfibentry::flow_mod_modify()
{
	rofl::cflowentry fe(dpt->get_version());

	fe.set_command(OFPFC_MODIFY_STRICT);
	fe.set_table_id(table_id);
	fe.set_hard_timeout(entry_timeout);
	fe.match.set_eth_dst(dst);

	fe.instructions.next() = rofl::cofinst_apply_actions(dpt->get_version());
	fe.instructions.back().actions.next() = rofl::cofaction_output(dpt->get_version(), out_port_no);

	rofbase->send_flow_mod_message(dpt, fe);
}



void
cfibentry::flow_mod_delete()
{
	if (OFPP12_FLOOD != out_port_no) {

		rofl::cflowentry fe(dpt->get_version());

		fe.set_command(OFPFC_DELETE_STRICT);
		fe.set_table_id(table_id);
		fe.match.set_eth_dst(dst);

		rofbase->send_flow_mod_message(dpt, fe);

	} else {

		rofl::cflowentry fe(dpt->get_version());

		fe.set_command(OFPFC_DELETE_STRICT);
		fe.set_table_id(table_id);
		fe.match.set_eth_src(dst);

		rofbase->send_flow_mod_message(dpt, fe);

	}
}



