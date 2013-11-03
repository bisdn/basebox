/*
 * cdptport.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#include <dptlink.h>

using namespace dptmap;

std::map<unsigned int, dptlink*> dptlink::dptlinks;


dptlink&
dptlink::get_dptlink(unsigned int ifindex)
{
	if (dptlink::dptlinks.find(ifindex) == dptlink::dptlinks.end()) {
		throw eDptLinkNotFound();
	}
	return *(dptlink::dptlinks[ifindex]);
}


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

	dptlink::dptlinks[ifindex] = this;
}



dptlink::~dptlink()
{
	dptlink::dptlinks.erase(ifindex);

	delete_all_neighs();

	delete_all_addrs();

	if (tapdev) delete tapdev;
}



void
dptlink::delete_all_addrs()
{
	for (std::map<uint16_t, dptaddr>::iterator
			it = addrs.begin(); it != addrs.end(); ++it) {
		it->second.flow_mod_delete();
	}
	addrs.clear();
}



void
dptlink::delete_all_neighs()
{
	for (std::map<uint16_t, dptneigh>::iterator
			it = neighs.begin(); it != neighs.end(); ++it) {
		it->second.flow_mod_delete();
	}
	neighs.clear();
}



void
dptlink::enqueue(cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		if (0 == dpt) {
			throw eDptLinkNoDptAttached();
		}

		ctapdev* tapdev = dynamic_cast<ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eDptLinkTapDevNotFound();
		}

		rofl::cofaclist actions;
		actions.next() = rofl::cofaction_output(dpt->get_version(), of_port_no);

		uint32_t port_no = 0;
		switch (dpt->get_version()) {
		case OFP10_VERSION: port_no = OFPP10_CONTROLLER; break;
		case OFP12_VERSION: port_no = OFPP12_CONTROLLER; break;
		case OFP13_VERSION: port_no = OFPP13_CONTROLLER; break;
		}

		rofbase->send_packet_out_message(dpt, OFP_NO_BUFFER, port_no, actions, pkt->soframe(), pkt->framelen());

	} catch (eDptLinkNoDptAttached& e) {

		fprintf(stderr, "dptport::enqueue() no data path attached\n");

	} catch (eDptLinkTapDevNotFound& e) {

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
		rofl::cofport& port = dpt->get_port(of_port_no);

		uint32_t config = dpt->get_port(of_port_no).get_config();

		uint32_t state  = dpt->get_port(of_port_no).get_state();

		if (port.link_state_is_link_down() || port.config_is_port_down()) {
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
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		//fprintf(stderr, "dptport::link_updated() ifindex=%d => ", ifindex);
		std::cerr << cnetlink::get_instance().get_link(ifindex) << std::endl;

		if (0 == dpt) {
			return;
		}

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptlink::link_updated() crtlink object not found" << std::endl;
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
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		std::cerr << "ADDR ADD? " << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;

		if (addrs.find(adindex) != addrs.end()) {
			// safe strategy: remove old FlowMod first
			addrs[adindex].flow_mod_delete();
		}

#if 0
		fprintf(stderr, "dptlink::addr_created() ifindex=%d adindex=%d => ", ifindex, adindex);
		std::cerr << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;
#endif

		addrs[adindex] = dptaddr(rofbase, dpt, ifindex, adindex);

		addrs[adindex].flow_mod_add();

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptlink::addr_created() crtlink object not found" << std::endl;

	} catch (eRtLinkNotFound& e) {
		std::cerr << "dptlink::addr_created() crtaddr object not found" << std::endl;
	}
}



void
dptlink::addr_updated(unsigned int ifindex, uint16_t adindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		std::cerr << "ADDR UPDATE? " << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;

		if (addrs.find(adindex) == addrs.end()) {
			addr_created(ifindex, adindex);
		}

		// TODO: check status changes and notify dptaddr instance

		addrs[adindex].flow_mod_modify();

		fprintf(stderr, "dptlink::addr_updated() ifindex=%d adindex=%d => ", ifindex, adindex);
		std::cerr << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptlink::addr_updated() crtlink object not found" << std::endl;

	} catch (eRtLinkNotFound& e) {
		std::cerr << "dptlink::addr_updated() crtaddr object not found" << std::endl;
	}
}



void
dptlink::addr_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		std::cerr << "ADDR DELETE? " << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;

		if (addrs.find(adindex) == addrs.end()) {
			// we have no dptaddr instance for the crtaddr instance scheduled for removal, so ignore it
			return;
		}

#if 0
		fprintf(stderr, "dptlink::addr_deleted() ifindex=%d adindex=%d => ", ifindex, adindex);
		std::cerr << cnetlink::get_instance().get_link(ifindex).get_addr(adindex) << std::endl;
#endif

		addrs[adindex].flow_mod_delete();

		addrs.erase(adindex);

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptlink::addr_deleted() crtlink object not found" << std::endl;

	} catch (eRtLinkNotFound& e) {
		std::cerr << "dptlink::addr_deleted() crtaddr object not found" << std::endl;
	}
}



void
dptlink::neigh_created(unsigned int ifindex, uint16_t nbindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		if (neighs.find(nbindex) != neighs.end()) {
			// safe strategy: remove old FlowMod first
			neighs[nbindex].flow_mod_delete();
		}

#if 0
		fprintf(stderr, "dptlink::neigh_created() ifindex=%d nbindex=%d => ", ifindex, nbindex);
		std::cerr << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex) << std::endl;
#endif

		neighs[nbindex] = dptneigh(rofbase, dpt, of_port_no, /*of_table_id=*/2, ifindex, nbindex);

		neighs[nbindex].flow_mod_add();

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptlink::neigh_created() crtlink object not found" << std::endl;

	} catch (eRtLinkNotFound& e) {
		std::cerr << "dptlink::neigh_created() crtneigh object not found" << std::endl;
	}
}



