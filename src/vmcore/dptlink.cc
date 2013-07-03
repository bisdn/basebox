/*
 * cdptport.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#include <dptlink.h>

using namespace dptmap;

dptlink::dptlink(
		rofl::crofbase *rofbase,
		rofl::cofdpt *dpt,
		uint32_t of_port_no) :
				rofbase(rofbase),
				dpt(dpt),
				of_port_no(of_port_no),
				tapdev(0),
				ifindex(0)
{
	tapdev = new ctapdev(this, dpt->get_ports()[of_port_no]->get_name(), dpt->get_ports()[of_port_no]->get_hwaddr());

	ifindex = tapdev->get_ifindex();
}



dptlink::~dptlink()
{
	for (std::map<uint16_t, dptneigh>::iterator
			it = neighs.begin(); it != neighs.end(); ++it) {
		it->second.flow_mod_delete();
	}
	neighs.clear();

	for (std::map<uint16_t, dptaddr>::iterator
			it = addrs.begin(); it != addrs.end(); ++it) {
		it->second.flow_mod_delete();
	}
	addrs.clear();

	if (tapdev) delete tapdev;
}



void
dptlink::enqueue(cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		if (0 == dpt) {
			throw eDptPortNoDptAttached();
		}

		ctapdev* tapdev = dynamic_cast<ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eDptPortTapDevNotFound();
		}

		rofl::cofaclist actions;
		actions.next() = rofl::cofaction_output(of_port_no);

		// FIXME: check the crc stuff
		rofbase->send_packet_out_message(dpt, OFP_NO_BUFFER, OFPP_CONTROLLER, actions, pkt->soframe(), pkt->framelen() - sizeof(uint32_t)/*CRC*/);

	} catch (eDptPortNoDptAttached& e) {

		fprintf(stderr, "dptport::enqueue() no data path attached\n");

	} catch (eDptPortTapDevNotFound& e) {

		fprintf(stderr, "dptport::enqueue() no tap device found\n");

	}

	cpacketpool::get_instance().release_pkt(pkt);
}



void
dptlink::enqueue(cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}



void
dptlink::handle_packet_in(rofl::cpacket const& pack)
{
	if (0 == tapdev)
		return;

	rofl::cpacket *pkt = cpacketpool::get_instance().acquire_pkt();

	*pkt = pack;

	tapdev->enqueue(pkt);
}



void
dptlink::handle_port_status()
{
	try {
		uint32_t config = dpt->get_port(of_port_no).get_config();

		uint32_t state  = dpt->get_port(of_port_no).get_state();

		if ((state & OFPPS_LINK_DOWN) || (config & OFPPC_PORT_DOWN)) {
			tapdev->disable_interface();
		} else {
			tapdev->enable_interface();
		}

	} catch (rofl::eOFdpathNotFound& e) {

	}
}



void
dptlink::link_created(unsigned int ifindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	//fprintf(stderr, "dptport::link_created() ifindex=%d => ", ifindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex) << std::endl;
}



void
dptlink::link_updated(unsigned int ifindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	//fprintf(stderr, "dptport::link_updated() ifindex=%d => ", ifindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex) << std::endl;

	if (0 == dpt) {
		return;
	}
}



void
dptlink::link_deleted(unsigned int ifindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	//fprintf(stderr, "dptport::link_deleted() ifindex=%d\n", ifindex);
}



void
dptlink::addr_created(unsigned int ifindex, uint16_t adindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	if (addrs.find(adindex) != addrs.end()) {
		// safe strategy: remove old FlowMod first
		addrs[adindex].flow_mod_delete();
	}

	fprintf(stderr, "dptlink::addr_created() ifindex=%d adindex=%d => ", ifindex, adindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;

	addrs[adindex] = dptaddr(rofbase, dpt, ifindex, adindex);

	addrs[adindex].flow_mod_add();
}



void
dptlink::addr_updated(unsigned int ifindex, uint16_t adindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	if (addrs.find(adindex) == addrs.end()) {
		addr_created(ifindex, adindex);
	}

	// TODO: check status changes and notify dptaddr instance

	addrs[adindex].flow_mod_modify();

	fprintf(stderr, "dptlink::addr_updated() ifindex=%d adindex=%d => ", ifindex, adindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;
}



void
dptlink::addr_deleted(unsigned int ifindex, uint16_t adindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	if (addrs.find(adindex) == addrs.end()) {
		// we have no dptaddr instance for the crtaddr instance scheduled for removal, so ignore it
		return;
	}

	fprintf(stderr, "dptlink::addr_deleted() ifindex=%d adindex=%d => ", ifindex, adindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;

	addrs[adindex].flow_mod_delete();

	addrs.erase(adindex);
}



void
dptlink::neigh_created(unsigned int ifindex, uint16_t nbindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	if (neighs.find(nbindex) != neighs.end()) {
		// safe strategy: remove old FlowMod first
		neighs[nbindex].flow_mod_delete();
	}

	fprintf(stderr, "dptlink::neigh_created() ifindex=%d nbindex=%d => ", ifindex, nbindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex) << std::endl;

	neighs[nbindex] = dptneigh(rofbase, dpt, of_port_no, ifindex, nbindex);

	neighs[nbindex].flow_mod_add();
}



void
dptlink::neigh_updated(unsigned int ifindex, uint16_t nbindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	if (neighs.find(nbindex) == neighs.end()) {
		neigh_created(ifindex, nbindex);
	}

	// TODO: check status changes and notify dptneigh instance

	neighs[nbindex].flow_mod_modify();

	fprintf(stderr, "dptlink::neigh_updated() ifindex=%d nbindex=%d => ", ifindex, nbindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex) << std::endl;
}



void
dptlink::neigh_deleted(unsigned int ifindex, uint16_t nbindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	if (neighs.find(nbindex) == neighs.end()) {
		// we have no dptaddr instance for the crtaddr instance scheduled for removal, so ignore it
		return;
	}

	fprintf(stderr, "dptlink::neigh_deleted() ifindex=%d nbindex=%d => ", ifindex, nbindex);
	std::cerr << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex) << std::endl;

	neighs[nbindex].flow_mod_delete();

	neighs.erase(nbindex);
}


