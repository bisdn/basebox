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
	route_deleted(table_id, rtindex);
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

		flowentry = rofl::cflowentry(dpt->get_version());

		flowentry.set_command(OFPFC_ADD);
		flowentry.set_buffer_id(OFP_NO_BUFFER);
		flowentry.set_idle_timeout(0);
		flowentry.set_hard_timeout(0);
		flowentry.set_priority(0x8000 + (rtr.get_prefixlen() << 8));
		flowentry.set_table_id(0);			// FIXME: check for first table-id in data path

		switch (rtr.get_family()) {
		case AF_INET: {
			flowentry.match.set_ipv4_dst(rtr.get_dst(), rtr.get_mask());
		} break;
		case AF_INET6: {
			flowentry.match.set_ipv6_dst(rtr.get_dst(), rtr.get_mask());
		} break;
		}

		//rtr.get_nexthop(0).get_gateway().get

		rofl::cmacaddr eth_src = cnetlink::get_instance().get_link(rtr.get_nexthop(0).get_ifindex()).get_hwaddr();


		std::map<uint32_t, rofl::cofport*>::iterator it;
		if ((it = find_if(dpt->get_ports().begin(), dpt->get_ports().end(),
				rofl::cofport_find_by_maddr(eth_src))) == dpt->get_ports().end()) {

			// TODO: dump error

			return;
		}
		uint32_t port_no = it->second->get_port_no();
		rofl::cmacaddr eth_dst("00:00:00:00:00:00");

		flowentry.instructions.next() = rofl::cofinst_write_actions();
		flowentry.instructions.back().actions.next() = rofl::cofaction_set_field(rofl::coxmatch_ofb_eth_src(eth_src));
		flowentry.instructions.back().actions.next() = rofl::cofaction_set_field(rofl::coxmatch_ofb_eth_dst(eth_dst));
		flowentry.instructions.back().actions.next() = rofl::cofaction_output(port_no);

		fprintf(stderr, "flowentry: %s\n", flowentry.c_str());

		rofbase->send_flow_mod_message(dpt, flowentry);

	} catch (eNetLinkNotFound& e) {

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

	fprintf(stderr, "flowentry: %s\n", flowentry.c_str());

	rofbase->send_flow_mod_message(dpt, flowentry);
}



