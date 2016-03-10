/*
 * dptnexthop.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#include "cnexthop.hpp"
#include "cipcore.hpp"
#include "roflibs/ethcore/cethcore.hpp"

using namespace roflibs::ip;

void cnexthop_in4::handle_dpt_open() {
  try {
    rofl::crofdpt &dpt = rofl::crofdpt::get_dpt(dptid);

    rofcore::cnetlink &netlink = rofcore::cnetlink::get_instance();

    // the route ...
    const rofcore::crtroute_in4 &rtr =
        netlink.get_routes_in4(get_rttblid()).get_route(get_rtindex());

    // ... with its destination reachable via this next hop ...
    const rofcore::crtnexthop_in4 &rtn =
        rtr.get_nexthops_in4().get_nexthop(get_nhindex());

    // in the meantime: watch our associated neighbour and track his fate
    rofcore::cnetlink_neighbour_observer::watch(rtn.get_ifindex(),
                                                rtn.get_gateway());

    // ... attached to this link (crtlink) ...
    const rofcore::crtlink &rtl =
        netlink.get_links().get_link(rtn.get_ifindex());

    // ... and the link's dpt representation (cdptlink) needed for OFP related
    // data ...
    const roflibs::ip::clink &dpl =
        cipcore::get_ip_core(dptid).get_link(rtn.get_ifindex());

    // .. and associated to this neighbour
    const rofcore::crtneigh_in4 &rtb =
        dpl.get_neigh_in4(rtn.get_gateway()).get_crtneigh_in4();
    const roflibs::ip::cneigh_in4 &neigh = dpl.get_neigh_in4(rtb.get_dst());

    // local outgoing interface mac address
    const rofl::cmacaddr &eth_src = rtl.get_hwaddr();

    // neighbour mac address
    const rofl::cmacaddr &eth_dst = rtb.get_lladdr();

    // local VLAN associated with interface
    bool tagged = dpl.get_vlan_tagged();
    uint16_t vid = dpl.get_vlan_vid();

    // local outgoing interface => OFP portno
    // uint32_t out_portno 			= dpl.get_ofp_port_no();

    rofl::openflow::cofflowmod fe(dpt.get_version_negotiated());

    switch (state) {
    case STATE_DETACHED: {
      fe.set_command(rofl::openflow::OFPFC_ADD);
    } break;
    case STATE_ATTACHED: {
      fe.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
    } break;
    }

    fe.set_buffer_id(
        rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()));
    fe.set_idle_timeout(0);
    fe.set_hard_timeout(0);
    fe.set_priority(0xf000 + (rtr.get_prefixlen() << 8) + rtn.get_weight());
    fe.set_table_id(out_ofp_table_id);
    fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

    fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
    fe.set_match().set_ipv4_dst(rtr.get_ipv4_dst(), rtr.get_ipv4_mask());
    fe.set_instructions()
        .set_inst_apply_actions()
        .set_actions()
        .add_action_group(rofl::cindex(0))
        .set_group_id(neigh.get_group_id());
    fe.set_instructions().set_inst_goto_table().set_table_id(out_ofp_table_id +
                                                             1);

    fe.set_cookie(cookie);
    dpt.send_flow_mod_message(rofl::cauxid(0), fe);

    state = STATE_ATTACHED;

  } catch (rofcore::eNetLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_open] unable to find link"
        << e.what() << std::endl;
  } catch (rofcore::crtlink::eRtLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_open] unable to find address"
        << e.what() << std::endl;
  } catch (eNeighNotFound &e) {
    rofcore::logging::debug
        << "[rofip][cnexthop_in4][handle_dpt_open] unable to find dst neighbour"
        << e.what() << std::endl;

    /* non-critical error: just indicating that the kernel has not yet resolved
     * (via ARP or NDP)
     * the neighbour carrying this next-hops's destination address.
     *
     * Strategy:
     * we have subscribed for notifications from cnetlink above (see call to
     * watch() method)
     * and will receive notifications about our associated neighbour
     *
     * see methods neigh_in4_created/updated/deleted for further details
     */

  } catch (rofl::eRofDptNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_open] unable to find data path"
        << e.what() << std::endl;
  } catch (rofl::eRofSockTxAgain &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_open] control channel congested"
        << e.what() << std::endl;
  } catch (rofl::eRofBaseNotConnected &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_open] control channel is down"
        << e.what() << std::endl;
  } catch (...) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_open] unexpected error"
        << std::endl;
  }
}

