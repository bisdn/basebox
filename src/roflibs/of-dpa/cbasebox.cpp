/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <cerrno>
#include <linux/if_ether.h>

#include "cbasebox.hpp"

#include "roflibs/netlink/cpacketpool.hpp"
#include "roflibs/of-dpa/ofdpa_datatypes.hpp"

namespace basebox {

struct vlan_hdr {
  struct ethhdr eth; // vid + cfi + pcp
  uint16_t vlan;     // ethernet type
} __attribute__((packed));

/*static*/ bool cbasebox::keep_on_running = true;

void cbasebox::handle_dpt_open(rofl::crofdpt &dpt) {

  if (rofl::openflow13::OFP_VERSION < dpt.get_version()) {
    LOG(ERROR) << __FUNCTION__ << "] datapath "
               << "attached with invalid OpenFlow protocol version: "
               << (int)dpt.get_version();
    return;
  }

#ifdef DEBUG
  dpt.set_conn(rofl::cauxid(0))
      .set_trace(true)
      .set_journal()
      .log_on_stderr(true)
      .set_max_entries(64);
  dpt.set_conn(rofl::cauxid(0))
      .set_tcp_journal()
      .log_on_stderr(true)
      .set_max_entries(16);
#endif

  VLOG(1) << __FUNCTION__ << "] dpid: " << dpt.get_dpid().str()
          << " dpt: " << dpt;

  dpt.send_features_request(rofl::cauxid(0));
  dpt.send_desc_stats_request(rofl::cauxid(0), 0);
  dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);

  // todo timeout?
}

void cbasebox::handle_wakeup(rofl::cthread &thread) {
  LOG(WARNING) << __FUNCTION__ << "] XXX not implemented";
}

void cbasebox::handle_dpt_close(const rofl::cdptid &dptid) {
  LOG(INFO) << __FUNCTION__ << "] dptid: " << dptid.str();
}

void cbasebox::handle_conn_terminated(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid) {
  LOG(WARNING) << __FUNCTION__ << ": XXX not implemented";
}

