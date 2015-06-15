#include "switch_behavior_ofdpa.hpp"
#include "ofdpa_fm_driver.hpp"
#include "ofdpa_datatypes.h"
#include "roflibs/netlink/clogging.hpp"

#include <map>
#include <rofl/common/openflow/cofport.h>
#include <rofl/common/openflow/cofports.h>
#include <rofl/common/protocols/fetherframe.h>


namespace basebox {

switch_behavior_ofdpa::switch_behavior_ofdpa(const rofl::cdptid& dptid) :
		switch_behavior(dptid),
		fm_driver(dptid),
		bridge(fm_driver)
{
	rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);

	rofcore::logging::debug << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] dpt: " << std::endl << dpt;

	init_ports();
}

switch_behavior_ofdpa::~switch_behavior_ofdpa()
{
	//todo need this?
//	try {
//		if (STATE_ATTACHED == state) {
//			handle_dpt_close();
//		}
//	} catch (rofl::eRofDptNotFound& e) {}

	clear_tap_devs(dptid);
}

void
switch_behavior_ofdpa::handle_packet_in(rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg)
{
	if (this->dptid != dpt.get_dptid()) { // todo maybe even assert here?
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] wrong dptid received" << std::endl;
		return;
	}

	switch (msg.get_table_id()) {
		case OFDPA_FLOW_TABLE_ID_SA_LOOKUP:
			this->handle_srcmac_table(msg);
			break;
		default:
			break;
	}

	// todo remove?
	if (rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()) == msg.get_buffer_id()) {
		// got the full packet
		// fixme deal with pkt-in
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] got packet-in:" << std::endl;

	} else {
		// packet is buffered
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] cannot handle buffered packet currently" << std::endl;
	}
}

void switch_behavior_ofdpa::handle_flow_removed(rofl::crofdpt& dpt,
		const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg)
{
	if (this->dptid != dpt.get_dptid()) { // todo maybe even assert here?
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] wrong dptid received" << std::endl;
		return;
	}

	std::cout << __FUNCTION__ << ": msg:" << std::endl << msg;

	// xxx implement flow_removed
}


void
switch_behavior_ofdpa::init_ports()
{
	using rofl::openflow::cofport;
	using std::map;
	using std::set;

	/* init 1:1 port mapping */
	try {
		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(dptid);
		const map<uint32_t, cofport*> &ports = dpt.get_ports().get_ports();

		for (const auto &i : dpt.get_ports().get_ports()) {
			const cofport* port = i.second;
			if (not has_tap_dev(dptid, port->get_name())) {
				rofcore::logging::debug << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] adding port " << port->get_name() << " with portno=" << port->get_port_no() << std::endl;
				add_tap_dev(dptid, port->get_name(), default_pvid, port->get_hwaddr());
			}
		}

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__<< "] ERROR: eRofDptNotFound" << std::endl;
	}
}

uint32_t
switch_behavior_ofdpa::get_of_port_no(const rofl::crofdpt& dpt, const std::string& dev) const
{
	using rofl::openflow::cofport;
	using std::map;

	uint32_t port_no = 0;

	for (const auto &i : dpt.get_ports().get_ports()) {
		if (0 == i.second->get_name().compare(dev)) {
			rofcore::logging::debug << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] outport=" << i.first << std::endl;
			port_no = i.first;
			break;
		}
	}

	return port_no;
}

void
switch_behavior_ofdpa::handle_srcmac_table(
		rofl::openflow::cofmsg_packet_in& msg)
{
	rofcore::logging::info << __FUNCTION__ << ": handle message" << std::endl << msg;

	rofcore::logging::info << __FUNCTION__ << ": in_port=" << msg.get_match().get_in_port() << std::endl;

	rofl::fetherframe eth_frame(msg.get_packet().soframe(), msg.get_packet().length());

	bridge.add_mac_to_fdb(eth_frame.get_dl_src(), msg.get_match().get_in_port());
}

