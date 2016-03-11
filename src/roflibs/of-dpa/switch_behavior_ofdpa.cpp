#include "switch_behavior_ofdpa.hpp"
#include "ofdpa_datatypes.h"
#include "roflibs/netlink/clogging.hpp"

#include <map>
#include <linux/if_ether.h>

#include <rofl/ofdpa/rofl_ofdpa_fm_driver.hpp>
#include <rofl/common/openflow/cofport.h>
#include <rofl/common/openflow/cofports.h>

namespace basebox {

struct vlan_hdr {
  struct ethhdr eth; // vid + cfi + pcp
  uint16_t vlan;     // ethernet type
} __attribute__((packed));

switch_behavior_ofdpa::switch_behavior_ofdpa(rofl::crofdpt &dpt)
    : switch_behavior(dpt.get_dptid()), fm_driver(dpt), bridge(fm_driver),
      dpt(dpt) {
  rofcore::logging::debug << "[switch_behavior_ofdpa][" << __FUNCTION__
                          << "] dpt: " << std::endl
                          << dpt;
  init_ports();
}

switch_behavior_ofdpa::~switch_behavior_ofdpa() {
  // todo need this?
  //	try {
  //		if (STATE_ATTACHED == state) {
  //			handle_dpt_close();
  //		}
  //	} catch (rofl::eRofDptNotFound& e) {}

  clear_tap_devs(dptid);
}

void switch_behavior_ofdpa::init_ports() {
  using rofl::openflow::cofport;
  using rofcore::logging;
  using std::map;
  using std::set;

  /* init 1:1 port mapping */
  try {
    for (const auto &i : dpt.get_ports().keys()) { // XXX get dpt using cdptid
      const cofport &port =
          dpt.get_ports().get_port(i); // XXX get dpt using cdptid
      if (not has_tap_dev(dptid, port.get_name())) {
        logging::notice << "[switch_behavior_ofdpa][" << __FUNCTION__
                        << "] adding port " << port.get_name()
                        << " with portno=" << port.get_port_no()
                        << " at dptid=" << dptid << std::endl;
        add_tap_dev(dptid, port.get_name(), 1,
                    port.get_hwaddr()); // todo remove pvid from tapdev
      }
    }

  } catch (rofl::eRofDptNotFound &e) {
    logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                   << "] ERROR: eRofDptNotFound" << std::endl;
  }
}

void switch_behavior_ofdpa::handle_packet_in(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_packet_in &msg) {
  using rofcore::logging;
  logging::debug << __FUNCTION__ << ": handle message" << std::endl << msg;

  if (this->dptid != dpt.get_dptid()) {
    logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                   << "] wrong dptid received" << std::endl;
    return;
  }

  switch (msg.get_table_id()) {
  case OFDPA_FLOW_TABLE_ID_SA_LOOKUP:
    this->handle_srcmac_table(dpt, msg);
    break;

  case OFDPA_FLOW_TABLE_ID_ACL_POLICY:
    this->handle_acl_policy_table(dpt, msg);
    break;
  default:
    break;
  }
}

void switch_behavior_ofdpa::handle_srcmac_table(
    const rofl::crofdpt &dpt, rofl::openflow::cofmsg_packet_in &msg) {
  using rofl::openflow::cofport;
  using rofcore::cnetlink;
  using rofcore::crtlink;
  using rofcore::logging;
  using rofl::caddress_ll;

  logging::info << __FUNCTION__ << ": in_port=" << msg.get_match().get_in_port()
                << std::endl;

  struct ethhdr *eth = (struct ethhdr *)msg.get_packet().soframe();

  uint16_t vlan = 0;
  if (ETH_P_8021Q == be16toh(eth->h_proto)) {
    vlan = be16toh(((struct vlan_hdr *)eth)->vlan) & 0xfff;
    logging::debug << __FUNCTION__ << ": vlan=0x" << std::hex << vlan
                   << std::dec << std::endl;
  }

  // todo this has to be improved
  const cofport &port = dpt.get_ports().get_port(msg.get_match().get_in_port());
  const crtlink &rtl =
      cnetlink::get_instance().get_links().get_link(port.get_name());

  if (0 == vlan) {
    vlan = rtl.get_pvid();
  }

  // update bridge fdb
  try {
    const caddress_ll srcmac(eth->h_source, ETH_ALEN);
    cnetlink::get_instance().add_neigh_ll(rtl.get_ifindex(), vlan, srcmac);
    bridge.add_mac_to_fdb(msg.get_match().get_in_port(), vlan, srcmac, false);
  } catch (rofcore::eNetLinkNotFound &e) {
    logging::notice << __FUNCTION__ << ": cannot add neighbor to interface"
                    << std::endl;
  } catch (rofcore::eNetLinkFailed &e) {
    logging::crit << __FUNCTION__ << ": netlink failed" << std::endl;
  }
}

void switch_behavior_ofdpa::handle_acl_policy_table(
    const rofl::crofdpt &dpt, rofl::openflow::cofmsg_packet_in &msg) {
  using rofl::openflow::cofport;

// fixme check for reason (ACTION)

#if 0 // xxx enable?
  struct ethhdr *eth = (struct ethhdr*)msg.get_packet().soframe();
  switch (htobe16(eth->h_proto)) {
  case ETH_P_8021Q:
    // fixme currently only arp should be coming in here
    break;
  default:
    rofcore::logging::error << __FUNCTION__
      << ": unsupported h_proto" << htobe16(eth->h_proto)
      << std::endl;
    return;
    break;
  }
#endif

  const cofport &port = dpt.get_ports().get_port(msg.get_match().get_in_port());
  rofcore::ctapdev &dev = set_tap_dev(dpt.get_dptid(), port.get_name());

  rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
  *pkt = msg.get_packet();

  dev.enqueue(pkt);
}

void switch_behavior_ofdpa::handle_flow_removed(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_flow_removed &msg) {

  using rofcore::logging;

  if (this->dptid != dpt.get_dptid()) {
    logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                   << "] wrong dptid received" << std::endl;
    return;
  }

  logging::info << __FUNCTION__ << ": msg:" << std::endl << msg;

  switch (msg.get_table_id()) {
  case OFDPA_FLOW_TABLE_ID_BRIDGING:
    this->handle_bridging_table_rm(dpt, msg);
    break;
  default:
    break;
  }
}

void switch_behavior_ofdpa::handle_bridging_table_rm(
    const rofl::crofdpt &dpt, rofl::openflow::cofmsg_flow_removed &msg) {
  using rofl::cmacaddr;
  using rofl::openflow::cofport;
  using rofcore::logging;
  using rofcore::cnetlink;
  using rofcore::ctapdev;

  logging::info << __FUNCTION__ << ": handle message" << std::endl << msg;

  cmacaddr eth_dst;
  uint16_t vlan = 0;
  try {
    eth_dst = msg.get_match().get_eth_dst();
    vlan = msg.get_match().get_vlan_vid() & 0xfff;
  } catch (rofl::openflow::eOxmNotFound &e) {
    logging::error << __FUNCTION__ << ": failed to get eth_dst or vlan"
                   << std::endl;
    return;
  }

  // todo this has to be improved
  uint32_t portno = msg.get_cookie(); // fixme cookiebox here??
  const cofport &port = dpt.get_ports().get_port(portno);
  const ctapdev &tapdev = get_tap_dev(dpt.get_dptid(), port.get_name());

  try {
    // update bridge fdb
    cnetlink::get_instance().drop_neigh_ll(tapdev.get_ifindex(), vlan, eth_dst);
  } catch (rofcore::eNetLinkFailed &e) {
    logging::crit << __FUNCTION__ << ": netlink failed: " << e.what()
                  << std::endl;
  }
}

void switch_behavior_ofdpa::enqueue(rofcore::cnetdev *netdev,
                                    rofl::cpacket *pkt) {
  using rofl::openflow::cofport;
  using rofcore::logging;
  using std::map;

  assert(pkt && "invalid enque");
  struct ethhdr *eth = (struct ethhdr *)pkt->soframe();

  if (eth->h_dest[0] == 0x33 && eth->h_dest[1] == 0x33) {
    logging::debug << "[switch_behavior_ofdpa][" << __FUNCTION__
                   << "]: drop multicast packet" << std::endl;
    rofcore::cpacketpool::get_instance().release_pkt(pkt);
    return;
  }

  try {
    rofcore::ctapdev *tapdev = dynamic_cast<rofcore::ctapdev *>(netdev);
    assert(tapdev);

    if (not dpt.is_established()) { // XXX get dpt using cdptid
      throw eLinkNoDptAttached(
          "switch_behavior_ofdpa::enqueue() dpt not found");
    }

    // todo move to separate function:
    uint32_t portno;
    try {
      // XXX get dpt using cdptid
      portno = dpt.get_ports().get_port(tapdev->get_devname()).get_port_no();
    } catch (rofl::openflow::ePortsNotFound &e) {
      logging::error << __FUNCTION__ << ": invalid port for packet "
                                        "out"
                     << std::endl;
      return;
    }

    /* only send packet-out if we can determine a port-no */
    if (portno) {
      logging::debug << "[switch_behavior_ofdpa][" << __FUNCTION__
                     << "]: send pkt-out, pkt:" << std::endl
                     << *pkt;

      rofl::openflow::cofactions actions(dpt.get_version());
      //			//actions.set_action_push_vlan(rofl::cindex(0)).set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
      //			//actions.set_action_set_field(rofl::cindex(1)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(tapdev->get_pvid()));
      actions.set_action_output(rofl::cindex(0)).set_port_no(portno);

      // XXX get dpt using cdptid
      dpt.send_packet_out_message(
          rofl::cauxid(0),
          rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()),
          rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()),
          actions, pkt->soframe(), pkt->length());
    }
  } catch (rofl::eRofDptNotFound &e) {
    logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                   << "] no data path attached, dropping outgoing packet"
                   << std::endl;

  } catch (eLinkNoDptAttached &e) {
    logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                   << "] no data path attached, dropping outgoing packet"
                   << std::endl;

  } catch (eLinkTapDevNotFound &e) {
    logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                   << "] unable to find tap device" << std::endl;
  }

  rofcore::cpacketpool::get_instance().release_pkt(pkt);
}

