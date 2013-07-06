#include "vmcore.h"

using namespace dptmap;

vmcore::vmcore() :
		dpt(0)
{

}


vmcore::~vmcore()
{
	if (0 != dpt) {
		delete_all_routes();

		delete_all_ports();
	}
}



bool
vmcore::link_is_mapped_from_dpt(int ifindex)
{
	std::map<uint32_t, dptlink*>::const_iterator it;
	for (it = dptlinks[dpt].begin(); it != dptlinks[dpt].end(); ++it) {
		if (it->second->get_ifindex() == ifindex) {
			return true;
		}
	}
	return false;
}



dptlink&
vmcore::get_mapped_link_from_dpt(int ifindex)
{
	std::map<uint32_t, dptlink*>::const_iterator it;
	for (it = dptlinks[dpt].begin(); it != dptlinks[dpt].end(); ++it) {
		if (it->second->get_ifindex() == ifindex) {
			return *(it->second);
		}
	}
	throw eVmCoreNotFound();
}



void
vmcore::delete_all_ports()
{
	for (std::map<rofl::cofdpt*, std::map<uint32_t, dptlink*> >::iterator
			jt = dptlinks.begin(); jt != dptlinks.end(); ++jt) {
		for (std::map<uint32_t, dptlink*>::iterator
				it = dptlinks[jt->first].begin(); it != dptlinks[jt->first].end(); ++it) {
			delete (it->second); // remove dptport instance from heap
		}
	}
	dptlinks.clear();
}



void
vmcore::delete_all_routes()
{
	for (std::map<uint8_t, std::map<unsigned int, dptroute*> >::iterator
			it = dptroutes.begin(); it != dptroutes.end(); ++it) {
		for (std::map<unsigned int, dptroute*>::iterator
				jt = it->second.begin(); jt != it->second.end(); ++jt) {
			delete (jt->second); // remove dptroute instance from heap
		}
	}
	dptroutes.clear();
}



void
vmcore::handle_dpath_open(
		rofl::cofdpt *dpt)
{
	try {
		this->dpt = dpt;

		/*
		 * remove all routes
		 */
		delete_all_routes();

		/*
		 * remove all old pending tap devices
		 */
		delete_all_ports();

		/*
		 * purge all entries from data path
		 */
		rofl::cflowentry fe(dpt->get_version());
		fe.set_command(OFPFC_DELETE);
		fe.set_table_id(OFPTT_ALL);
		send_flow_mod_message(dpt, fe);

		// TODO: how many data path elements are allowed to connect to ourselves? only one makes sense ...


		/*
		 * create new tap devices for (reconnecting?) data path
		 */
		std::map<uint32_t, rofl::cofport*> ports = dpt->get_ports();
		for (std::map<uint32_t, rofl::cofport*>::iterator
				it = ports.begin(); it != ports.end(); ++it) {
			rofl::cofport *port = it->second;
			if (dptlinks[dpt].find(port->get_port_no()) == dptlinks[dpt].end()) {
				dptlinks[dpt][port->get_port_no()] = new dptlink(this, dpt, port->get_port_no());
			}
		}

		// get full-length packets (what is the ethernet max length on dpt?)
		send_set_config_message(dpt, 0, 1518);

#if 0
		/*
		 * dump dpt tables for debugging
		 */
		for (std::map<uint8_t, rofl::coftable_stats_reply>::iterator
				it = dpt->get_tables().begin(); it != dpt->get_tables().end(); ++it) {
			std::cout << it->second << std::endl;
		}
#endif

		/*
		 * install default FlowMod entry for table 0 => GotoTable(1)
		 */
		rofl::cflowentry fed = rofl::cflowentry(dpt->get_version());

		fed.set_command(OFPFC_ADD);
		fed.set_table_id(0);
		fed.set_idle_timeout(0);
		fed.set_hard_timeout(0);
		fed.set_priority(0); // lowest priority
		fed.instructions.next() = rofl::cofinst_goto_table(1);

		send_flow_mod_message(dpt, fed);

	} catch (eNetDevCritical& e) {

		fprintf(stderr, "vmcore::handle_dpath_open() unable to create tap device\n");
		throw;

	}
}