void cbasebox::handle_conn_refused(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void cbasebox::handle_conn_failed(rofl::crofdpt &dpt,
                                  const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void cbasebox::handle_conn_negotiation_failed(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void cbasebox::handle_conn_congestion_occured(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void cbasebox::handle_conn_congestion_solved(rofl::crofdpt &dpt,
                                             const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void cbasebox::handle_features_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_features_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid().str() << std::endl
          << msg;
}

void cbasebox::handle_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_desc_stats_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << std::endl << dpt << std::endl << msg;
}

void cbasebox::handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                rofl::openflow::cofmsg_packet_in &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid().str()
          << " pkt received: " << std::endl
          << msg;

  VLOG(1) << __FUNCTION__ << ": handle message" << std::endl << msg;

#if 0 // XXX FIXME check if needed
  if (this->dptid != dpt.get_dptid()) {
    LOG(ERROR) << __FUNCTION__
                   << "] wrong dptid received";
    return;
  }
#endif

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

void cbasebox::handle_flow_removed(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid,
                                   rofl::openflow::cofmsg_flow_removed &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid().str()
          << " pkt received: " << std::endl
          << msg;

#if 0 // XXX FIXME check if needed
  if (this->dptid != dpt.get_dptid()) {
    LOG(ERROR) << __FUNCTION__
                   << "] wrong dptid received";
    return;
  }
#endif

  switch (msg.get_table_id()) {
  case OFDPA_FLOW_TABLE_ID_BRIDGING:
    this->handle_bridging_table_rm(dpt, msg);
    break;
  default:
    break;
  }
}

void cbasebox::handle_port_status(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                  rofl::openflow::cofmsg_port_status &msg) {
  using rofcore::nbi;
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid().str()
          << " pkt received: " << std::endl
          << msg;

  switch (msg.get_reason()) {
  case rofl::openflow::OFPPR_MODIFY: {
    enum nbi::port_status status = (nbi::port_status)0;
    if (msg.get_port().get_config() & rofl::openflow13::OFPPC_PORT_DOWN) {
      status = (nbi::port_status)(status | nbi::PORT_STATUS_ADMIN_DOWN);
    }

    if (msg.get_port().get_state() & rofl::openflow13::OFPPS_LINK_DOWN) {
      status = (nbi::port_status)(status | nbi::PORT_STATUS_LOWER_DOWN);
    }

    try {
      this->nbi->port_status_changed(
          of_port_to_port_id.at(msg.get_port().get_port_no()), status);
    } catch (std::out_of_range &e) {
      LOG(WARNING) << __FUNCTION__ << ": unknown port with OF portno="
                   << msg.get_port().get_port_no();
    }
  } break;
  case rofl::openflow::OFPPR_ADD:
  case rofl::openflow::OFPPR_DELETE:
    LOG(WARNING) << __FUNCTION__ << ": not implemented";
    break;
  default:
    LOG(ERROR) << __FUNCTION__ << ": invalid port status";
    break;
  }
}

void cbasebox::handle_error_message(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_error &msg) {
  LOG(INFO) << __FUNCTION__ << ": dpid=" << dpt.get_dpid().str()
            << " pkt received: " << std::endl
            << msg;

  // XXX FIXME not implemented
  LOG(WARNING) << __FUNCTION__ << ": not implemented";
}

void cbasebox::handle_port_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_port_desc_stats_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid().str()
          << " pkt received: " << std::endl
          << msg;
  init(dpt);
}

void cbasebox::handle_port_desc_stats_reply_timeout(rofl::crofdpt &dpt,
                                                    uint32_t xid) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid().str();

  LOG(WARNING) << ": not implemented";
}

void cbasebox::handle_experimenter_message(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_experimenter &msg) {
  VLOG(1) << __FUNCTION__ << "] dpid: " << dpt.get_dpid().str()
          << " pkt received: " << std::endl
          << msg;

  uint32_t experimenterId = msg.get_exp_id();
  uint32_t experimenterType = msg.get_exp_type();
  uint32_t xidExperimenterCAR = msg.get_xid();

  if (experimenterId == BISDN) {
    switch (experimenterType) {
    case QUERY_FLOW_ENTRIES:
      dpt.send_experimenter_message(auxid, xidExperimenterCAR, experimenterId,
                                    RECEIVED_FLOW_ENTRIES_QUERY);
      nbi->resend_state();
      break;
    }
  }
}

void cbasebox::handle_srcmac_table(rofl::crofdpt &dpt,
                                   rofl::openflow::cofmsg_packet_in &msg) {
#if 0 // XXX FIXME currently disabled
  using rofl::openflow::cofport;
  using rofcore::cnetlink;
  using rofcore::crtlink;
  using rofl::caddress_ll;

  LOG(INFO) << __FUNCTION__ << ": in_port=" << msg.get_match().get_in_port()
               ;

  struct ethhdr *eth = (struct ethhdr *)msg.get_packet().soframe();

  uint16_t vlan = 0;
  if (ETH_P_8021Q == be16toh(eth->h_proto)) {
    vlan = be16toh(((struct vlan_hdr *)eth)->vlan) & 0xfff;
    VLOG(1) << __FUNCTION__ << ": vlan=0x" << std::hex << vlan
                   << std::dec;
  }

  // TODO this has to be improved
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
    bridge.add_mac_to_fdb(dpt, msg.get_match().get_in_port(), vlan, srcmac,
                          false);
  } catch (rofcore::eNetLinkNotFound &e) {
    LOG(INFO) << __FUNCTION__ << ": cannot add neighbor to interface"
                   ;
  } catch (rofcore::eNetLinkFailed &e) {
    LOG(ERROR) << __FUNCTION__ << ": netlink failed";
  }
#endif
  LOG(WARNING) << ": not implemented";
}

void cbasebox::handle_acl_policy_table(rofl::crofdpt &dpt,
                                       rofl::openflow::cofmsg_packet_in &msg) {
  using rofl::openflow::cofport;

  try {
    const cofport &port =
        dpt.get_ports().get_port(msg.get_match().get_in_port());

    rofl::cpacket *pkt = rofcore::cpacketpool::get_instance().acquire_pkt();
    *pkt = msg.get_packet();

    tap_man->get_dev(of_port_to_port_id.at(port.get_port_no())).enqueue(pkt);
  } catch (rofcore::ePacketPoolExhausted &e) {
    LOG(ERROR) << __FUNCTION__ << " ePacketPoolExhausted: " << e.what();
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " exception: " << e.what();
  }
}

