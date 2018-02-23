/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cassert>
#include <cerrno>
#include <cstring>
#include <linux/if_ether.h>
#include <stdio.h>

#include <grpc++/grpc++.h>

#include "controller.hpp"
#include "ofdpa_client.hpp"
#include "ofdpa_datatypes.h"
#include "utils.hpp"

namespace basebox {

struct vlan_hdr {
  struct ethhdr eth; // vid + cfi + pcp
  uint16_t vlan;     // ethernet type
} __attribute__((packed));

void controller::handle_dpt_open(rofl::crofdpt &dpt) {

  std::lock_guard<std::mutex> lock(conn_mutex);
  dptid = dpt.get_dptid();

  LOG(INFO) << __FUNCTION__ << ": opening connection to dptid=0x" << std::hex
            << dptid << std::dec;

  if (rofl::openflow13::OFP_VERSION != dpt.get_version()) {
    LOG(ERROR) << __FUNCTION__
               << ": datapath attached with invalid OpenFlow protocol version: "
               << (int)dpt.get_version();
    return;
  }

  // set max queue size in rofl
  dpt.set_conn(rofl::cauxid(0)).set_txqueue_max_size(128 * 1024);

  rofl::csockaddr raddr = dpt.set_conn(rofl::cauxid(0)).get_raddr();
  std::string buf;

  switch (raddr.get_family()) {
  case AF_INET: {
    rofl::caddress_in4 addr;
    addr.set_addr_nbo(raddr.ca_s4addr->sin_addr.s_addr);
    buf = addr.str();
  } break;
  case AF_INET6: {
    rofl::caddress_in6 addr;
    addr.unpack(raddr.ca_s6addr->sin6_addr.s6_addr, 16);
    buf = addr.str();
  } break;
  default:
    LOG(FATAL) << __FUNCTION__ << ": invalid socket address " << raddr;
  }

  std::string remote = buf + ":50060";

  VLOG(1) << __FUNCTION__ << ": remote=" << remote;

  std::shared_ptr<grpc::Channel> chan =
      grpc::CreateChannel(remote, grpc::InsecureChannelCredentials());

  ofdpa = std::make_shared<ofdpa_client>(chan);

  // open connection already
  chan->GetState(true);

  dpt.send_features_request(rofl::cauxid(0));
  dpt.send_desc_stats_request(rofl::cauxid(0), 0);
  dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);
}

void controller::handle_dpt_close(const rofl::cdptid &dptid) {
  std::lock_guard<std::mutex> lock(conn_mutex);

  LOG(INFO) << __FUNCTION__ << ": closing connection to dptid=0x" << std::hex
            << dptid << std::dec;

  std::deque<nbi::port_notification_data> ntfys;
  try {
    {
      std::lock_guard<std::mutex> lock(l2_domain_mutex);
      // clear local state, all ports are gone
      l2_domain.clear();
    }

    // TODO check dptid and dptid?
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    for (auto &id : dpt.get_ports().keys()) {
      auto &port = dpt.get_ports().get_port(id);
      ntfys.emplace_back(nbi::port_notification_data{
          nbi::PORT_EVENT_DEL, port.get_port_no(), port.get_name()});
    }

    nb->port_notification(ntfys);

  } catch (rofl::eRofDptNotFound &e) {
    LOG(ERROR) << __FUNCTION__
               << ": no data path attached, dropping outgoing packet";
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": " << e.what();
  } catch (rofl::openflow::ePortsNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": invalid port for packet out";
  }

  this->dptid = rofl::cdptid(0);
}

