/*
 * cgtpcore.cpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#include "cgtpcore.hpp"
#include "roflibs/ethcore/cethcore.hpp"

using namespace roflibs::gtp;

/*static*/std::map<rofl::cdpid, cgtpcore*> cgtpcore::gtpcores;


void
cgtpcore::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {

		rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
		switch (state) {
		case STATE_DETACHED: {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		// install GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv4
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv4);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		fm.set_instructions().drop_inst_goto_table();
		fm.set_instructions().add_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// install GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv6
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv6);
		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		fm.set_instructions().drop_inst_goto_table();
		fm.set_instructions().add_inst_apply_actions().set_actions().
				add_action_output(rofl::cindex(0)).set_port_no(rofl::openflow::OFPP_CONTROLLER);
		fm.set_instructions().set_inst_apply_actions().set_actions().
				set_action_output(rofl::cindex(0)).set_max_len(1526);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// relays and termination points are handled by the relaying and termination application, not here!

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
cgtpcore::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;


		rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);

		// remove GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv4
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv4);
		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// remove GTP UDP dst rule in GTP local-stage-table (ActionOutput Controller) IPv6
		fm.set_table_id(gtp_table_id);
		fm.set_priority(0x4000);
		fm.set_cookie(cookie_miss_entry_ipv6);
		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ip_proto(rofl::fudpframe::UDP_IP_PROTO);
		fm.set_match().set_udp_dst(DEFAULT_GTPU_PORT);
		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		// relays and termination points are handled by the relaying and termination application, not here!

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgtp][cgtpcore][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
cgtpcore::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::debug << "[cgtpcore][handle_packet_in] pkt received: " << std::endl << msg;
	// store packet in ethcore and thus, tap devices
	roflibs::eth::cethcore::set_eth_core(dpt.get_dptid()).handle_packet_in(dpt, auxid, msg);
}



