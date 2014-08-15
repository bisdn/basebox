/*
 * cdptport.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#include "clink.hpp"

using namespace ipcore;




clink::clink() :
		state(STATE_DETACHED),
		ofp_port_no(0),
		ifindex(0),
		tapdev(0),
		in_ofp_table_id(0),
		fwd_ofp_table_id(1),
		out_ofp_table_id(2)
{

}



clink::clink(
		const rofl::cdptid& dptid,
		uint32_t ofp_port_no,
		const std::string& devname,
		const rofl::caddress_ll& hwaddr,
		uint8_t in_ofp_table_id,
		uint8_t fwd_ofp_table_id,
		uint8_t out_ofp_table_id) :
				state(STATE_DETACHED),
				ofp_port_no(ofp_port_no),
				devname(devname),
				hwaddr(hwaddr),
				ifindex(0),
				tapdev(0),
				dptid(dptid),
				in_ofp_table_id(in_ofp_table_id),
				fwd_ofp_table_id(fwd_ofp_table_id),
				out_ofp_table_id(out_ofp_table_id)
{

}



clink::~clink()
{
	if (ifindex > 0) {
		tap_close();
	}
	try {
		if (STATE_ATTACHED == state) {
			handle_dpt_close(rofl::crofdpt::get_dpt(dptid));
		}
	} catch (rofl::eRofDptNotFound& e) {};
}



void
clink::tap_open()
{
	if (NULL != tapdev) {
		// tap device already exists, ignoring
		return;
	}

	try {
		std::cerr << "TTT1: " << get_devname() << std::endl;
	} catch (...) {
		std::cerr << "UUU1" << std::endl;

		return;
	}

	try {
		std::cerr << "TTT2: " << get_hwaddr() << std::endl;
	} catch (...) {
		std::cerr << "UUU2" << std::endl;

		return;
	}

	tapdev = new rofcore::ctapdev(this, get_devname(), get_hwaddr());

	ifindex = tapdev->get_ifindex();

	flags.set(FLAG_TAP_DEVICE_ACTIVE);
}



void
clink::tap_close()
{
	if (tapdev) delete tapdev;

	tapdev = NULL;

	ifindex = -1;

	flags.reset(FLAG_TAP_DEVICE_ACTIVE);
}



void
clink::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		if (not rofl::crofdpt::get_dpt(dptid).get_channel().is_established()) {
			throw eLinkNoDptAttached("clink::enqueue() dpt not found");
		}

		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eLinkTapDevNotFound("clink::enqueue() tap device not found");
		}

		rofl::openflow::cofactions actions(rofl::crofdpt::get_dpt(dptid).get_version());
		actions.set_action_output(rofl::cindex(0)).set_port_no(ofp_port_no);

		rofl::crofdpt::get_dpt(dptid).send_packet_out_message(
				rofl::cauxid(0),
				rofl::openflow::base::get_ofp_no_buffer(rofl::crofdpt::get_dpt(dptid).get_version()),
				rofl::openflow::base::get_ofpp_controller_port(rofl::crofdpt::get_dpt(dptid).get_version()),
				actions,
				pkt->soframe(),
				pkt->framelen());

	} catch (eLinkNoDptAttached& e) {
		rofcore::logging::warn << "[ipcore][clink][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkTapDevNotFound& e) {
		rofcore::logging::warn << "[ipcore][clink][enqueue] unable to find tap device" << std::endl;
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}



void
clink::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}


void
clink::handle_dpt_open(rofl::crofdpt& dpt)
{
	try {
		state = STATE_ATTACHED;

		for (std::map<unsigned int, caddr_in4*>::iterator
				it = addrs_in4.begin(); it != addrs_in4.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, caddr_in6*>::iterator
				it = addrs_in6.begin(); it != addrs_in6.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, cneigh_in4*>::iterator
				it = neighs_in4.begin(); it != neighs_in4.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}
		for (std::map<unsigned int, cneigh_in6*>::iterator
				it = neighs_in6.begin(); it != neighs_in6.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}

	} catch (rofl::eRofBaseNotConnected& e) {

	} catch (rofl::eRofSockTxAgain& e) {

	}
}



void
clink::handle_dpt_close(
		rofl::crofdpt& dpt)
{
	try {
		state = STATE_DETACHED;

		for (std::map<unsigned int, caddr_in4*>::iterator
				it = addrs_in4.begin(); it != addrs_in4.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, caddr_in6*>::iterator
				it = addrs_in6.begin(); it != addrs_in6.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, cneigh_in4*>::iterator
				it = neighs_in4.begin(); it != neighs_in4.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}
		for (std::map<unsigned int, cneigh_in6*>::iterator
				it = neighs_in6.begin(); it != neighs_in6.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}

	} catch (rofl::eRofBaseNotConnected& e) {

	} catch (rofl::eRofSockTxAgain& e) {

	}
}



void
clink::handle_packet_in(
		rofl::cpacket const& pack)
{
	if (0 == tapdev)
		return;

	rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();

	*pkt = pack;

	tapdev->enqueue(pkt);
}



void
clink::handle_port_status(
		rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg)
{
	try {
		if (0 == tapdev)
			return;

		if (msg.get_port().link_state_is_link_down() || msg.get_port().config_is_port_down()) {
			tapdev->disable_interface();
		} else {
			tapdev->enable_interface();
		}

	} catch (rofl::openflow::ePortNotFound& e) {

	}
}