void cbasebox::handle_bridging_table_rm(
    rofl::crofdpt &dpt, rofl::openflow::cofmsg_flow_removed &msg) {
#if 0 // XXX FIXME disabled for tapdev refactoring:
// this is used only for srcmac learning, which is disabled
  using rofl::cmacaddr;
  using rofl::openflow::cofport;
  using rofcore::cnetlink;
  using rofcore::ctapdev;

  LOG(INFO) << __FUNCTION__ << ": handle message" << std::endl << msg;

  cmacaddr eth_dst;
  uint16_t vlan = 0;
  try {
    eth_dst = msg.get_match().get_eth_dst();
    vlan = msg.get_match().get_vlan_vid() & 0xfff;
  } catch (rofl::openflow::eOxmNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to get eth_dst or vlan"
                  ;
    return;
  }

  // TODO this has to be improved 
  uint32_t portno = msg.get_cookie();
  const cofport &port = dpt.get_ports().get_port(portno);
  const ctapdev &tapdev = get_tap_dev(dpt.get_dptid(), port.get_name());

  try {
    // update bridge fdb
    cnetlink::get_instance().drop_neigh_ll(tapdev.get_ifindex(), vlan,
    eth_dst);
  } catch (rofcore::eNetLinkFailed &e) {
    LOG(ERROR) << __FUNCTION__ << ": netlink failed: " << e.what()
                 ;
  }
#endif
  LOG(WARNING) << ": not implemented";
}

void cbasebox::init(rofl::crofdpt &dpt) {
  using rofl::openflow::cofport;

  std::deque<std::string> ports;

  /* init 1:1 port mapping */
  try {
    for (const auto &i : dpt.get_ports().keys()) {
      const cofport &port = dpt.get_ports().get_port(i);
      ports.push_back(port.get_name());
    }

    std::deque<std::pair<int, std::string>> devs =
        tap_man->register_tapdevs(ports, *this);

    for (auto i : devs) {
      const rofl::openflow::cofport &port = dpt.get_ports().get_port(i.second);
      of_port_to_port_id[port.get_port_no()] = i.first;
      port_id_to_of_port[i.first] = port.get_port_no();
    }

    tap_man->start();

    LOG(INFO) << "ports initialized";

  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << "] ERROR: unknown error " << e.what();
  }
}

int cbasebox::enqueue(rofcore::ctapdev *tapdev, rofl::cpacket *pkt) {
  using rofl::openflow::cofport;
  using std::map;
  int rv = 0;

  assert(tapdev && "no tapdev");
  assert(pkt && "invalid enque");
  struct ethhdr *eth = (struct ethhdr *)pkt->soframe();

  if (eth->h_dest[0] == 0x33 && eth->h_dest[1] == 0x33) {
    VLOG(1) << __FUNCTION__ << ": drop multicast packet";
    rv = -ENOTSUP;
    goto errout;
  }

  try {
    rofl::crofdpt &dpt = set_dpt(this->dptid, true);
    if (not dpt.is_established()) {
      LOG(WARNING) << __FUNCTION__ << "] not connected, dropping packet";
      rv = -ENOTCONN;
      goto errout;
    }

    // TODO move to separate function:
    uint32_t portno =
        dpt.get_ports().get_port(tapdev->get_devname()).get_port_no();

    /* only send packet-out if we can determine a port-no */
    if (portno) {
      VLOG(1) << __FUNCTION__ << ": send pkt-out, pkt:" << std::endl << *pkt;

      rofl::openflow::cofactions actions(dpt.get_version());
      //			//actions.set_action_push_vlan(rofl::cindex(0)).set_eth_type(rofl::fvlanframe::VLAN_CTAG_ETHER);
      //			//actions.set_action_set_field(rofl::cindex(1)).set_oxm(rofl::openflow::coxmatch_ofb_vlan_vid(tapdev->get_pvid()));
      actions.set_action_output(rofl::cindex(0)).set_port_no(portno);

      dpt.send_packet_out_message(
          rofl::cauxid(0),
          rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()),
          rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()),
          actions, pkt->soframe(), pkt->length());
    }
  } catch (rofl::eRofDptNotFound &e) {
    LOG(ERROR) << __FUNCTION__
               << "] no data path attached, dropping outgoing packet";

  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": " << e.what();

  } catch (rofl::openflow::ePortsNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": invalid port for packet out";
    rv = -EINVAL;
    goto errout;
  }

