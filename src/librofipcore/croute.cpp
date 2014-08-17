/*
 * cdptroute.cc
 *
 *  Created on: 02.07.2013
 *      Author: andreas
 */

#include "croute.hpp"

using namespace ipcore;

croute_in4::croute_in4(
		uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid,
		uint8_t fwd_ofp_table_id, uint8_t out_ofp_table_id) :
			croute(rttblid, rtindex, dptid, fwd_ofp_table_id, out_ofp_table_id)
{
	const rofcore::crtroute_in4& rtroute =
			rofcore::cnetlink::get_instance().get_routes_in4(rttblid).get_route(rtindex);
	std::cerr << "UUU:" << rtroute;
	for (std::map<unsigned int, rofcore::crtnexthop_in4>::const_iterator
			it = rtroute.get_nexthops_in4().get_nexthops_in4().begin();
					it != rtroute.get_nexthops_in4().get_nexthops_in4().end(); ++it) {
		unsigned int nhindex = it->first;
		add_nexthop_in4(nhindex) = cnexthop_in4(rttblid, rtindex, nhindex, dptid, out_ofp_table_id);
	}
}



void
croute_in4::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		for (std::map<unsigned int, cnexthop_in4>::iterator
				it = nexthops_in4.begin(); it != nexthops_in4.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, cnexthop_in6>::iterator
				it = nexthops_in6.begin(); it != nexthops_in6.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}

#if 0
		const rofcore::crtroute_in4& rtroute =
				rofcore::cnetlink::get_instance().get_routes_in4(get_rttblid()).get_route(get_rtindex());

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(get_dptid());
		rofl::openflow::cofflowmod fm(dpt.get_version());

		switch (state) {
		case STATE_DETACHED: {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		fm.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000 + (rtroute.get_prefixlen() << 8));
		fm.set_table_id(fwd_ofp_table_id);
		fm.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_dst(rtroute.get_ipv4_dst(), rtroute.get_ipv4_mask());

		fm.set_instructions().add_inst_goto_table().set_table_id(out_ofp_table_id);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);
#endif
		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route create - dpt instance not found: " << e.what() << std::endl;

	} catch (std::out_of_range& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route create - routing table not found: " << e.what() << std::endl;

	} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route create - route index not found: " << e.what() << std::endl;

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route create - OFP channel congested: " << e.what() << std::endl;

	} catch (std::runtime_error& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route create - generic error caught: " << e.what() << std::endl;

	}
}



void
croute_in4::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		for (std::map<unsigned int, cnexthop_in4>::iterator
				it = nexthops_in4.begin(); it != nexthops_in4.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, cnexthop_in6>::iterator
				it = nexthops_in6.begin(); it != nexthops_in6.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}
#if 0
		const rofcore::crtroute_in4& rtroute =
				rofcore::cnetlink::get_instance().get_routes_in4(get_rttblid()).get_route(get_rtindex());

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(get_dptid());
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000 + (rtroute.get_prefixlen() << 8));
		fm.set_table_id(fwd_ofp_table_id);
		fm.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		fm.set_match().set_eth_type(rofl::fipv4frame::IPV4_ETHER);
		fm.set_match().set_ipv4_dst(rtroute.get_ipv4_dst(), rtroute.get_ipv4_mask());

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);
#endif
		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route delete - dpt instance not found: " << e.what() << std::endl;

	} catch (std::out_of_range& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route delete - routing table not found: " << e.what() << std::endl;

	} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route delete - route index not found: " << e.what() << std::endl;

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route delete - OFP channel congested: " << e.what() << std::endl;

	} catch (std::runtime_error& e) {
		rofcore::logging::debug << "[cipcore][route_in4] route delete - generic error caught: " << e.what() << std::endl;

	}
}