void cnexthop_in4::handle_dpt_close() {
  try {
    rofl::crofdpt &dpt = rofl::crofdpt::get_dpt(dptid);

    rofcore::cnetlink &netlink = rofcore::cnetlink::get_instance();

    // the route ...
    const rofcore::crtroute_in4 &rtr =
        netlink.get_routes_in4(get_rttblid()).get_route(get_rtindex());

    // ... with its destination reachable via this next hop ...
    const rofcore::crtnexthop_in4 &rtn =
        rtr.get_nexthops_in4().get_nexthop(get_nhindex());

    // ... finally, un-watch our associated neighbour
    rofcore::cnetlink_neighbour_observer::unwatch(rtn.get_ifindex(),
                                                  rtn.get_gateway());

    rofl::openflow::cofflowmod fe(dpt.get_version_negotiated());

    fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
    fe.set_buffer_id(
        rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()));
    fe.set_idle_timeout(0);
    fe.set_hard_timeout(0);
    fe.set_priority(0xf000 + (rtr.get_prefixlen() << 8) + rtn.get_weight());
    fe.set_table_id(out_ofp_table_id);

    /*
     * TODO: match sollte das Routingprefix sein, nicht nur die Adresse des
     * Gateways!!!
     */
    fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
    fe.set_match().set_ipv4_dst(rtr.get_ipv4_dst(), rtr.get_ipv4_mask());

    fe.set_cookie(cookie);
    dpt.send_flow_mod_message(rofl::cauxid(0), fe);

    state = STATE_DETACHED;

  } catch (rofl::eRofDptNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_close] unable to find data path"
        << e.what() << std::endl;
  } catch (rofcore::eNetLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_close] unable to find link"
        << e.what() << std::endl;
  } catch (rofcore::crtlink::eRtLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_close] unable to find address"
        << e.what() << std::endl;
  } catch (rofl::eRofSockTxAgain &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_close] control channel congested"
        << e.what() << std::endl;
  } catch (rofl::eRofBaseNotConnected &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_close] control channel is down"
        << e.what() << std::endl;
  } catch (...) {
    rofcore::logging::error
        << "[rofip][cnexthop_in4][handle_dpt_close] unexpected error"
        << std::endl;
  }
}

void cnexthop_in4::handle_packet_in(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_packet_in &msg) {
  rofcore::logging::debug << "[cnexthop_in4][handle_packet_in] pkt received: "
                          << std::endl
                          << msg;
  // store packet in ethcore and thus, tap devices
  roflibs::eth::cethcore::set_eth_core(dpt.get_dptid())
      .handle_packet_in(dpt, auxid, msg);
}

void cnexthop_in6::handle_dpt_open() {
  try {
    rofl::crofdpt &dpt = rofl::crofdpt::get_dpt(dptid);

    rofcore::cnetlink &netlink = rofcore::cnetlink::get_instance();

    // the route ...
    const rofcore::crtroute_in6 &rtr =
        netlink.get_routes_in6(get_rttblid()).get_route(get_rtindex());

    // ... with its destination reachable via this next hop ...
    const rofcore::crtnexthop_in6 &rtn =
        rtr.get_nexthops_in6().get_nexthop(get_nhindex());

    // in the meantime: watch our associated neighbour and track his fate
    rofcore::cnetlink_neighbour_observer::watch(rtn.get_ifindex(),
                                                rtn.get_gateway());

    // ... attached to this link (crtlink) ...
    const rofcore::crtlink &rtl =
        netlink.get_links().get_link(rtn.get_ifindex());

    // ... and the link's dpt representation (cdptlink) needed for OFP related
    // data ...
    const roflibs::ip::clink &dpl =
        cipcore::get_ip_core(dptid).get_link(rtn.get_ifindex());

    // .. and associated to this neighbour
    const rofcore::crtneigh_in6 &rtb =
        dpl.get_neigh_in6(rtn.get_gateway()).get_crtneigh_in6();
    const roflibs::ip::cneigh_in6 &neigh = dpl.get_neigh_in6(rtb.get_dst());

    // local outgoing interface mac address
    const rofl::cmacaddr &eth_src = rtl.get_hwaddr();

    // neighbour mac address
    const rofl::cmacaddr &eth_dst = rtb.get_lladdr();

    // local VLAN associated with interface
    bool tagged = dpl.get_vlan_tagged();
    uint16_t vid = dpl.get_vlan_vid();

    // local outgoing interface => OFP portno
    // uint32_t out_portno 			= dpl.get_ofp_port_no();

    rofl::openflow::cofflowmod fe(dpt.get_version_negotiated());

    switch (state) {
    case STATE_DETACHED: {
      fe.set_command(rofl::openflow::OFPFC_ADD);
    } break;
    case STATE_ATTACHED: {
      fe.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
    } break;
    }

    fe.set_buffer_id(
        rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()));
    fe.set_idle_timeout(0);
    fe.set_hard_timeout(0);
    fe.set_priority(0xf000 + (rtr.get_prefixlen() << 8) + rtn.get_weight());
    fe.set_table_id(out_ofp_table_id);
    fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

    fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
    fe.set_match().set_ipv6_dst(rtr.get_ipv6_dst(), rtr.get_ipv6_mask());
    fe.set_instructions()
        .set_inst_apply_actions()
        .set_actions()
        .add_action_group(rofl::cindex(0))
        .set_group_id(neigh.get_group_id());

    fe.set_cookie(cookie);
    dpt.send_flow_mod_message(rofl::cauxid(0), fe);

    state = STATE_ATTACHED;

  } catch (rofcore::eNetLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_open] unable to find link"
        << e.what() << std::endl;
  } catch (rofcore::crtlink::eRtLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_open] unable to find address"
        << e.what() << std::endl;
  } catch (eNeighNotFound &e) {
    rofcore::logging::debug
        << "[rofip][cnexthop_in6][handle_dpt_open] unable to find dst neighbour"
        << e.what() << std::endl;

    /* non-critical error: just indicating that the kernel has not yet resolved
     * (via ARP or NDP)
     * the neighbour carrying this next-hops's destination address.
     *
     * Strategy:
     * we have subscribed for notifications from cnetlink above (see call to
     * watch() method)
     * and will receive notifications about our associated neighbour
     *
     * see methods neigh_in4_created/updated/deleted for further details
     */

  } catch (rofl::eRofDptNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_open] unable to find data path"
        << e.what() << std::endl;
  } catch (rofl::eRofSockTxAgain &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_open] control channel congested"
        << e.what() << std::endl;
  } catch (rofl::eRofBaseNotConnected &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_open] control channel is down"
        << e.what() << std::endl;
  } catch (std::runtime_error &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_open] runtime error: " << e.what()
        << e.what() << std::endl;
  } catch (...) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_open] unexpected error"
        << std::endl;
  }
}

