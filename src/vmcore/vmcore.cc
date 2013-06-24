#include "vmcore.h"

using namespace dptmap;

vmcore::vmcore()
{

}


vmcore::~vmcore()
{
	for (std::map<rofl::cofdpt*, std::map<std::string, ctapdev*> >::iterator
			jt = tapdevs.begin(); jt != tapdevs.end(); ++jt) {
		for (std::map<std::string, ctapdev*>::iterator
				it = tapdevs[jt->first].begin(); it != tapdevs[jt->first].end(); ++it) {
			delete (it->second); // remove ctapdev instance from heap
		}
	}
	tapdevs.clear();

}



void
vmcore::handle_dpath_open(
		rofl::cofdpt *dpt)
{
	try {
		// TODO: how many data path elements are allowed to connect to ourselves? only one makes sense ...

		std::map<uint32_t, rofl::cofport*> ports = dpt->get_ports();
		for (std::map<uint32_t, rofl::cofport*>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			rofl::cofport *port = it->second;
			if (tapdevs[dpt].find(port->get_name()) == tapdevs[dpt].end()) {
				tapdevs[dpt][port->get_name()] = new ctapdev(this, port->get_name());
			}
		}

	} catch (eNetDevCritical& e) {

		fprintf(stderr, "vmcore::handle_dpath_open() unable to create tap device\n");
		throw;

	}
}



void
vmcore::handle_dpath_close(
		rofl::cofdpt *dpt)
{
	for (std::map<std::string, ctapdev*>::iterator
			it = tapdevs[dpt].begin(); it != tapdevs[dpt].end(); ++it) {
		delete (it->second); // remove ctapdev instance from heap
	}
	tapdevs.erase(dpt);
}



void
vmcore::handle_port_status(
		rofl::cofdpt *dpt,
		rofl::cofmsg_port_status *msg)
{
	try {
		switch (msg->get_reason()) {
		case OFPPR_ADD: {
			if (tapdevs[dpt].find(msg->get_port().get_name()) == tapdevs[dpt].end()) {
				tapdevs[dpt][msg->get_port().get_name()] = new ctapdev(this, msg->get_port().get_name());
			}
		} break;
		case OFPPR_MODIFY: {

			// TODO: mirror port status on ctapdev instance

		} break;
		case OFPPR_DELETE: {
			if (tapdevs[dpt].find(msg->get_port().get_name()) != tapdevs[dpt].end()) {
				delete tapdevs[dpt][msg->get_port().get_name()];
				tapdevs[dpt].erase(msg->get_port().get_name());
			}
		} break;
		default: {

			fprintf(stderr, "vmcore::handle_port_status() message with invalid reason code received, ignoring\n");

		} break;
		}

	} catch (eNetDevCritical& e) {

		// TODO: handle error condition appropriately, for now: rethrow
		throw;

	}

	delete msg;
}



#if 0
void
vmcore::nl_route_new(cnetlink const* netlink, croute const& route)
{
	croute rt(route);

	uint32_t port_no = 0;  // TODO: get mapping from ifindex in VM to port_no in data path

	switch (rt.rt_type) {
	/* a LOCAL route entry generates a FlowMod that redirects all packets
	 * received at the data path towards the control plane, as it defines
	 * an address assignment to a local interface
	 */
	case croute::RT_LOCAL: {
		fprintf(stderr, "vmcore::nl_route_new() %s\n", rt.c_str());

		uint8_t of_version = (dpt) ? dpt->get_version() : OFP12_VERSION; // for testing purpose
#if 0
		rofl::cflowentry fe(dpt->get_version());
#endif
		rofl::cflowentry fe(of_version);

		fe.set_command(OFPFC_ADD);
		fe.set_buffer_id(OFP_NO_BUFFER);
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_table_id(0);

		fe.match.set_in_port(port_no);

		switch (rt.rt_family) {
		case AF_INET: {
			fe.match.set_ipv4_dst(rt.dst);
		} break;
		case AF_INET6: {
			fe.match.set_ipv6_dst(rt.dst);
		} break;
		default: {
			fprintf(stderr, "route family %d not implemented\n", rt.rt_family);
		} return;
		}

		fe.instructions.next() = rofl::cofinst_apply_actions();
		fe.instructions[0].actions.next() = rofl::cofaction_output(OFPP_CONTROLLER, rt.mtu);

		fprintf(stderr, "vmcore::nl_route_new() FlowMod: %s", fe.c_str());

		if (dpt)
			send_flow_mod_message(dpt, fe);
	} break;
	}
}