void switch_behavior_ofdpa::enqueue(rofcore::cnetdev *netdev,
                                    std::vector<rofl::cpacket *> pkts) {
  for (std::vector<rofl::cpacket *>::iterator it = pkts.begin();
       it != pkts.end(); ++it) {
    enqueue(netdev, *it);
  }
}

void switch_behavior_ofdpa::link_created(unsigned int ifindex) {
  using rofcore::logging;

  const rofcore::crtlink &rtl =
      rofcore::cnetlink::get_instance().get_links().get_link(ifindex);
  logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
                << "]:" << std::endl
                << rtl;

  // currently ignore all interfaces besides the tap devs
  if (not has_tap_dev(dptid, rtl.get_devname())) {
    logging::notice << "[switch_behavior_ofdpa][" << __FUNCTION__
                    << "]: ignore interface " << rtl.get_devname()
                    << " with dptid=" << dptid << std::endl
                    << rtl;
    return;
  }

  if (AF_BRIDGE == rtl.get_family()) {

    // check for new bridge slaves
    if (rtl.get_master()) {
      // slave interface
      logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
                    << "]: is new slave interface" << std::endl;

      // use only first bridge an of interface is attached to
      if (not bridge.has_bridge_interface()) {
        bridge.set_bridge_interface(
            rofcore::cnetlink::get_instance().get_links().get_link(
                rtl.get_master()));
      }

      bridge.add_interface(rtl);
    } else {
      // bridge (master)
      logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
                    << "]: is new bridge" << std::endl;
    }
  }
}

