/*
 * dptroute.cc
 *
 *  Created on: 02.07.2013
 *      Author: andreas
 */

#include <dptroute.h>


using namespace dptmap;


dptroute::dptroute(
		rofl::crofbase* rofbase,
		rofl::cofdpt* dpt,
		uint8_t table_id,
		unsigned int rtindex) :
		rofbase(rofbase),
		dpt(dpt),
		table_id(table_id),
		rtindex(rtindex),
		flowentry(OFP12_VERSION)
{
	route_created(table_id, rtindex);
}



dptroute::~dptroute()
{
	delete_all_nexthops();

	route_deleted(table_id, rtindex);
}




void
dptroute::set_nexthops()
{
	std::cerr << "dptroute::set_nexthops()" << std::endl;

	crtroute& rtr = cnetlink::get_instance().get_route(table_id, rtindex);

	std::vector<crtnexthop>::iterator it;
	for (it = rtr.get_nexthops().begin(); it != rtr.get_nexthops().end(); ++it) {
		crtnexthop& rtnh = (*it);
		try {

			crtlink& rtl = cnetlink::get_instance().get_link(rtnh.get_ifindex());

			crtneigh& rtn = rtl.get_neigh(rtnh.get_gateway());

			uint16_t nbindex = rtl.get_neigh_index(rtn);

			// dptnexthop instance already active? yes => continue with next next-hop
			if (dptnexthops.find(nbindex) != dptnexthops.end())
				continue;

			// no => create new dptnexthop instance
			dptnexthops[nbindex] = dptnexthop(
										rofbase,
										dpt,
										dptlink::get_dptlink(rtnh.get_ifindex()).get_of_port_no(),
										/*of_table_id=*/2,
										rtnh.get_ifindex(),
										nbindex,
										rtr.get_dst(),
										rtr.get_mask());

			// and activate its FlowMod (installs FlowMod on data path element)
			dptnexthops[nbindex].flow_mod_add();

			std::cerr << "dptroute::set_nexthops() => " << dptnexthops[nbindex] << std::endl;

		} catch (eNetLinkNotFound& e) {
			// oops, link assigned to next hop not found, skip this one for now
			continue;
		} catch (eRtLinkNotFound& e){
			// no valid neighbour entry found for gateway specified in next hop, skip this one for now
			continue;
		} catch (eDptLinkNotFound& e) {
			// no dptlink instance found for interface referenced by ifindex
			continue;
		}
	}
}



void
dptroute::delete_all_nexthops()
{
	std::map<uint16_t, dptnexthop>::iterator it;
	for (it = dptnexthops.begin(); it != dptnexthops.end(); ++it) {
		it->second.flow_mod_delete();
	}
	dptnexthops.clear();
}



void
dptroute::addr_deleted(
			unsigned int ifindex,
			uint16_t adindex)
{
	std::cerr << "dptroute::addr_deleted() => ifindex=" << ifindex << std::endl;

	crtaddr& rta = cnetlink::get_instance().get_link(ifindex).get_addr(adindex);

	rofl::caddress addr = rta.get_local_addr();
	rofl::caddress mask = rta.get_mask();

	std::map<uint16_t, dptnexthop>::iterator it;
restart:
	for (it = dptnexthops.begin(); it != dptnexthops.end(); ++it) {
		dptnexthop& nhop = it->second;
		if ((nhop.get_ifindex() == ifindex) &&
				((nhop.get_gateway() & rta.get_mask()) == (rta.get_local_addr() & rta.get_mask()))) {
			it->second.flow_mod_delete();
			dptnexthops.erase(it->first);
			goto restart;
		}
	}
	if (dptnexthops.empty()) {
		std::cerr << "dptroute::addr_deleted() => ifindex=" << ifindex
				<< "all next hops lost, deleting route" << std::endl;
		route_deleted(table_id, rtindex);
	}
}