void
dptlink::neigh_updated(unsigned int ifindex, uint16_t nbindex)
{
	try {

		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		if (neighs.find(nbindex) == neighs.end()) {
			neigh_created(ifindex, nbindex);
		}

		// TODO: check status changes and notify dptneigh instance

		crtneigh& rtn = cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

		switch (rtn.get_state()) {
		case NUD_INCOMPLETE:
		case NUD_DELAY:
		case NUD_PROBE:
		case NUD_FAILED: {
			/* go on and delete entry */
			if (neighs.find(nbindex) == neighs.end())
				return;
			std::cerr << "dptlink::neigh_updated() DELETE => " << neighs[nbindex] << std::endl;
			neighs[nbindex].flow_mod_delete();
			neighs.erase(nbindex);
		} break;

		case NUD_STALE:
		case NUD_NOARP:
		case NUD_REACHABLE:
		case NUD_PERMANENT: {
			if (neighs.find(nbindex) != neighs.end()) {
				neighs[nbindex].flow_mod_delete();
				neighs.erase(nbindex);
			}

			std::cerr << "dptlink::neigh_updated() ADD => " << neighs[nbindex] << std::endl;

			neighs[nbindex] = dptneigh(rofbase, dpt, of_port_no, /*of_table_id=*/2, ifindex, nbindex);

			neighs[nbindex].flow_mod_add();

		} break;
		}

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptlink::neigh_created() crtlink object not found" << std::endl;

	}
}



void
dptlink::neigh_deleted(unsigned int ifindex, uint16_t nbindex)
{
	try {
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

	} catch (eNetLinkNotFound& e) {
		std::cerr << "dptlink::neigh_deleted() crtlink object not found" << std::endl;

	} catch (eRtLinkNotFound& e) {
		std::cerr << "dptlink::neigh_deleted() crtneigh object not found" << std::endl;
	}
}