void controller::handle_conn_terminated(rofl::crofdpt &dpt,
                                        const rofl::cauxid &auxid) {
  LOG(WARNING) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_refused(rofl::crofdpt &dpt,
                                     const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_failed(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_negotiation_failed(rofl::crofdpt &dpt,
                                                const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_congestion_occurred(rofl::crofdpt &dpt,
                                                 const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_congestion_solved(rofl::crofdpt &dpt,
                                               const rofl::cauxid &auxid) {
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_features_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_features_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid() << std::endl << msg;
}

void controller::handle_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_desc_stats_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << std::endl << dpt << std::endl << msg;
}

void controller::handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                  rofl::openflow::cofmsg_packet_in &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

#if 0 // XXX FIXME check if needed
  if (dptid != dpt) {
    LOG(ERROR) << __FUNCTION__
                   << "] wrong dptid received";
    return;
  }
#endif

  switch (msg.get_table_id()) {
  case OFDPA_FLOW_TABLE_ID_SA_LOOKUP:
    handle_srcmac_table(dpt, msg);
    break;

  case OFDPA_FLOW_TABLE_ID_UNICAST_ROUTING:
  case OFDPA_FLOW_TABLE_ID_ACL_POLICY:
    send_packet_in_to_cpu(dpt, msg);
    break;
  default:
    LOG(WARNING) << __FUNCTION__ << ": unexpected packet-in from table "
                 << msg.get_table_id() << ". Dropping packet.";
    break;
  }
}

void controller::handle_flow_removed(rofl::crofdpt &dpt,
                                     const rofl::cauxid &auxid,
                                     rofl::openflow::cofmsg_flow_removed &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

#if 0 // XXX FIXME check if needed
  if (dptid != dpt) {
    LOG(ERROR) << __FUNCTION__
                   << "] wrong dptid received";
    return;
  }
#endif

  switch (msg.get_table_id()) {
  case OFDPA_FLOW_TABLE_ID_BRIDGING:
    handle_bridging_table_rm(dpt, msg);
    break;
  default:
    break;
  }
}

void controller::handle_port_status(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_port_status &msg) {
  using basebox::nbi;
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

  std::deque<nbi::port_notification_data> ntfys;
  uint32_t port_no = msg.get_port().get_port_no();

  enum nbi::port_status status = (nbi::port_status)0;
  if (msg.get_port().get_config() & rofl::openflow13::OFPPC_PORT_DOWN) {
    status = (nbi::port_status)(status | nbi::PORT_STATUS_ADMIN_DOWN);
  }

  if (msg.get_port().get_state() & rofl::openflow13::OFPPS_LINK_DOWN) {
    status = (nbi::port_status)(status | nbi::PORT_STATUS_LOWER_DOWN);
  }

  switch (msg.get_reason()) {
  case rofl::openflow::OFPPR_MODIFY: {

    try {
      nb->port_status_changed(port_no, status);
    } catch (std::out_of_range &e) {
      LOG(WARNING) << __FUNCTION__
                   << ": unknown port with OF portno=" << port_no;
    }
  } break;
  case rofl::openflow::OFPPR_ADD:
    ntfys.emplace_back(nbi::port_notification_data{nbi::PORT_EVENT_ADD, port_no,
                                                   msg.get_port().get_name()});
    nb->port_notification(ntfys);
    nb->port_status_changed(port_no, status);
    break;
  case rofl::openflow::OFPPR_DELETE:
    ntfys.emplace_back(nbi::port_notification_data{nbi::PORT_EVENT_DEL, port_no,
                                                   msg.get_port().get_name()});
    nb->port_notification(ntfys);
    break;
  default:
    LOG(ERROR) << __FUNCTION__ << ": invalid port status";
    break;
  }
}

void controller::handle_error_message(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid,
                                      rofl::openflow::cofmsg_error &msg) {
  LOG(INFO) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
            << " pkt received: " << std::endl
            << msg;

  LOG(WARNING) << __FUNCTION__ << ": not implemented";
}

void controller::handle_port_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_port_desc_stats_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

  using basebox::nbi;
  using rofl::openflow::cofport;

  std::deque<struct nbi::port_notification_data> notifications;
  std::deque<std::pair<uint32_t, enum nbi::port_status>> stats;
  for (auto i : msg.get_ports().keys()) {
    const cofport &port = msg.get_ports().get_port(i);
    notifications.emplace_back(nbi::port_notification_data{
        nbi::PORT_EVENT_ADD, port.get_port_no(), port.get_name()});

    enum nbi::port_status status = (nbi::port_status)0;
    if (port.get_config() & rofl::openflow13::OFPPC_PORT_DOWN) {
      status = (nbi::port_status)(status | nbi::PORT_STATUS_ADMIN_DOWN);
    }

    if (port.get_state() & rofl::openflow13::OFPPS_LINK_DOWN) {
      status = (nbi::port_status)(status | nbi::PORT_STATUS_LOWER_DOWN);
    }
    stats.emplace_back(port.get_port_no(), status);
  }

  /* init 1:1 port mapping */
  try {
    nb->port_notification(notifications);

    for (auto status : stats) {
      nb->port_status_changed(status.first, status.second);
    }
    LOG(INFO) << "ports initialized";

    bb_thread.add_timer(
        TIMER_port_stats_request,
        rofl::ctimespec().expire_in(port_stats_request_interval));

  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": unknown error " << e.what();
  }
}

void controller::handle_port_desc_stats_reply_timeout(rofl::crofdpt &dpt,
                                                      uint32_t xid) {
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid();

  LOG(WARNING) << ": not implemented";
}

void controller::handle_experimenter_message(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_experimenter &msg) {
  VLOG(1) << __FUNCTION__ << ": dpid: " << dpt.get_dpid()
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
      nb->resend_state();
      break;
    }
  }
}