void
vmcore::nl_route_del(cnetlink const* netlink, croute const& route)
{
	croute rt(route);

	uint32_t port_no = 0;  // TODO: get mapping from ifindex in VM to port_no in data path

	switch (rt.rt_type) {
	/* a LOCAL route entry generates a FlowMod that redirects all packets
	 * received at the data path towards the control plane, as it defines
	 * an address assignment to a local interface
	 */
	case croute::RT_LOCAL: {
		fprintf(stderr, "vmcore::nl_route_del() %s\n", rt.c_str());

		uint8_t of_version = (dpt) ? dpt->get_version() : OFP12_VERSION; // for testing purpose
#if 0
		rofl::cflowentry fe(dpt->get_version());
#endif
		rofl::cflowentry fe(of_version);

		fe.set_command(OFPFC_ADD);
		fe.set_buffer_id(OFP_NO_BUFFER);
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_table_id(0);

		fe.match.set_in_port(port_no);

		switch (rt.rt_family) {
		case AF_INET: {
			fe.match.set_ipv4_dst(rt.dst);
		} break;
		case AF_INET6: {
			fe.match.set_ipv6_dst(rt.dst);
		} break;
		default: {
			fprintf(stderr, "route family %d not implemented\n", rt.rt_family);
		} return;
		}

		fe.instructions.next() = rofl::cofinst_apply_actions();
		fe.instructions[0].actions.next() = rofl::cofaction_output(OFPP_CONTROLLER, rt.mtu);

		fprintf(stderr, "vmcore::nl_route_del() FlowMod: %s", fe.c_str());

		if (dpt)
			send_flow_mod_message(dpt, fe);
	} break;
	}
}


void
vmcore::nl_route_change(cnetlink const* netlink, croute const& route)
{
	croute rt(route);

	uint32_t port_no = 0;  // TODO: get mapping from ifindex in VM to port_no in data path

	switch (rt.rt_type) {
	/* a LOCAL route entry generates a FlowMod that redirects all packets
	 * received at the data path towards the control plane, as it defines
	 * an address assignment to a local interface
	 */
	case croute::RT_LOCAL: {
		fprintf(stderr, "vmcore::nl_route_change() %s\n", rt.c_str());

		uint8_t of_version = (dpt) ? dpt->get_version() : OFP12_VERSION; // for testing purpose
#if 0
		rofl::cflowentry fe(dpt->get_version());
#endif
		rofl::cflowentry fe(of_version);

		fe.set_command(OFPFC_ADD);
		fe.set_buffer_id(OFP_NO_BUFFER);
		fe.set_idle_timeout(0);
		fe.set_hard_timeout(0);
		fe.set_table_id(0);

		fe.match.set_in_port(port_no);

		switch (rt.rt_family) {
		case AF_INET: {
			fe.match.set_ipv4_dst(rt.dst);
		} break;
		case AF_INET6: {
			fe.match.set_ipv6_dst(rt.dst);
		} break;
		default: {
			fprintf(stderr, "route family %d not implemented\n", rt.rt_family);
		} return;
		}

		fe.instructions.next() = rofl::cofinst_apply_actions();
		fe.instructions[0].actions.next() = rofl::cofaction_output(OFPP_CONTROLLER, rt.mtu);

		fprintf(stderr, "vmcore::nl_route_change() FlowMod: %s", fe.c_str());

		if (dpt)
			send_flow_mod_message(dpt, fe);
	} break;
	}
}
#endif

