/*
 * cdptport.cc
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#include <dptlink.h>

using namespace ipcore;




cdptlink::cdptlink() :
				ofp_port_no(0),
				ifindex(0),
				tapdev(0),
				table_id(0)
{

}



cdptlink::~cdptlink()
{
	if (ifindex > 0) {
		tap_close();
	}
}



void
cdptlink::tap_open()
{
	if (NULL != tapdev) {
		// tap device already exists, ignoring
		return;
	}

	tapdev = new rofcore::ctapdev(this, get_devname(), get_hwaddr());

	ifindex = tapdev->get_ifindex();

	flags.set(FLAG_TAP_DEVICE_ACTIVE);
}



void
cdptlink::tap_close()
{
	if (tapdev) delete tapdev;

	tapdev = NULL;

	ifindex = -1;

	flags.reset(FLAG_TAP_DEVICE_ACTIVE);
}



void
cdptlink::install()
{
	addrtable.install();
	neightable.install();
	flags.set(FLAG_FLOW_MOD_INSTALLED);
}



void
cdptlink::uninstall()
{
	addrtable.uninstall();
	neightable.uninstall();
	flags.reset(FLAG_FLOW_MOD_INSTALLED);
}



void
cdptlink::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	try {
		if (not rofl::crofdpt::get_dpt(dptid).get_channel().is_established())
			throw eDptLinkNoDptAttached();
		}

		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		if (0 == tapdev) {
			throw eDptLinkTapDevNotFound();
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

	} catch (eDptLinkNoDptAttached& e) {
		rofcore::logging::warn << "[ipcore][dptlink][enqueue] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eDptLinkTapDevNotFound& e) {
		rofcore::logging::warn << "[ipcore][dptlink][enqueue] unable to find tap device" << std::endl;
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}



void
cdptlink::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}



void
cdptlink::handle_packet_in(rofl::cpacket const& pack)
{
	if (0 == tapdev)
		return;

	rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();

	*pkt = pack;

	tapdev->enqueue(pkt);
}



void
cdptlink::handle_port_status()
{
	try {
		if (0 == tapdev)
			return;

		if (get_ofp_port().link_state_is_link_down() || get_ofp_port().config_is_port_down()) {
			tapdev->disable_interface();
		} else {
			tapdev->enable_interface();
		}

	} catch (rofl::openflow::ePortNotFound& e) {

	}
}





