/*
 * cfibentry.cpp
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#include "cfibentry.hpp"

using namespace roflibs::eth;

void cfibentry::handle_dpt_open(rofl::crofdpt &dpt) {
  try {
    assert(dpt.get_dptid() == dptid);

    state = STATE_ATTACHED;

    if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version_negotiated()) {
      return;
    }

    // check for existence of our port on dpt
    if (not dpt.get_ports().has_port(portno)) {
      return;
    }

    // src table
    rofl::openflow::cofflowmod fm_src_table(dpt.get_version_negotiated());
    fm_src_table.set_command(rofl::openflow::OFPFC_ADD);
    fm_src_table.set_table_id(src_stage_table_id);
    fm_src_table.set_priority(0x8000);
    fm_src_table.set_cookie(cookie_src);
    fm_src_table.set_idle_timeout(entry_timeout);
    fm_src_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
    fm_src_table.set_match().set_in_port(portno);
    fm_src_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
    fm_src_table.set_match().set_eth_src(
        lladdr); // yes, indeed: set_eth_src for dst
    fm_src_table.set_instructions().add_inst_goto_table().set_table_id(
        src_stage_table_id + 1);
    dpt.send_flow_mod_message(rofl::cauxid(0), fm_src_table);

    // dst table
    rofl::openflow::cofflowmod fm_dst_table(dpt.get_version_negotiated());
    fm_dst_table.set_command(rofl::openflow::OFPFC_ADD);
    fm_dst_table.set_table_id(dst_stage_table_id);
    fm_dst_table.set_priority(0x8000);
    fm_dst_table.set_cookie(cookie_dst);
    fm_dst_table.set_idle_timeout(entry_timeout);
    fm_dst_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
    fm_dst_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
    fm_dst_table.set_match().set_eth_dst(lladdr);
    rofl::cindex index(0);
    if (not tagged) {
      fm_dst_table.set_instructions()
          .set_inst_apply_actions()
          .set_actions()
          .add_action_pop_vlan(index++);
    }
    fm_dst_table.set_instructions()
        .set_inst_apply_actions()
        .set_actions()
        .add_action_output(index++)
        .set_port_no(portno);
    dpt.send_flow_mod_message(rofl::cauxid(0), fm_dst_table);

  } catch (rofl::eRofSockTxAgain &e) {
    // TODO: handle congested control channel
  }
}

void cfibentry::handle_dpt_close(rofl::crofdpt &dpt) {
  try {
    assert(dpt.get_dptid() == dptid);

    state = STATE_DETACHED;

    if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version_negotiated()) {
      return;
    }

    // check for existence of our port on dpt
    if (not dpt.get_ports().has_port(portno)) {
      return;
    }

    // src table
    rofl::openflow::cofflowmod fm_src_table(dpt.get_version_negotiated());
    fm_src_table.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
    fm_src_table.set_table_id(src_stage_table_id);
    fm_src_table.set_priority(0x8000);
    fm_src_table.set_cookie(cookie_src);
    fm_src_table.set_idle_timeout(entry_timeout);
    fm_src_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
    fm_src_table.set_match().set_in_port(portno);
    fm_src_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
    fm_src_table.set_match().set_eth_src(
        lladdr); // yes, indeed: set_eth_src for dst
    dpt.send_flow_mod_message(rofl::cauxid(0), fm_src_table);

    // dst table
    rofl::openflow::cofflowmod fm_dst_table(dpt.get_version_negotiated());
    fm_dst_table.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
    fm_dst_table.set_table_id(dst_stage_table_id);
    fm_dst_table.set_priority(0x8000);
    fm_dst_table.set_cookie(cookie_dst);
    fm_dst_table.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);
    fm_dst_table.set_idle_timeout(entry_timeout);
    fm_dst_table.set_match().set_vlan_vid(vid | rofl::openflow::OFPVID_PRESENT);
    fm_dst_table.set_match().set_eth_dst(lladdr);
    dpt.send_flow_mod_message(rofl::cauxid(0), fm_dst_table);

  } catch (rofl::eRofSockTxAgain &e) {
    // TODO: handle congested control channel
  }
}

void cfibentry::handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                 rofl::openflow::cofmsg_packet_in &msg) {
  try {
    // oops ...
    if (STATE_ATTACHED != state) {
      return;
    }

    // sanity check
    if (rofl::openflow::OFP_VERSION_UNKNOWN == dpt.get_version_negotiated()) {
      return;
    }

    // has station moved to another port? src-stage flowmod entry has not
    // matched causing this packet-in event
    uint32_t in_port = msg.get_match().get_in_port();
    if (in_port != portno) {
      handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
      portno = in_port;
      handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
    }

#if 0
		// handle buffered packet
		rofl::openflow::cofactions actions(dpt.get_version_negotiated());
		rofl::cindex index(0);
		if (not tagged) {
			actions.add_action_pop_vlan(index++);
		}
		actions.add_action_output(index++).set_port_no(portno);

		if (msg.get_buffer_id() == rofl::openflow::OFP_NO_BUFFER) {
			dpt.send_packet_out_message(auxid, msg.get_buffer_id(),
					msg.get_match().get_in_port(), actions,
					msg.get_packet().soframe(), msg.get_packet().framelen());
		} else {
			dpt.send_packet_out_message(auxid, msg.get_buffer_id(),
					msg.get_match().get_in_port(), actions);
		}
#endif

  } catch (rofl::eRofSockTxAgain &e) {
    // TODO: handle control channel congestion

  } catch (rofl::openflow::eOxmNotFound &e) {
  }
}

void cfibentry::handle_flow_removed(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_flow_removed &msg) {
  if (not flags.test(FLAG_PERMANENT_ENTRY)) {
    fib->fib_expired(*this);
  }
}

void cfibentry::handle_port_status(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid,
                                   rofl::openflow::cofmsg_port_status &msg) {
  if (STATE_ATTACHED != state) {
    return;
  }

  if (msg.get_port().get_port_no() != portno) {
    return;
  }

  switch (msg.get_reason()) {
  case rofl::openflow::OFPPR_ADD: {
    handle_dpt_open(dpt);
  } break;
  case rofl::openflow::OFPPR_MODIFY: {
    // TODO: port up/down?
  } break;
  case rofl::openflow::OFPPR_DELETE: {
    handle_dpt_close(dpt);
  } break;
  }
}

void cfibentry::handle_error_message(rofl::crofdpt &dpt,
                                     const rofl::cauxid &auxid,
                                     rofl::openflow::cofmsg_error &msg) {}