errout:

  rofcore::cpacketpool::get_instance().release_pkt(pkt);
  return rv;
}

int cbasebox::l2_addr_remove_all_in_vlan(uint32_t port, uint16_t vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    fm_driver.remove_bridging_unicast_vlan_all(dpt, of_port, vid);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::l2_addr_add(uint32_t port, uint16_t vid,
                          const rofl::cmacaddr &mac, bool filtered) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    // XXX have the knowlege here about filtered/unfiltered?
    uint32_t of_port = port_id_to_of_port.at(port);
    fm_driver.add_bridging_unicast_vlan(dpt, of_port, vid, mac, true, filtered);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::l2_addr_remove(uint32_t port, uint16_t vid,
                             const rofl::cmacaddr &mac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    fm_driver.remove_bridging_unicast_vlan(dpt, of_port, vid, mac);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::ingress_port_vlan_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    fm_driver.enable_port_vid_allow_all(dpt, of_port);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::ingress_port_vlan_drop_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    fm_driver.disable_port_vid_allow_all(dpt, of_port);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::ingress_port_vlan_add(uint32_t port, uint16_t vid,
                                    bool pvid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    if (pvid) {
      fm_driver.enable_port_pvid_ingress(dpt, of_port, vid);
    } else {
      fm_driver.enable_port_vid_ingress(dpt, of_port, vid);
    }
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::ingress_port_vlan_remove(uint32_t port, uint16_t vid,
                                       bool pvid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    if (pvid) {
      fm_driver.disable_port_pvid_ingress(dpt, of_port, vid);
    } else {
      fm_driver.disable_port_vid_ingress(dpt, of_port, vid);
    }
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::egress_port_vlan_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    fm_driver.enable_group_l2_unfiltered_interface(dpt, of_port);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::egress_port_vlan_drop_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    fm_driver.disable_group_l2_unfiltered_interface(dpt, of_port);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::egress_port_vlan_add(uint32_t port, uint16_t vid,
                                   bool untagged) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    // create filtered egress interface
    uint32_t of_port = port_id_to_of_port.at(port);
    uint32_t group_id =
        fm_driver.enable_group_l2_interface(dpt, of_port, vid, untagged);
    l2_domain[vid].insert(group_id);

    // remove old L2 flooding group
    fm_driver.remove_bridging_dlf_vlan(dpt, vid);
    dpt.send_barrier_request(rofl::cauxid(0));
    fm_driver.disable_group_l2_flood(dpt, vid, vid);
    dpt.send_barrier_request(rofl::cauxid(0));

    // add new L2 flooding group
    group_id = fm_driver.enable_group_l2_flood(dpt, vid, vid, l2_domain[vid]);
    dpt.send_barrier_request(rofl::cauxid(0));
    fm_driver.add_bridging_dlf_vlan(dpt, vid, group_id);
    dpt.send_barrier_request(rofl::cauxid(0));
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::egress_port_vlan_remove(uint32_t port, uint16_t vid,
                                      bool untagged) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    uint32_t of_port = port_id_to_of_port.at(port);
    uint32_t group_id = fm_driver.group_id_l2_interface(of_port, vid);
    l2_domain[vid].erase(group_id);

    // remove old L2 flooding group
    fm_driver.remove_bridging_dlf_vlan(dpt, vid);
    dpt.send_barrier_request(rofl::cauxid(0));
    fm_driver.disable_group_l2_flood(dpt, vid, vid);
    dpt.send_barrier_request(rofl::cauxid(0));

    if (l2_domain[vid].size()) {
      // add new L2 flooding group
      group_id = fm_driver.enable_group_l2_flood(dpt, vid, vid, l2_domain[vid]);
      dpt.send_barrier_request(rofl::cauxid(0));
      fm_driver.add_bridging_dlf_vlan(dpt, vid, group_id);
      dpt.send_barrier_request(rofl::cauxid(0));
    }

    // remove filtered egress interface
    fm_driver.disable_group_l2_interface(dpt, of_port, vid);
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

int cbasebox::subscribe_to(enum swi_flags flags) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (flags & switch_interface::SWIF_ARP) {
      fm_driver.enable_policy_arp(dpt, 0, -1);
    }
  } catch (rofl::eRofBaseNotFound &e) {
    // TODO log error
    rv = -EINVAL;
  }
  return rv;
}

} // namespace basebox
