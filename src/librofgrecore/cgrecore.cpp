/*
 * cgrecore.cpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#include "cgrecore.hpp"

using namespace roflibs::gre;

/*static*/std::map<rofl::cdpid, cgrecore*> cgrecore::grecores;


void
cgrecore::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {

		rofl::openflow::cofflowmod fm(dpt.get_version());
		switch (state) {
		case STATE_DETACHED: {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		// install GRE rule in IP local table
		fm.set_table_id(ip_local_table_id);
		fm.set_priority(0xd000);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ip_proto(GRE_IP_PROTO);
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0));
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING));

		fm.set_instructions().set_inst_goto_table().set_table_id(gre_local_table_id);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] unexpected error" << std::endl;
	}
}



void
cgrecore::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;


		rofl::openflow::cofflowmod fm(dpt.get_version());
		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);

		// remove GRE UDP dst rule in IP local-stage-table (GotoTable gre_table_id) IPv4
		fm.set_table_id(ip_local_table_id);
		fm.set_priority(0xd000);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ip_proto(GRE_IP_PROTO);
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0));
		fm.set_match().set_matches().add_match(
				rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING));

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] unable to find data path" << e.what() << std::endl;
	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] control channel congested" << e.what() << std::endl;
	} catch (rofl::eRofBaseNotConnected& e) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] control channel is down" << e.what() << std::endl;
	} catch (...) {
		rofcore::logging::error << "[rofgre][cgrecore][handle_dpt_open] unexpected error" << std::endl;
	}
}