void controller::request_port_stats() {
  rofl::crofdpt &dpt = set_dpt(dptid, true);
  const uint16_t stats_flags = 0;
  const int timeout_in_secs = 3;
  uint32_t xid = 0;
  rofl::openflow::cofport_stats_request request(
      dpt.get_version(), rofl::openflow13::OFPP_ANY); // request for all ports

  dpt.send_port_stats_request(rofl::cauxid(0), stats_flags, request,
                              timeout_in_secs, &xid);
  VLOG(3) << __FUNCTION__ << " sent, xid=" << xid;
}

void controller::handle_port_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_port_stats_reply &msg) {
  VLOG(3) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

  std::lock_guard<std::mutex> lock(stats_mutex);
  stats_array = msg.get_port_stats_array();
}

void controller::handle_timeout(rofl::cthread &thread, uint32_t timer_id) {
  try {
    switch (timer_id) {
    case TIMER_port_stats_request:
      thread.add_timer(
          TIMER_port_stats_request,
          rofl::ctimespec().expire_in(port_stats_request_interval));
      request_port_stats();
      break;
    default:
      rofl::crofbase::handle_timeout(thread, timer_id);
      break;
    }
  } catch (std::exception &error) {
    LOG(ERROR) << "Exception for timer_id: " << timer_id << " caught "
               << error.what();
  }
}

void controller::handle_srcmac_table(rofl::crofdpt &dpt,
                                     rofl::openflow::cofmsg_packet_in &msg) {
#if 0 // XXX FIXME currently disabled
  using rofl::openflow::cofport;
  using basebox::cnetlink;
  using basebox::crtlink;
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
  } catch (basebox::eNetLinkNotFound &e) {
    LOG(INFO) << __FUNCTION__ << ": cannot add neighbor to interface"
                   ;
  } catch (basebox::eNetLinkFailed &e) {
    LOG(ERROR) << __FUNCTION__ << ": netlink failed";
  }
#endif
  LOG(WARNING) << ": not implemented";
}

void controller::send_packet_in_to_cpu(rofl::crofdpt &dpt,
                                       rofl::openflow::cofmsg_packet_in &msg) {
  using rofl::openflow::cofport;

  basebox::packet *pkt = nullptr;
  try {
    const cofport &port =
        dpt.get_ports().get_port(msg.get_match().get_in_port());
    const rofl::cpacket &pkt_in = msg.get_packet();

    if (pkt_in.length() >= basebox::packet_data_len) {
      return;
    }

    pkt = (basebox::packet *)std::malloc(sizeof(basebox::packet));

    if (pkt == nullptr)
      return;

    std::memcpy(pkt->data, pkt_in.soframe(), pkt_in.length());
    pkt->len = pkt_in.length();

    nb->enqueue(port.get_port_no(), pkt);

  } catch (std::out_of_range &e) {
    LOG(ERROR) << __FUNCTION__ << ": invalid range";
    std::free(pkt);
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " exception: " << e.what();
    std::free(pkt);
  }
}