void switch_behavior_ofdpa::link_updated(const rofcore::crtlink &newlink) {
  using rofcore::crtlink;
  using rofcore::cnetlink;
  using rofcore::logging;

  // currently ignore all interfaces besides the tap devs
  if (not has_tap_dev(dptid, newlink.get_devname())) {
    logging::notice << "[switch_behavior_ofdpa][" << __FUNCTION__
                    << "]: ignore interface " << newlink.get_devname()
                    << " with dptid=" << dptid << std::endl
                    << newlink;
    return;
  }

  const crtlink &oldlink =
      cnetlink::get_instance().get_links().get_link(newlink.get_ifindex());
  logging::notice << "[switch_behavior_ofdpa][" << __FUNCTION__
                  << "] oldlink:" << std::endl
                  << oldlink;
  logging::notice << "[switch_behavior_ofdpa][" << __FUNCTION__
                  << "] newlink:" << std::endl
                  << newlink;

  bridge.update_interface(oldlink, newlink);
}

void switch_behavior_ofdpa::link_deleted(unsigned int ifindex) {
  const rofcore::crtlink &rtl =
      rofcore::cnetlink::get_instance().get_links().get_link(ifindex);

  // currently ignore all interfaces besides the tap devs
  if (not has_tap_dev(dptid, rtl.get_devname())) {
    rofcore::logging::notice << "[switch_behavior_ofdpa][" << __FUNCTION__
                             << "]: ignore interface " << rtl.get_devname()
                             << " with dptid=" << dptid << std::endl
                             << rtl;
    return;
  }

  bridge.delete_interface(rtl);
}