void
switch_behavior_ofdpa::enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt)
{
	using rofl::openflow::cofport;
	using std::map;

	try {
		rofcore::ctapdev* tapdev = dynamic_cast<rofcore::ctapdev*>( netdev );
		assert(tapdev);

		rofl::crofdpt& dpt = rofl::crofdpt::get_dpt(tapdev->get_dptid());

		if (not dpt.is_established()) {
			throw eLinkNoDptAttached("switch_behavior_ofdpa::enqueue() dpt not found");
		}

		// todo move to separate function:
		uint32_t portno = get_of_port_no(dpt, tapdev->get_devname());

		/* only send packet-out if we can determine a port-no */
		if (portno) {
			// XXX pkt-out temporarily disabled
//			rofl::openflow::cofactions actions(dpt.get_version_negotiated());
//			//actions.set_action_push_vlan(rofl::cindex(0)).set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
//			//actions.set_action_set_field(rofl::cindex(1)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(tapdev->get_pvid()));
//			actions.set_action_output(rofl::cindex(0)).set_port_no(portno);
//
//			dpt.send_packet_out_message(
//					rofl::cauxid(0),
//					rofl::openflow::base::get_ofp_no_buffer(dpt.get_version_negotiated()),
//					rofl::openflow::base::get_ofpp_controller_port(dpt.get_version_negotiated()),
//					actions,
//					pkt->soframe(),
//					pkt->length());

		}
	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkNoDptAttached& e) {
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] no data path attached, dropping outgoing packet" << std::endl;

	} catch (eLinkTapDevNotFound& e) {
		rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "] unable to find tap device" << std::endl;
	}

	rofcore::cpacketpool::get_instance().release_pkt(pkt);
}

void
switch_behavior_ofdpa::enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		enqueue(netdev, *it);
	}
}

void
switch_behavior_ofdpa::link_created(unsigned int ifindex)
{
	const rofcore::crtlink& rtl = rofcore::cnetlink::get_instance().get_links().get_link(ifindex);
	rofcore::logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__ << "]:" << std::endl << rtl;

	// currently ignore all interfaces besides the tap devs
	if (not has_tap_dev(this->dptid, rtl.get_devname())) {
		rofcore::logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
				<< "]: ignore interface " << rtl.get_devname() << std::endl
				<< rtl;
		return;
	}

	if (AF_BRIDGE == rtl.get_family()) {

		// check for new bridges
		if (rtl.get_master()) {
			// slave interface
			rofcore::logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__ << "]: is new slave interface" << std::endl;

			if (not bridge.has_bridge_interface()) {
				bridge.set_bridge_interface(rtl.get_master());
				// get of_port_no and add the interface to the bridge
				uint32_t port_no = get_of_port_no(rofl::crofdpt::get_dpt(this->dptid), rtl.get_devname());
				if (port_no) {
					bridge.add_interface(port_no);
				}
			} else if (bridge.get_bridge_interface() == rtl.get_master()) {
				// get of_port_no and add the interface to the bridge
				uint32_t port_no = get_of_port_no(rofl::crofdpt::get_dpt(this->dptid), rtl.get_devname());
				if (port_no) {
					bridge.add_interface(port_no);
				}
			} else {
				rofcore::logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__ << "]: only a single bridge is supported currently" << std::endl;
				// TODO implement fault handling
			}

		} else {
			// bridge
			rofcore::logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__ << "]: is new bridge" << std::endl;
		}

		// new bridge slaves will be shown as new link as well
	}
}

void
switch_behavior_ofdpa::link_updated(unsigned int ifindex)
{
	const rofcore::crtlink& rtl = rofcore::cnetlink::get_instance().get_links().get_link(ifindex);
	rofcore::logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__ << "]:" << std::endl << rtl;

	// fixme check for link de/attachments from/to bridges (i.e. check for master)
}

void
switch_behavior_ofdpa::link_deleted(unsigned int ifindex)
{
	const rofcore::crtlink& rtl = rofcore::cnetlink::get_instance().get_links().get_link(ifindex);
	rofcore::logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__ << "]: " << std::endl << rtl;



	// todo same as in link_updated?
}

} /* namespace basebox */