void controller::handle_bridging_table_rm(
    rofl::crofdpt &dpt, rofl::openflow::cofmsg_flow_removed &msg) {
#if 0 // XXX FIXME disabled for tapdev refactoring:
// this is used only for srcmac learning, which is disabled
  using rofl::cmacaddr;
  using rofl::openflow::cofport;
  using basebox::cnetlink;
  using basebox::ctapdev;

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
  const ctapdev &tapdev = get_tap_dev(dpt, port.get_name());

  try {
    // update bridge fdb
    cnetlink::get_instance().drop_neigh_ll(tapdev.get_ifindex(), vlan,
    eth_dst);
  } catch (basebox::eNetLinkFailed &e) {
    LOG(ERROR) << __FUNCTION__ << ": netlink failed: " << e.what()
                 ;
  }
#endif
  LOG(WARNING) << ": not implemented";
}

int controller::enqueue(uint32_t port_id, basebox::packet *pkt) noexcept {
  using rofl::openflow::cofport;
  using std::map;
  int rv = 0;

  assert(pkt && "invalid enque");
  struct ethhdr *eth = (struct ethhdr *)pkt->data;

  if (eth->h_dest[0] == 0x33 && eth->h_dest[1] == 0x33) {
    VLOG(3) << __FUNCTION__ << ": drop multicast packet";
    rv = -ENOTSUP;
    goto errout;
  }

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (not dpt.is_established()) {
      LOG(WARNING) << __FUNCTION__ << " not connected, dropping packet";
      rv = -ENOTCONN;
      goto errout;
    }

    /* only send packet-out if the port with port_id is actually existing */
    if (dpt.get_ports().has_port(port_id)) {

      if (VLOG_IS_ON(3)) {
        char src_mac[32];
        char dst_mac[32];

        snprintf(dst_mac, sizeof(dst_mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3],
                 eth->h_dest[4], eth->h_dest[5]);
        snprintf(src_mac, sizeof(src_mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 eth->h_source[0], eth->h_source[1], eth->h_source[2],
                 eth->h_source[3], eth->h_source[4], eth->h_source[5]);
        LOG(INFO) << __FUNCTION__ << ": send packet out to port_id=" << port_id
                  << " pkg.len=" << pkt->len
                  << " eth.dst=" << std::string(dst_mac)
                  << " eth.src=" << std::string(src_mac)
                  << " called from tid=" << pthread_self();
      }

      rofl::openflow::cofactions actions(dpt.get_version());
      actions.set_action_output(rofl::cindex(0)).set_port_no(port_id);

      dpt.send_packet_out_message(
          rofl::cauxid(0),
          rofl::openflow::base::get_ofp_no_buffer(dpt.get_version()),
          rofl::openflow::base::get_ofpp_controller_port(dpt.get_version()),
          actions, (uint8_t *)pkt->data, pkt->len);
    } else {
      LOG(ERROR) << __FUNCTION__ << ": packet sent to invalid port_id "
                 << port_id;
    }
  } catch (rofl::eRofDptNotFound &e) {
    LOG(ERROR) << __FUNCTION__
               << ": no data path attached, dropping outgoing packet";
    rv = -ENOTCONN;
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": " << e.what();
    rv = -EINVAL;
  } catch (rofl::openflow::ePortsNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": invalid port for packet out";
    rv = -EINVAL;
  }

errout:

  std::free(pkt);
  return rv;
}

int controller::overlay_tunnel_add(uint32_t tunnel_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_overlay_tunnel(dpt.get_version(), tunnel_id));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::l2_addr_remove_all_in_vlan(uint32_t port,
                                           uint16_t vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.remove_bridging_unicast_vlan_all(
                                  dpt.get_version(), port, vid));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::l2_addr_add(uint32_t port, uint16_t vid,
                            const rofl::cmacaddr &mac, bool filtered) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    // XXX have the knowlege here about filtered/unfiltered?
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.add_bridging_unicast_vlan(dpt.get_version(), port, vid, mac,
                                            true, filtered));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::l2_addr_remove(uint32_t port, uint16_t vid,
                               const rofl::cmacaddr &mac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.remove_bridging_unicast_vlan(
                                  dpt.get_version(), port, vid, mac));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::l3_termination_add(uint32_t sport, uint16_t vid,
                                   const rofl::cmacaddr &dmac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_tmac_ipv4_unicast_mac(
                                  dpt.get_version(), sport, vid, dmac));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }

  return rv;
}

