/*
 * cdptroute.cc
 *
 *  Created on: 02.07.2013
 *      Author: andreas
 */

#include "croute.hpp"

using namespace rofip;

croute_in4::croute_in4(
		uint8_t rttblid, unsigned int rtindex, const rofl::cdpid& dpid,
		uint8_t out_ofp_table_id) :
			croute(rttblid, rtindex, dpid, out_ofp_table_id)
{
	const rofcore::crtroute_in4& rtroute =
			rofcore::cnetlink::get_instance().get_routes_in4(rttblid).get_route(rtindex);

	for (std::map<unsigned int, rofcore::crtnexthop_in4>::const_iterator
			it = rtroute.get_nexthops_in4().get_nexthops_in4().begin();
					it != rtroute.get_nexthops_in4().get_nexthops_in4().end(); ++it) {
		unsigned int nhindex = it->first;
		add_nexthop_in4(nhindex) = cnexthop_in4(rttblid, rtindex, nhindex, dpid, out_ofp_table_id);
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
		uint8_t rttblid, unsigned int rtindex, const rofl::cdpid& dpid,
		uint8_t out_ofp_table_id) :
			croute(rttblid, rtindex, dpid, out_ofp_table_id)
{
	const rofcore::crtroute_in6& rtroute =
			rofcore::cnetlink::get_instance().get_routes_in6(rttblid).get_route(rtindex);

	for (std::map<unsigned int, rofcore::crtnexthop_in6>::const_iterator
			it = rtroute.get_nexthops_in6().get_nexthops_in6().begin();
					it != rtroute.get_nexthops_in6().get_nexthops_in6().end(); ++it) {
		unsigned int nhindex = it->first;
		add_nexthop_in6(nhindex) = cnexthop_in6(rttblid, rtindex, nhindex, dpid, out_ofp_table_id);
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



