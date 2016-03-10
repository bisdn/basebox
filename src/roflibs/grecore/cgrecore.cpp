/*
 * cgrecore.cpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#include "cgrecore.hpp"
#include "roflibs/ethcore/cethcore.hpp"

using namespace roflibs::gre;

/*static*/ std::map<rofl::cdptid, cgrecore *> cgrecore::grecores;

void cgrecore::handle_dpt_open() {
  try {
    rofl::crofdpt &dpt = rofl::crofdpt::get_dpt(dptid);

    rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
    switch (state) {
    case STATE_DETACHED: {
      fm.set_command(rofl::openflow::OFPFC_ADD);
    } break;
    case STATE_ATTACHED: {
      fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
    } break;
    }

    // install GRE rule in IP local table
    fm.set_table_id(gre_local_table_id);
    fm.set_priority(0xd000);
    fm.set_cookie(cookie_miss_entry);

    fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
    fm.set_match().set_ip_proto(GRE_IP_PROTO);
    fm.set_match().set_matches().add_match(
        rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0));
    fm.set_match().set_matches().add_match(
        rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(
            GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING));
    fm.set_instructions()
        .set_inst_apply_actions()
        .set_actions()
        .add_action_output(rofl::cindex(0))
        .set_port_no(rofl::openflow::OFPP_CONTROLLER);
    fm.set_instructions()
        .set_inst_apply_actions()
        .set_actions()
        .set_action_output(rofl::cindex(0))
        .set_max_len(1526);
    dpt.send_flow_mod_message(rofl::cauxid(0), fm);

    state = STATE_ATTACHED;

    for (std::map<uint32_t, cgreterm_in4 *>::iterator it = terms_in4.begin();
         it != terms_in4.end(); ++it) {
      (*(it->second)).handle_dpt_open(dpt);
    }

    for (std::map<uint32_t, cgreterm_in6 *>::iterator it = terms_in6.begin();
         it != terms_in6.end(); ++it) {
      (*(it->second)).handle_dpt_open(dpt);
    }

  } catch (rofl::eRofDptNotFound &e) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] unable to find data path"
        << e.what() << std::endl;
  } catch (rofl::eRofSockTxAgain &e) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] control channel congested"
        << e.what() << std::endl;
  } catch (rofl::eRofBaseNotConnected &e) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] control channel is down"
        << e.what() << std::endl;
  } catch (...) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] unexpected error" << std::endl;
  }
}

void cgrecore::handle_dpt_close() {
  try {
    state = STATE_DETACHED;

    rofl::crofdpt &dpt = rofl::crofdpt::get_dpt(dptid);

    rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
    fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);

    // remove GRE UDP dst rule in IP local-stage-table (GotoTable gre_table_id)
    // IPv4
    fm.set_table_id(gre_local_table_id);
    fm.set_priority(0xd000);
    fm.set_cookie(cookie_miss_entry);

    fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
    fm.set_match().set_ip_proto(GRE_IP_PROTO);
    fm.set_match().set_matches().add_match(
        rofl::openflow::experimental::gre::coxmatch_ofx_gre_version(0));
    fm.set_match().set_matches().add_match(
        rofl::openflow::experimental::gre::coxmatch_ofx_gre_prot_type(
            GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING));
    dpt.send_flow_mod_message(rofl::cauxid(0), fm);

    for (std::map<uint32_t, cgreterm_in4 *>::iterator it = terms_in4.begin();
         it != terms_in4.end(); ++it) {
      (*(it->second)).handle_dpt_close(dpt);
    }

    for (std::map<uint32_t, cgreterm_in6 *>::iterator it = terms_in6.begin();
         it != terms_in6.end(); ++it) {
      (*(it->second)).handle_dpt_close(dpt);
    }

  } catch (rofl::eRofDptNotFound &e) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] unable to find data path"
        << e.what() << std::endl;
  } catch (rofl::eRofSockTxAgain &e) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] control channel congested"
        << e.what() << std::endl;
  } catch (rofl::eRofBaseNotConnected &e) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] control channel is down"
        << e.what() << std::endl;
  } catch (...) {
    rofcore::logging::error
        << "[rofgre][cgrecore][handle_dpt_open] unexpected error" << std::endl;
  }
}

void cgrecore::handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                rofl::openflow::cofmsg_packet_in &msg) {
  rofcore::logging::debug << "[cgrecore][handle_packet_in] pkt received: "
                          << std::endl
                          << msg;
  // store packet in ethcore and thus, tap devices
  roflibs::eth::cethcore::set_eth_core(dpt.get_dptid())
      .handle_packet_in(dpt, auxid, msg);
}