int controller::l3_termination_remove(uint32_t sport, uint16_t vid,
                                      const rofl::cmacaddr &dmac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.disable_tmac_ipv4_unicast_mac(
                                  dpt.get_version(), sport, vid, dmac));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }

  return rv;
}

int controller::l3_egress_create(uint32_t port, uint16_t vid,
                                 const rofl::caddress_ll &src_mac,
                                 const rofl::caddress_ll &dst_mac,
                                 uint32_t *l3_interface_id) noexcept {
  int rv = 0;
  uint32_t _egress_interface_id;
  bool increment = false;

  if (freed_egress_interfaces_ids.size()) {
    _egress_interface_id = *freed_egress_interfaces_ids.begin();
  } else {
    _egress_interface_id = egress_interface_id;
    increment = true;
  }

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    // TODO unfiltered interface
    dpt.send_group_mod_message(rofl::cauxid(0),
                               fm_driver.enable_group_l3_unicast(
                                   dpt.get_version(), _egress_interface_id,
                                   src_mac, dst_mac,
                                   fm_driver.group_id_l2_interface(port, vid)));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }

  if (increment) {
    egress_interface_id++;
  } else {
    freed_egress_interfaces_ids.erase(freed_egress_interfaces_ids.begin());
  }

  *l3_interface_id = _egress_interface_id;
  return rv;
}

int controller::l3_egress_remove(uint32_t l3_interface_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_group_l3_unicast(dpt.get_version(), l3_interface_id));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }

  if (l3_interface_id == egress_interface_id + 1) {
    egress_interface_id--;
    // TODO free even more ids from set?
  } else {
    freed_egress_interfaces_ids.insert(l3_interface_id);
  }

  return rv;
}

int controller::l3_unicast_host_add(const rofl::caddress_in4 &ipv4_dst,
                                    uint32_t l3_interface_id) noexcept {
  int rv = 0;

  if (l3_interface_id > 0x0fffffff)
    return -EINVAL;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    if (l3_interface_id)
      l3_interface_id = fm_driver.group_id_l3_unicast(l3_interface_id);

    dpt.send_flow_mod_message(
        rofl::cauxid(0), fm_driver.enable_ipv4_unicast_host(
                             dpt.get_version(), ipv4_dst, l3_interface_id));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }

  return rv;
}

int controller::l3_unicast_host_remove(
    const rofl::caddress_in4 &ipv4_dst) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_ipv4_unicast_host(dpt.get_version(), ipv4_dst));
    dpt.send_barrier_request(rofl::cauxid(0));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }

  return rv;
}