void
dptroute::route_created(
		uint8_t table_id,
		unsigned int rtindex)
{
	try {
		if ((this->table_id != table_id) || (this->rtindex != rtindex))
			return;

#if 1
		std::cerr << "dptroute::route_created() " << cnetlink::get_instance().get_route(table_id, rtindex) << std::endl;
#endif

		crtroute& rtr = cnetlink::get_instance().get_route(table_id, rtindex);

		set_nexthops();

		flowentry = rofl::cflowentry(dpt->get_version());

		flowentry.set_command(OFPFC_ADD);
		flowentry.set_buffer_id(OFP_NO_BUFFER);
		flowentry.set_idle_timeout(0);
		flowentry.set_hard_timeout(0);
		flowentry.set_priority(0x8000 + (rtr.get_prefixlen() << 8));
		flowentry.set_table_id(1);			// FIXME: check for first table-id in data path

		switch (rtr.get_family()) {
		case AF_INET: {
			flowentry.match.set_ipv4_dst(rtr.get_dst(), rtr.get_mask());
		} break;
		case AF_INET6: {
			flowentry.match.set_ipv6_dst(rtr.get_dst(), rtr.get_mask());
		} break;
		}

		flowentry.instructions.next() = rofl::cofinst_goto_table(2);

		fprintf(stderr, "dptroute::route_created() => flowentry: %s\n", flowentry.c_str());

		rofbase->send_flow_mod_message(dpt, flowentry);

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptroute::route_created() crtroute object not found" << std::endl;

	} catch (eRtRouteNotFound& e) {
		std::cerr << "dptroute::route_created() crtnexthop object not found" << std::endl;

	}
}



void
dptroute::route_updated(
		uint8_t table_id,
		unsigned int rtindex)
{
	if ((this->table_id != table_id) || (this->rtindex != rtindex))
		return;

#if 1
	fprintf(stderr, "dptroute::route_updated() rtindex=%d => ", rtindex);
	std::cerr << cnetlink::get_instance().get_route(table_id, rtindex) << std::endl;
#endif
}



void
dptroute::route_deleted(
		uint8_t table_id,
		unsigned int rtindex)
{
	if ((this->table_id != table_id) || (this->rtindex != rtindex))
		return;

#if 1
	fprintf(stderr, "dptroute::route_deleted() table_id=%d rtindex=%d\n", table_id, rtindex);
#endif

	flowentry.set_command(OFPFC_DELETE_STRICT);

	rofbase->send_flow_mod_message(dpt, flowentry);

	fprintf(stderr, "\n\n\n FLOWENTRY => %s\n\n\n\n", flowentry.c_str());

	delete_all_nexthops();
}



void
dptroute::neigh_created(unsigned int ifindex, uint16_t nbindex)
{
	//std::cerr << "dptroute::neigh_created() => ifindex=" << ifindex
	//		<< " nbindex=" << (unsigned int)nbindex << std::endl;

	crtroute& rtr = cnetlink::get_instance().get_route(table_id, rtindex);
	crtneigh& rtn = cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

	std::cerr << "dptroute::neigh_created() => " << rtn << std::endl;

	// do for all next hops defined in crtroute ...
	std::vector<crtnexthop>::iterator it;
	for (it = rtr.get_nexthops().begin(); it != rtr.get_nexthops().end(); ++it) {
		crtnexthop& rtnh = (*it);

		// nexthop and neighbour must be bound to same link
		if (rtnh.get_ifindex() != rtn.get_ifindex()) {
			fprintf(stderr, "rtnh.get_ifindex(%d) != rtn.get_ifindex(%d)\n",
					rtnh.get_ifindex(), rtn.get_ifindex());
			continue;
		}

		// gateway in nexthop must match dst address in neighbor instance
		if (rtnh.get_gateway() != rtn.get_dst()) {
			std::cerr << "rtnh.get_gateway(" << rtnh.get_gateway() << ")!="
					<< "rtn.get_dst(" << rtn.get_dst() << ")" << std::endl;
			continue;
		}

		// state of neighbor instance must be REACHABLE, PERMANENT, or NOARP
		switch (rtn.get_state()) {
		case NUD_INCOMPLETE: 	{ fprintf(stderr, "NUD_INCOMPLETE\n"); } 	continue;
		case NUD_DELAY: 		{ fprintf(stderr, "NUD_DELAY\n"); } 		continue;
		case NUD_PROBE: 		{ fprintf(stderr, "NUD_PROBE\n"); } 		continue;
		case NUD_FAILED: 		{ fprintf(stderr, "NUD_FAILED\n"); } 		continue;

		case NUD_STALE:
		case NUD_NOARP:
		case NUD_REACHABLE:
		case NUD_PERMANENT: {
			/* go on and create entry */
		} break;
		}

		// lookup the dptlink instance mapped to crtlink
		try {
			if (dptnexthops.find(nbindex) != dptnexthops.end()) {
				dptnexthops[nbindex].flow_mod_delete();
			}
			dptnexthops[nbindex] = dptnexthop(
										rofbase,
										dpt,
										dptlink::get_dptlink(rtnh.get_ifindex()).get_of_port_no(),
										/*of_table_id=*/2,
										ifindex,
										nbindex,
										rtr.get_dst(),
										rtr.get_mask());
			dptnexthops[nbindex].flow_mod_add();

			std::cerr << "dptroute::neigh_created() => " << dptnexthops[nbindex] << std::endl;

		} catch (eDptLinkNotFound& e) {

		}
	}
}