void switch_behavior_ofdpa::neigh_ll_created(unsigned int ifindex,
                                             uint16_t nbindex) {
  using rofcore::crtneigh;
  using rofcore::crtlink;
  using rofcore::cnetlink;
  using rofl::crofdpt;
  using rofcore::logging;

  const crtlink &rtl = cnetlink::get_instance().get_links().get_link(ifindex);
  const crtneigh &rtn =
      cnetlink::get_instance().neighs_ll[ifindex].get_neigh(nbindex);

  logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
                << "]: " << std::endl
                << rtn;

  if (bridge.has_bridge_interface()) {
    try {
      if (0 > rtn.get_vlan() || 0x1000 < rtn.get_vlan()) {
        logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                       << "]: invalid vlan" << rtn.get_vlan() << std::endl;
        return;
      }

      // XXX get dpt using cdptid
      bridge.add_mac_to_fdb(
          dpt.get_ports().get_port(rtl.get_devname()).get_port_no(),
          rtn.get_vlan(), rtn.get_lladdr(), true);
    } catch (rofl::openflow::ePortsNotFound &e) {
      logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                     << "ePortsNotFound: " << e.what() << std::endl;
    }
  } else {
    logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
                  << "]: no bridge interface" << std::endl;
  }
}

void switch_behavior_ofdpa::neigh_ll_updated(unsigned int ifindex,
                                             uint16_t nbindex) {
  using rofcore::cnetlink;
  using rofcore::crtneigh;
  const crtneigh &rtn =
      cnetlink::get_instance().neighs_ll[ifindex].get_neigh(nbindex);

  rofcore::logging::warn << "[switch_behavior_ofdpa][" << __FUNCTION__
                         << "]: NOT handled neighbor:" << std::endl
                         << rtn;
}

void switch_behavior_ofdpa::neigh_ll_deleted(unsigned int ifindex,
                                             uint16_t nbindex) {
  using rofcore::crtneigh;
  using rofcore::crtlink;
  using rofcore::cnetlink;
  using rofl::crofdpt;
  using rofcore::logging;

  const crtlink &rtl = cnetlink::get_instance().get_links().get_link(ifindex);
  const crtneigh &rtn =
      cnetlink::get_instance().neighs_ll[ifindex].get_neigh(nbindex);

  logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
                << "]: " << std::endl
                << rtn;

  if (bridge.has_bridge_interface()) {
    try {
      // XXX get dpt using cdptid
      bridge.remove_mac_from_fdb(
          dpt.get_ports().get_port(rtl.get_devname()).get_port_no(),
          rtn.get_vlan(), rtn.get_lladdr());
    } catch (rofl::openflow::ePortsNotFound &e) {
      logging::error << "[switch_behavior_ofdpa][" << __FUNCTION__
                     << "]: invalid port:" << e.what() << std::endl;
    }
  } else {
    logging::info << "[switch_behavior_ofdpa][" << __FUNCTION__
                  << "]: no bridge interface" << std::endl;
  }
}

} /* namespace basebox */
