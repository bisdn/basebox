/*
 * dptnexthop.cc
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#include "cdptnexthop.h"
#include "cipcore.h"

using namespace ipcore;




void
cdptnexthop_in4::update()
{
	flow_mod_add(rofl::openflow::OFPFC_MODIFY_STRICT);
}



void
cdptnexthop_in4::flow_mod_add(uint8_t command)
{
	try {
		rofcore::cnetlink& netlink = rofcore::cnetlink::get_instance();
		// the route ...
		const rofcore::crtroute_in4& rtr =
				netlink.get_routes_in4(rttblid).get_route(rtindex);
		// ... with its destination reachable via this next hop ...
		const rofcore::crtnexthop_in4& rtn =
				rtr.get_nexthops_in4().get_nexthop(nhindex);
		// ... attached to this link
		const rofcore::crtlink& rtl =
				netlink.get_links().get_link(rtn.get_ifindex());


		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(command);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(rttblid);
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rtr.get_ipv4_dst(), rtr.get_ipv4_mask());


		const rofl::cmacaddr& eth_src 	= rtl.get_hwaddr();
		const rofl::cmacaddr& eth_dst 	= rtl.get_neighs_in4().get_neigh(nhindex).get_lladdr();
		const std::string& devname 		= rtl.get_devname();
		uint32_t out_portno 			= cipcore::get_instance().get_link_table().
											get_link_by_ifindex(rtl.get_neighs_in4().get_neigh(nhindex).get_ifindex()).
													get_ofp_port_no();

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().
				add_action_set_field(index++).set_oxm(rofl::openflow::coxmatch_ofb_eth_src(eth_src));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_set_field(index++).set_oxm(rofl::openflow::coxmatch_ofb_eth_dst(eth_dst));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(index++).set_port_no(out_portno);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptnexthop_in4::flow_mod_delete()
{
	try {
		const rofcore::crtroute_in4& rtr =
				rofcore::cnetlink::get_instance().
						get_routes_in4(rttblid).get_route(rtindex);

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(rttblid);

		/*
		 * TODO: match sollte das Routingprefix sein, nicht nur die Adresse des Gateways!!!
		 */
		fe.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fe.set_match().set_ipv4_dst(rtr.get_ipv4_dst(), rtr.get_ipv4_mask());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in4][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}




void
cdptnexthop_in6::update()
{
	flow_mod_add(rofl::openflow::OFPFC_MODIFY_STRICT);
}



void
cdptnexthop_in6::flow_mod_add(uint8_t command)
{
	try {
		rofcore::cnetlink& netlink = rofcore::cnetlink::get_instance();
		// the route ...
		const rofcore::crtroute_in6& rtr =
				netlink.get_routes_in6(rttblid).get_route(rtindex);
		// ... with its destination reachable via this next hop ...
		const rofcore::crtnexthop_in6& rtn =
				rtr.get_nexthops_in6().get_nexthop(nhindex);
		// ... attached to this link
		const rofcore::crtlink& rtl =
				netlink.get_links().get_link(rtn.get_ifindex());


		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(command);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(rttblid);
		fe.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rtr.get_ipv6_dst(), rtr.get_ipv6_mask());

		const rofl::cmacaddr& eth_src 	= rtl.get_hwaddr();
		const rofl::cmacaddr& eth_dst 	= rtl.get_neighs_in6().get_neigh(nhindex).get_lladdr();
		const std::string& devname 		= rtl.get_devname();
		uint32_t out_portno 			= cipcore::get_instance().get_link_table().
											get_link_by_ifindex(rtl.get_neighs_in6().get_neigh(nhindex).get_ifindex()).
													get_ofp_port_no();

		rofl::cindex index(0);

		fe.set_instructions().add_inst_apply_actions().set_actions().
				add_action_set_field(index++).set_oxm(rofl::openflow::coxmatch_ofb_eth_src(eth_src));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_set_field(index++).set_oxm(rofl::openflow::coxmatch_ofb_eth_dst(eth_dst));
		fe.set_instructions().set_inst_apply_actions().set_actions().
				add_action_output(index++).set_port_no(out_portno);

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_add] unexpected error" << std::endl << *this;

	}
}



void
cdptnexthop_in6::flow_mod_delete()
{
	try {
		const rofcore::crtroute_in6& rtr =
				rofcore::cnetlink::get_instance().
						get_routes_in6(rttblid).get_route(rtindex);

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		rofl::openflow::cofflowmod fe(dpt.get_version());

		fe.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fe.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_priority(0xfffe);
		fe.set_table_id(rttblid);

		fe.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fe.set_match().set_ipv6_dst(rtr.get_ipv6_dst(), rtr.get_ipv6_mask());

		dpt.send_flow_mod_message(rofl::cauxid(0), fe);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unable to find data path" << std::endl << *this;

	} catch (rofcore::eNetLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unable to find link" << std::endl << *this;

	} catch (rofcore::crtlink::eRtLinkNotFound& e) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unable to find address" << std::endl << *this;

	} catch (...) {
		rofcore::logging::error << "[dptnexthop_in6][flow_mod_delete] unexpected error" << std::endl << *this;

	}
}




