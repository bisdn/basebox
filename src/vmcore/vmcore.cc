#include "vmcore.h"


vmcore::vmcore() :
	dpt(0)
{

}


vmcore::~vmcore()
{

}


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