int controller::ingress_port_vlan_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_port_vid_allow_all(dpt.get_version(), port));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::ingress_port_vlan_drop_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_port_vid_allow_all(dpt.get_version(), port));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::ingress_port_vlan_add(uint32_t port, uint16_t vid,
                                      bool pvid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (pvid) {
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.enable_port_vid_ingress(dpt.get_version(), port, vid));
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.enable_port_pvid_ingress(dpt.get_version(), port, vid));
    } else {
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.enable_port_vid_ingress(dpt.get_version(), port, vid));
    }
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::ingress_port_vlan_remove(uint32_t port, uint16_t vid,
                                         bool pvid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (pvid) {
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.disable_port_pvid_ingress(dpt.get_version(), port, vid));
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.disable_port_vid_ingress(dpt.get_version(), port, vid));
    } else {
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.disable_port_vid_ingress(dpt.get_version(), port, vid));
    }
    dpt.send_barrier_request(rofl::cauxid(0));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::egress_port_vlan_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_group_mod_message(rofl::cauxid(0),
                               fm_driver.enable_group_l2_unfiltered_interface(
                                   dpt.get_version(), port));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::egress_port_vlan_drop_accept_all(uint32_t port) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_group_mod_message(rofl::cauxid(0),
                               fm_driver.disable_group_l2_unfiltered_interface(
                                   dpt.get_version(), port));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::egress_port_vlan_add(uint32_t port, uint16_t vid,
                                     bool untagged) noexcept {
  int rv = 0;
  try {
    // create filtered egress interface
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    rofl::openflow::cofgroupmod gm = fm_driver.enable_group_l2_interface(
        dpt.get_version(), port, vid, untagged);
    dpt.send_group_mod_message(rofl::cauxid(0), gm);
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::egress_port_vlan_remove(uint32_t port, uint16_t vid) noexcept {
  int rv = 0;
  try {
    // remove filtered egress interface
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_group_l2_interface(dpt.get_version(), port, vid));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::egress_bridge_port_vlan_add(uint32_t port, uint16_t vid,
                                            bool untagged) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    std::set<uint32_t> l2_dom_set;

    // create filtered egress interface
    rv = egress_port_vlan_add(port, vid, untagged);
    if (rv < 0)
      return rv;
    dpt.send_barrier_request(rofl::cauxid(0));

    {
      std::lock_guard<std::mutex> lock(l2_domain_mutex);
      // get set of group_ids for this vid
      auto l2_dom_it = l2_domain.find(vid);
      if (l2_dom_it == l2_domain.end()) {
        std::set<uint32_t> empty_set;
        l2_dom_it = l2_domain.emplace(vid, empty_set).first;
      }

      // insert group_id to set
      l2_dom_it->second.insert(fm_driver.group_id_l2_interface(port, vid));
      l2_dom_set = l2_dom_it->second; // copy set to minimize lock
    }

    // create/update new L2 flooding group
    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_group_l2_flood(dpt.get_version(), vid, vid, l2_dom_set,
                                        (l2_dom_set.size() != 1)));

    if (l2_dom_set.size() == 1) { // send barrier + DLF on creation
      dpt.send_barrier_request(rofl::cauxid(0));
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.add_bridging_dlf_vlan(
              dpt.get_version(), vid, fm_driver.group_id_l2_flood(vid, vid)));
    }
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::egress_bridge_port_vlan_remove(uint32_t port,
                                               uint16_t vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    std::set<uint32_t> l2_dom_set;

    {
      std::lock_guard<std::mutex> lock(l2_domain_mutex);
      // get set of group_ids for this vid
      auto l2_dom_it = l2_domain.find(vid);
      if (l2_dom_it == l2_domain.end()) {
        // vid not present
        return 0;
      }

      // remove group_id from set
      l2_dom_it->second.erase(fm_driver.group_id_l2_interface(port, vid));
      l2_dom_set = l2_dom_it->second; // copy set to minimize lock
    }

    if (l2_dom_set.size()) {
      // update L2 flooding group
      dpt.send_group_mod_message(
          rofl::cauxid(0), fm_driver.enable_group_l2_flood(
                               dpt.get_version(), vid, vid, l2_dom_set, true));
    } else {
      // remove DLF + L2 flooding group
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.remove_bridging_dlf_vlan(dpt.get_version(), vid));
      dpt.send_barrier_request(rofl::cauxid(0));
      dpt.send_group_mod_message(
          rofl::cauxid(0),
          fm_driver.disable_group_l2_flood(dpt.get_version(), vid, vid));
    }

    dpt.send_barrier_request(rofl::cauxid(0));

    // remove filtered egress interface
    rv = egress_port_vlan_remove(port, vid);
    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_group_l2_interface(dpt.get_version(), port, vid));

    if (rv < 0) {
      LOG(ERROR) << __FUNCTION__ << ": failed to remove vid=" << vid
                 << " from port=" << port;
    }

  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::subscribe_to(enum swi_flags flags) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (flags & switch_interface::SWIF_ARP) {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.enable_policy_arp(dpt.get_version()));
    }
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound";
    rv = -EINVAL;
  } catch (rofl::eRofConnNotConnected &e) {
    LOG(ERROR) << ": not connected msg=" << e.what();
    rv = -ENOTCONN;
  } catch (std::exception &e) {
    LOG(ERROR) << ": caught unknown exception: " << e.what();
    rv = -EINVAL;
  }
  return rv;
}