void cnexthop_in6::handle_dpt_close() {
  try {
    rofl::crofdpt &dpt = rofl::crofdpt::get_dpt(dptid);

    rofcore::cnetlink &netlink = rofcore::cnetlink::get_instance();

    // the route ...
    const rofcore::crtroute_in6 &rtr =
        netlink.get_routes_in6(get_rttblid()).get_route(get_rtindex());

    // ... with its destination reachable via this next hop ...
    const rofcore::crtnexthop_in6 &rtn =
        rtr.get_nexthops_in6().get_nexthop(get_nhindex());

    // ... finally, un-watch our associated neighbour
    rofcore::cnetlink_neighbour_observer::unwatch(rtn.get_ifindex(),
                                                  rtn.get_gateway());

    rofl::openflow::cofflowmod fe(dpt.get_version_negotiated());

    fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
    fe.set_buffer_id(
        rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()));
    fe.set_idle_timeout(0);
    fe.set_hard_timeout(0);
    fe.set_priority(0xf000 + (rtr.get_prefixlen() << 8) + rtn.get_weight());
    fe.set_table_id(out_ofp_table_id);

    fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
    fe.set_match().set_ipv6_dst(rtr.get_ipv6_dst(), rtr.get_ipv6_mask());

    fe.set_cookie(cookie);
    dpt.send_flow_mod_message(rofl::cauxid(0), fe);

    state = STATE_DETACHED;

  } catch (rofl::eRofDptNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_close] unable to find data path"
        << e.what() << std::endl;
  } catch (rofcore::eNetLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_close] unable to find link"
        << e.what() << std::endl;
  } catch (rofcore::crtlink::eRtLinkNotFound &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_close] unable to find address"
        << e.what() << std::endl;
  } catch (rofl::eRofSockTxAgain &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_close] control channel congested"
        << e.what() << std::endl;
  } catch (rofl::eRofBaseNotConnected &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_close] control channel is down"
        << e.what() << std::endl;
  } catch (std::runtime_error &e) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_close] runtime error: " << e.what()
        << e.what() << std::endl;
  } catch (...) {
    rofcore::logging::error
        << "[rofip][cnexthop_in6][handle_dpt_close] unexpected error"
        << std::endl;
  }
}

void cnexthop_in6::handle_packet_in(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_packet_in &msg) {
  rofcore::logging::debug << "[cnexthop_in6][handle_packet_in] pkt received: "
                          << std::endl
                          << msg;
  // store packet in ethcore and thus, tap devices
  roflibs::eth::cethcore::set_eth_core(dpt.get_dptid())
      .handle_packet_in(dpt, auxid, msg);
}