croute_in6::croute_in6(
		uint8_t rttblid, unsigned int rtindex, const rofl::cdptid& dptid,
		uint8_t fwd_ofp_table_id, uint8_t out_ofp_table_id) :
			croute(rttblid, rtindex, dptid, fwd_ofp_table_id, out_ofp_table_id)
{
	const rofcore::crtroute_in6& rtroute =
			rofcore::cnetlink::get_instance().get_routes_in6(rttblid).get_route(rtindex);

	for (std::map<unsigned int, rofcore::crtnexthop_in6>::const_iterator
			it = rtroute.get_nexthops_in6().get_nexthops_in6().begin();
					it != rtroute.get_nexthops_in6().get_nexthops_in6().end(); ++it) {
		unsigned int nhindex = it->first;
		add_nexthop_in6(nhindex) = cnexthop_in6(rttblid, rtindex, nhindex, dptid, out_ofp_table_id);
	}
}



void
croute_in6::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		for (std::map<unsigned int, cnexthop_in4>::iterator
				it = nexthops_in4.begin(); it != nexthops_in4.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, cnexthop_in6>::iterator
				it = nexthops_in6.begin(); it != nexthops_in6.end(); ++it) {
			it->second.handle_dpt_open(dpt);
		}
#if 0
		const rofcore::crtroute_in6& rtroute =
				rofcore::cnetlink::get_instance().get_routes_in6(get_rttblid()).get_route(get_rtindex());

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(get_dptid());
		rofl::openflow::cofflowmod fm(dpt.get_version());

		switch (state) {
		case STATE_DETACHED: {
			fm.set_command(rofl::openflow::OFPFC_ADD);
		} break;
		case STATE_ATTACHED: {
			fm.set_command(rofl::openflow::OFPFC_MODIFY_STRICT);
		} break;
		}

		fm.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000 + (rtroute.get_prefixlen() << 8));
		fm.set_table_id(fwd_ofp_table_id);
		fm.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ipv6_dst(rtroute.get_ipv6_dst(), rtroute.get_ipv6_mask());

		fm.set_instructions().add_inst_goto_table().set_table_id(out_ofp_table_id);

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);
#endif
		state = STATE_ATTACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route create - dpt instance not found: " << e.what() << std::endl;

	} catch (std::out_of_range& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route create - routing table not found: " << e.what() << std::endl;

	} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route create - route index not found: " << e.what() << std::endl;

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route create - OFP channel congested: " << e.what() << std::endl;

	} catch (std::runtime_error& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route create - generic error caught: " << e.what() << std::endl;

	}
}



void
croute_in6::handle_dpt_close(rofl::crofdpt& dpt)
{
	try {
		for (std::map<unsigned int, cnexthop_in4>::iterator
				it = nexthops_in4.begin(); it != nexthops_in4.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, cnexthop_in6>::iterator
				it = nexthops_in6.begin(); it != nexthops_in6.end(); ++it) {
			it->second.handle_dpt_close(dpt);
		}
#if 0
		const rofcore::crtroute_in6& rtroute =
				rofcore::cnetlink::get_instance().get_routes_in6(get_rttblid()).get_route(get_rtindex());

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(get_dptid());
		rofl::openflow::cofflowmod fm(dpt.get_version());

		fm.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
		fm.set_buffer_id(rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()));
		fm.set_idle_timeout(0);
		fm.set_hard_timeout(0);
		fm.set_priority(0x8000 + (rtroute.get_prefixlen() << 8));
		fm.set_table_id(fwd_ofp_table_id);
		fm.set_flags(rofl::openflow13::OFPFF_SEND_FLOW_REM);

		fm.set_match().set_eth_type(rofl::fipv6frame::IPV6_ETHER);
		fm.set_match().set_ipv6_dst(rtroute.get_ipv6_dst(), rtroute.get_ipv6_mask());

		dpt.send_flow_mod_message(rofl::cauxid(0), fm);
#endif
		state = STATE_DETACHED;

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route delete - dpt instance not found: " << e.what() << std::endl;

	} catch (std::out_of_range& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route delete - routing table not found: " << e.what() << std::endl;

	} catch (rofcore::crtroute::eRtRouteNotFound& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route delete - route index not found: " << e.what() << std::endl;

	} catch (rofl::eRofSockTxAgain& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route delete - OFP channel congested: " << e.what() << std::endl;

	} catch (std::runtime_error& e) {
		rofcore::logging::debug << "[cipcore][route_in6] route delete - generic error caught: " << e.what() << std::endl;

	}
}