int controller::get_statistics(uint64_t port_no, uint32_t number_of_counters,
                               const sai_port_stat_t *counter_ids,
                               uint64_t *counters) noexcept {
  int rv = 0;

  if (!(counter_ids && counters))
    return -1;

  if (!port_no)
    return -1;

  std::lock_guard<std::mutex> lock(stats_mutex);

  if (!stats_array.has_port_stats(port_no))
    return -1;

  for (uint32_t i = 0; i < number_of_counters; i++) {
    switch (counter_ids[i]) {
    case (SAI_PORT_STAT_RX_PACKETS):
      counters[i] = stats_array.get_port_stats(port_no).get_rx_packets();
      break;
    case (SAI_PORT_STAT_TX_PACKETS):
      counters[i] = stats_array.get_port_stats(port_no).get_tx_packets();
      break;
    case (SAI_PORT_STAT_RX_BYTES):
      counters[i] = stats_array.get_port_stats(port_no).get_rx_bytes();
      break;
    case (SAI_PORT_STAT_TX_BYTES):
      counters[i] = stats_array.get_port_stats(port_no).get_tx_bytes();
      break;
    case (SAI_PORT_STAT_RX_DROPPED):
      counters[i] = stats_array.get_port_stats(port_no).get_rx_dropped();
      break;
    case (SAI_PORT_STAT_TX_DROPPED):
      counters[i] = stats_array.get_port_stats(port_no).get_tx_dropped();
      break;
    case (SAI_PORT_STAT_RX_ERRORS):
      counters[i] = stats_array.get_port_stats(port_no).get_rx_errors();
      break;
    case (SAI_PORT_STAT_TX_ERRORS):
      counters[i] = stats_array.get_port_stats(port_no).get_tx_errors();
      break;
    case (SAI_PORT_STAT_RX_FRAME_ERR):
      counters[i] = stats_array.get_port_stats(port_no).get_rx_frame_err();
      break;
    case (SAI_PORT_STAT_RX_OVER_ERR):
      counters[i] = stats_array.get_port_stats(port_no).get_rx_over_err();
      break;
    case (SAI_PORT_STAT_RX_CRC_ERR):
      counters[i] = stats_array.get_port_stats(port_no).get_rx_crc_err();
      break;
    case (SAI_PORT_STAT_COLLISIONS):
      counters[i] = stats_array.get_port_stats(port_no).get_collisions();
      break;
    }
  }
  return rv;
}

int controller::tunnel_tenant_create(uint32_t tunnel_id,
                                     uint32_t vni) noexcept {
  return ofdpa->ofdpaTunnelTenantCreate(tunnel_id, vni);
}

int controller::tunnel_next_hop_create(uint32_t next_hop_id, uint64_t src_mac,
                                       uint64_t dst_mac, uint32_t physical_port,
                                       uint16_t vlan_id) noexcept {
  return ofdpa->ofdpaTunnelNextHopCreate(next_hop_id, src_mac, dst_mac,
                                         physical_port, vlan_id);
}

int controller::tunnel_access_port_create(uint32_t port_id,
                                          const std::string &port_name,
                                          uint32_t physical_port,
                                          uint16_t vlan_id) noexcept {
  return ofdpa->ofdpaTunnelAccessPortCreate(port_id, port_name, physical_port,
                                            vlan_id);
}

int controller::tunnel_enpoint_create(
    uint32_t port_id, const std::string &port_name, uint32_t remote_ipv4,
    uint32_t local_ipv4, uint32_t ttl, uint32_t next_hop_id,
    uint32_t terminator_udp_dst_port, uint32_t initiator_udp_dst_port,
    uint32_t udp_src_port_if_no_entropy, bool use_entropy) noexcept {
  return ofdpa->ofdpaTunnelEndpointPortCreate(
      port_id, port_name, remote_ipv4, local_ipv4, ttl, next_hop_id,
      terminator_udp_dst_port, initiator_udp_dst_port,
      udp_src_port_if_no_entropy, use_entropy);
}
int controller::tunnel_port_tenant_add(uint32_t port_id,
                                       uint32_t tunnel_id) noexcept {
  return ofdpa->ofdpaTunnelPortTenantAdd(port_id, tunnel_id);
}

} // namespace basebox
