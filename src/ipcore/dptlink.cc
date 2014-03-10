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
		rofl::crofdpt *dpt,
		uint32_t of_port_no) :
				rofbase(rofbase),
				dpt(dpt),
				of_port_no(of_port_no),
				tapdev(0),
				ifindex(0)
{
	tapdev = new ctapdev(this, dpt->get_ports().get_port(of_port_no).get_name(), dpt->get_ports().get_port(of_port_no).get_hwaddr());

	ifindex = tapdev->get_ifindex();

	dptlink::dptlinks[ifindex] = this;
}



dptlink::~dptlink()
{
	dptlink::dptlinks.erase(ifindex);

	if (tapdev) delete tapdev;
}



void
dptlink::open()
{
	// do nothing
}



void
dptlink::close()
{
	delete_all_neighs();

	delete_all_addrs();
}



void
dptlink::delete_all_addrs()
{
	for (std::map<uint16_t, dptaddr>::iterator
			it = addrs.begin(); it != addrs.end(); ++it) {
		it->second.close();
	}
	addrs.clear();
}



void
dptlink::delete_all_neighs()
{
	for (std::map<uint16_t, dptneigh>::iterator
			it = neighs.begin(); it != neighs.end(); ++it) {
		it->second.close();
	}
	neighs.clear();
}



void
dptlink::enqueue(cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		if ((0 == dpt) || (not dpt->get_channel().is_established())) {
			throw eDptLinkNoDptAttached();
		}

		ctapdev* tapdev = dynamic_cast<ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eDptLinkTapDevNotFound();
		}

		rofl::cofactions actions(dpt->get_version());
		actions.append_action_output(of_port_no);

		dpt->send_packet_out_message(
				rofl::openflow::base::get_ofp_no_buffer(dpt->get_version()),
				rofl::openflow::base::get_ofpp_controller_port(dpt->get_version()),
				actions,
				pkt->soframe(),
				pkt->framelen());

	} catch (eDptLinkNoDptAttached& e) {
		rofl::logging::warn << "[ipcore][dptlink][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eDptLinkTapDevNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink][enqueue] unable to find tap device" << std::endl;
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
		rofl::cofport& port = dpt->set_ports().set_port(of_port_no);

		uint32_t config = dpt->get_ports().get_port(of_port_no).get_config();

		uint32_t state  = dpt->get_ports().get_port(of_port_no).get_state();

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

	rofl::logging::info << "[ipcore][dptlink] crtlink CREATE:" << std::endl << cnetlink::get_instance().get_link(ifindex);
}



void
dptlink::link_updated(unsigned int ifindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		rofl::logging::info << "[ipcore][dptlink] crtlink UPDATE:" << std::endl << cnetlink::get_instance().get_link(ifindex);

		if (0 == dpt) {
			return;
		}

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtlink UPDATE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;
	}
}



void
dptlink::link_deleted(unsigned int ifindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	rofl::logging::info << "[ipcore][dptlink] crtlink DELETE:" << std::endl << cnetlink::get_instance().get_link(ifindex);
}



void
dptlink::addr_created(unsigned int ifindex, uint16_t adindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		if (addrs.find(adindex) != addrs.end()) {
			// safe strategy: remove old FlowMod first
			addrs[adindex].close();
		}

		rofl::logging::info << "[ipcore][dptlink] crtaddr CREATE:" << std::endl << cnetlink::get_instance().get_link(ifindex).get_addr(adindex);

		addrs[adindex] = dptaddr(rofbase, dpt, ifindex, adindex);

		addrs[adindex].open();

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtaddr CREATE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;

	} catch (eRtLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtaddr CREATE notification rcvd => "
				<< "unable to find crtaddr for adindex:" << adindex << std::endl;
	}
}



void
dptlink::addr_updated(unsigned int ifindex, uint16_t adindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		if (addrs.find(adindex) == addrs.end()) {
			addr_created(ifindex, adindex);
		}

		// TODO: check status changes and notify dptaddr instance

		rofl::logging::info << "[ipcore][dptlink] crtaddr UPDATE:" << std::endl << cnetlink::get_instance().get_link(ifindex).get_addr(adindex);

		addrs[adindex].close();
		addrs[adindex].open();

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtaddr UPDATE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;

	} catch (eRtLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtaddr UPDATE notification rcvd => "
				<< "unable to find crtaddr for adindex:" << adindex << std::endl;
	}
}



void
dptlink::addr_deleted(unsigned int ifindex, uint16_t adindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		rofl::logging::info << "[ipcore][dptlink] crtaddr DELETE:" << std::endl << cnetlink::get_instance().get_link(ifindex).get_addr(adindex);

		if (addrs.find(adindex) == addrs.end()) {
			// we have no dptaddr instance for the crtaddr instance scheduled for removal, so ignore it
			return;
		}

		addrs[adindex].close();

		addrs.erase(adindex);

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtaddr DELETE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;

	} catch (eRtLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtaddr DELETE notification rcvd => "
				<< "unable to find crtaddr for adindex:" << adindex << std::endl;
	}
}



void
dptlink::neigh_created(unsigned int ifindex, uint16_t nbindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		rofl::logging::info << "[ipcore][dptlink] crtneigh CREATE:" << std::endl << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

		if (neighs.find(nbindex) != neighs.end()) {
			// safe strategy: remove old FlowMod first
			neighs[nbindex].close();
		}

		neighs[nbindex] = dptneigh(rofbase, dpt, of_port_no, /*of_table_id=*/2, ifindex, nbindex);

		neighs[nbindex].open();

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtneigh CREATE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;

	} catch (eRtLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtneigh CREATE notification rcvd => "
				<< "unable to find crtneigh for nbindex:" << nbindex << std::endl;
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

			rofl::logging::info << "[ipcore][dptlink] crtneigh UPDATE (scheduled for removal):" << std::endl << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

			neighs[nbindex].close();
			neighs.erase(nbindex);
		} break;

		case NUD_STALE:
		case NUD_NOARP:
		case NUD_REACHABLE:
		case NUD_PERMANENT: {
			if (neighs.find(nbindex) != neighs.end()) {
				neighs[nbindex].close();
				neighs.erase(nbindex);
			}

			rofl::logging::info << "[ipcore][dptlink] crtneigh UPDATE (refresh):" << std::endl << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

			neighs[nbindex] = dptneigh(rofbase, dpt, of_port_no, /*of_table_id=*/2, ifindex, nbindex);

			neighs[nbindex].open();

		} break;
		}

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtneigh UPDATE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;
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

		rofl::logging::info << "[ipcore][dptlink] crtneigh DELETE:" << std::endl << cnetlink::get_instance().get_link(ifindex).get_neigh(nbindex);

		neighs[nbindex].close();

		neighs.erase(nbindex);

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtneigh DELETE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;

	} catch (eRtLinkNotFound& e) {
		rofl::logging::warn << "[ipcore][dptlink] crtneigh DELETE notification rcvd => "
				<< "unable to find crtneigh for nbindex:" << nbindex << std::endl;
	}
}


