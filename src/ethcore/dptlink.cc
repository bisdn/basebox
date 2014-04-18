/*
 * cdptport.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#include <dptlink.h>

using namespace ethercore;

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
	// do nothing
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

		rofl::openflow::cofactions actions(dpt->get_version());
		actions.append_action_output(of_port_no);

		dpt->send_packet_out_message(
				rofl::openflow::base::get_ofp_no_buffer(dpt->get_version()),
				rofl::openflow::base::get_ofpp_controller_port(dpt->get_version()),
				actions,
				pkt->soframe(),
				pkt->framelen());

	} catch (eDptLinkNoDptAttached& e) {
		rofl::logging::warn << "[vmcore][dptlink][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eDptLinkTapDevNotFound& e) {
		rofl::logging::warn << "[vmcore][dptlink][enqueue] unable to find tap device" << std::endl;
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
		rofl::openflow::cofport const& port = dpt->get_ports().get_port(of_port_no);

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

	rofl::logging::info << "[vmcore][dptlink] crtlink CREATE:" << std::endl << cnetlink::get_instance().get_link(ifindex);
}



void
dptlink::link_updated(unsigned int ifindex)
{
	try {
		// filter out any events not related to our port
		if (ifindex != this->ifindex)
			return;

		rofl::logging::info << "[vmcore][dptlink] crtlink UPDATE:" << std::endl << cnetlink::get_instance().get_link(ifindex);

		if (0 == dpt) {
			return;
		}

	} catch (eNetLinkNotFound& e) {
		rofl::logging::warn << "[vmcore][dptlink] crtlink UPDATE notification rcvd => "
				<< "unable to find crtlink for ifindex:" << ifindex << std::endl;
	}
}



void
dptlink::link_deleted(unsigned int ifindex)
{
	// filter out any events not related to our port
	if (ifindex != this->ifindex)
		return;

	rofl::logging::info << "[vmcore][dptlink] crtlink DELETE:" << std::endl << cnetlink::get_instance().get_link(ifindex);
}