void
dptroute::neigh_updated(unsigned int ifindex, uint16_t nbindex)
{
	try {
		dptlink::get_dptlink(ifindex);

		crtroute& rtr = cnetlink::get_instance().get_route(table_id, rtindex);
		crtneigh& rtn = cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

		std::cerr << "dptroute::neigh_updated() => " << rtn << std::endl;

		// TODO: check status update and notify dptnexthop instance

		// do for all next hops defined in crtroute ...
		std::vector<crtnexthop>::iterator it;
		for (it = rtr.get_nexthops().begin(); it != rtr.get_nexthops().end(); ++it) {
			crtnexthop& rtnh = (*it);

			// nexthop and neighbour must be bound to same link
			if (rtnh.get_ifindex() != rtn.get_ifindex()) {
				fprintf(stderr, "rtnh.get_ifindex(%d) != rtn.get_ifindex(%d)\n",
						rtnh.get_ifindex(), rtn.get_ifindex());
				continue;
			}

			// gateway in nexthop must match dst address in neighbor instance
			if (rtnh.get_gateway() != rtn.get_dst()) {
				std::cerr << "rtnh.get_gateway(" << rtnh.get_gateway() << ")!="
							<< "rtn.get_dst(" << rtn.get_dst() << ")" << std::endl;
				continue;
			}

			// state of neighbor instance must be REACHABLE, PERMANENT, or NOARP
			switch (rtn.get_state()) {
			case NUD_INCOMPLETE:
			case NUD_DELAY:
			case NUD_PROBE:
			case NUD_FAILED: {
				/* go on and delete entry */
				if (dptnexthops.find(nbindex) == dptnexthops.end())
					continue;
				std::cerr << "dptroute::neigh_updated() DELETE => " << dptnexthops[nbindex] << std::endl;
				dptnexthops[nbindex].flow_mod_delete();
				dptnexthops.erase(nbindex);
			} break;

			case NUD_STALE:
			case NUD_NOARP:
			case NUD_REACHABLE:
			case NUD_PERMANENT: {
				if (dptnexthops.find(nbindex) != dptnexthops.end()) {
					dptnexthops[nbindex].flow_mod_delete();
					dptnexthops.erase(nbindex);
				}

				std::cerr << "dptroute::neigh_updated() ADD => " << dptnexthops[nbindex] << std::endl;
				dptnexthops[nbindex] = dptnexthop(
											rofbase,
											dpt,
											dptlink::get_dptlink(rtnh.get_ifindex()).get_of_port_no(),
											/*of_table_id=*/2,
											ifindex,
											nbindex,
											rtr.get_dst(),
											rtr.get_mask());
				dptnexthops[nbindex].flow_mod_add();

			} break;
			}
		}

	} catch (eDptLinkNotFound& e) {
		// this happens, when an update for a non-data path interface is received
	}
}



void
dptroute::neigh_deleted(unsigned int ifindex, uint16_t nbindex)
{
	std::cerr << "dptroute::neigh_deleted() => ifindex=" << ifindex
			<< " nbindex=" << (unsigned int)nbindex << std::endl;

	crtroute& rtr = cnetlink::get_instance().get_route(table_id, rtindex);
	crtneigh& rtn = cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

	// do for all next hops defined in crtroute ...
	std::vector<crtnexthop>::iterator it;
	for (it = rtr.get_nexthops().begin(); it != rtr.get_nexthops().end(); ++it) {
		crtnexthop& rtnh = (*it);

		// nexthop and neighbour must be bound to same link
		if (rtnh.get_ifindex() != rtn.get_ifindex())
			continue;

		// gateway in nexthop must match dst address in neighbor instance
		if (rtnh.get_gateway() != rtn.get_dst())
			continue;

		// do we have to check here rtn.get_state() once again?

		if (dptnexthops.find(nbindex) == dptnexthops.end())
			continue;

		std::cerr << "dptroute::neigh_deleted() => " << dptnexthops[nbindex] << std::endl;

		dptnexthops[nbindex].flow_mod_delete();
		dptnexthops.erase(nbindex);
	}
}