void
vmcore::handle_dpath_close(
		rofl::cofdpt *dpt)
{
	delete_all_routes();

	delete_all_ports();

	this->dpt = (rofl::cofdpt*)0;
}



void
vmcore::handle_port_status(
		rofl::cofdpt *dpt,
		rofl::cofmsg_port_status *msg)
{
	if (this->dpt != dpt) {
		fprintf(stderr, "vmcore::handle_port_stats() received PortStatus from invalid data path\n");
		delete msg; return;
	}

	uint32_t port_no = msg->get_port().get_port_no();

	try {
		switch (msg->get_reason()) {
		case OFPPR_ADD: {
			if (dptlinks[dpt].find(port_no) == dptlinks[dpt].end()) {
				dptlinks[dpt][port_no] = new dptlink(this, dpt, msg->get_port().get_port_no());
			}
		} break;
		case OFPPR_MODIFY: {
			if (dptlinks[dpt].find(port_no) != dptlinks[dpt].end()) {
				dptlinks[dpt][port_no]->handle_port_status();
			}
		} break;
		case OFPPR_DELETE: {
			if (dptlinks[dpt].find(port_no) != dptlinks[dpt].end()) {
				delete dptlinks[dpt][port_no];
				dptlinks[dpt].erase(port_no);
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



void
vmcore::handle_packet_out(rofl::cofctl *ctl, rofl::cofmsg_packet_out *msg)
{
	delete msg;
}




void
vmcore::handle_packet_in(rofl::cofdpt *dpt, rofl::cofmsg_packet_in *msg)
{
	try {
		uint32_t port_no = msg->get_match().get_in_port();

		if (dptlinks[dpt].find(port_no) == dptlinks[dpt].end()) {
			fprintf(stderr, "vmcore::handle_packet_in() frame for port_no=%d received, but port not found\n", port_no);

			delete msg; return;
		}

		dptlinks[dpt][port_no]->handle_packet_in(msg->get_packet());

		delete msg;

	} catch (ePacketPoolExhausted& e) {

		fprintf(stderr, "vmcore::handle_packet_in() packetpool exhausted\n");

	}
}



void
vmcore::route_created(uint8_t table_id, unsigned int rtindex)
{
	// ignore local route table and unspecified table_id
	if ((255 == table_id) || (0 == table_id)) {
		//std::cerr << "vmcore::route_created() => suppressing table_id=" << (unsigned int)table_id << std::endl;
		return;
	}

	//std::cerr << "vmcore::route_created() " << cnetlink::get_instance().get_route(table_id, rtindex) << std::endl;
	if (dptroutes[table_id].find(rtindex) == dptroutes[table_id].end()) {
		dptroutes[table_id][rtindex] = new dptroute(this, dpt, table_id, rtindex);
	}
}



void
vmcore::route_updated(uint8_t table_id, unsigned int rtindex)
{
	// ignore local route table and unspecified table_id
	if ((255 == table_id) || (0 == table_id)) {
		//std::cerr << "vmcore::route_updated() => suppressing table_id=" << (unsigned int)table_id << std::endl;
		return;
	}

	//std::cerr << "vmcore::route_updated() " << cnetlink::get_instance().get_route(table_id, rtindex) << std::endl;
	if (dptroutes[table_id].find(rtindex) == dptroutes[table_id].end()) {
		route_created(table_id, rtindex);
	}

	// do nothing here, this event is handled directly by dptroute instance
}



void
vmcore::route_deleted(uint8_t table_id, unsigned int rtindex)
{
	// ignore local route table and unspecified table_id
	if ((255 == table_id) || (0 == table_id)) {
		//std::cerr << "vmcore::route_deleted() => suppressing table_id=" << (unsigned int)table_id << std::endl;
		return;
	}

	//std::cerr << "vmcore::route_deleted() " << cnetlink::get_instance().get_route(table_id, rtindex) << std::endl;
	if (dptroutes[table_id].find(rtindex) != dptroutes[table_id].end()) {
		delete dptroutes[table_id][rtindex];
		dptroutes[table_id].erase(rtindex);
	}
}





