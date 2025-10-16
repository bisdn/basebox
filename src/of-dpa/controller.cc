// SPDX-FileCopyrightText: © 2014 BISDN GmbH
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include <linux/if_bridge.h>
#include <linux/if_ether.h>
#include <gflags/gflags.h>
#include <grpc++/grpc++.h>
#include <systemd/sd-daemon.h>

#include "controller.h"
#include "ofdpa_client.h"
#include "ofdpa_datatypes.h"
#include "utils/utils.h"
#include "utils/rofl-utils.h"

DECLARE_bool(clear_switch_configuration);
DECLARE_bool(use_knet);
DECLARE_int32(rx_rate_limit);

namespace basebox {

void controller::handle_conn_established(rofl::crofdpt &dpt,
                                         const rofl::cauxid &auxid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
}

void controller::handle_dpt_open(rofl::crofdpt &dpt) {
  std::lock_guard<std::mutex> lock(conn_mutex);

  // Avoid reopening the connection

  dptid = dpt.get_dptid();
  auto dpid = dpt.get_dpid();

  LOG(INFO) << __FUNCTION__ << ": opening connection to dptid=" << std::showbase
            << std::hex << dptid << ", dpid=" << dpid << std::dec
            << ", n_tables=" << static_cast<unsigned>(dpt.get_n_tables())
            << ", dpt=" << &dpt;

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
    LOG(ERROR) << __FUNCTION__ << ": invalid socket address " << raddr;
    return;
  }

  std::string remote = buf + ":" + std::to_string(ofdpa_grpc_port);

  VLOG(1) << __FUNCTION__ << ": remote=" << remote;

  std::shared_ptr<grpc::Channel> chan =
      grpc::CreateChannel(remote, grpc::InsecureChannelCredentials());

  ofdpa = std::make_shared<ofdpa_client>(chan);

  // open connection already
  chan->GetState(true);

  if (FLAGS_clear_switch_configuration) {
    // first delete all flows, as they may reference groups
    dpt.flow_mod_reset();
    dpt.send_barrier_request(rofl::cauxid(0));
    // now we can delete all groups, which may reference logical ports
    dpt.group_mod_reset();
    dpt.send_barrier_request(rofl::cauxid(0));

    // now we can delete all tunnel ports, tenents and nexthops
    // LAG ports will be handled separately
    ofdpa->ofdpaTunnelReset();

    // finally reset the STG groups
    ofdpa->ofdpaStgReset();
  }

  int rate = FLAGS_rx_rate_limit;
  if (rate < 0) {
    if (FLAGS_use_knet)
      // Packets use a different channel, so set unlimited.
      rate = 0;
    else
      // TAP packets use the same channel as control traffic, so set the
      // default limit of 1024 pps (about ~11 Mbit/s) to ensure the OpenFlow
      // connection is stable.
      rate = 1024;
  }

  ofdpa->ofdpaRxRateSet(rate);

  dpt.send_features_request(rofl::cauxid(0), 1);
  dpt.send_desc_stats_request(rofl::cauxid(0), 0, 1);

  if (flags)
    subscribe_to(flags);

  connected = true;
}

void controller::handle_dpt_close(const rofl::cdptid &dptid) {
  std::lock_guard<std::mutex> lock(conn_mutex);

  LOG(INFO) << __FUNCTION__ << ": closing connection to dptid=" << std::showbase
            << std::hex << dptid;

  connected = false;
  std::deque<nbi::port_notification_data> ntfys;
  try {
    {
      std::lock_guard<std::mutex> lock(l2_domain_mutex);
      // clear local state, all ports are gone
      l2_domain.clear();
      lag.clear();
      // we keep the tunnel_dlf_flood for now
    }

    // TODO check dptid and dptid?
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    for (auto &id : dpt.get_ports().keys()) {
      auto &port = dpt.get_ports().get_port(id);
      ntfys.emplace_back(
          nbi::port_notification_data{nbi::PORT_EVENT_DEL, port.get_port_no(),
                                      port.get_hwaddr(), port.get_name()});
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

  nb->switch_state_notification(nbi::SWITCH_STATE_DOWN);
}

void controller::handle_conn_terminated(rofl::crofdpt &dpt,
                                        const rofl::cauxid &auxid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  LOG(WARNING) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_refused(rofl::crofdpt &dpt,
                                     const rofl::cauxid &auxid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_failed(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_negotiation_failed(rofl::crofdpt &dpt,
                                                const rofl::cauxid &auxid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_congestion_occurred(rofl::crofdpt &dpt,
                                                 const rofl::cauxid &auxid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_conn_congestion_solved(rofl::crofdpt &dpt,
                                               const rofl::cauxid &auxid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  LOG(ERROR) << __FUNCTION__ << ": XXX not implemented";
}

void controller::handle_features_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_features_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid
          << ", msg: " << msg;
}

void controller::handle_barrier_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_barrier_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << ", auxid=" << auxid
          << ", xid=" << std::showbase << std::hex << (unsigned)msg.get_xid();
}

void controller::handle_barrier_reply_timeout(rofl::crofdpt &dpt,
                                              uint32_t xid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << ", xid=" << std::showbase
          << std::hex << (unsigned)xid;
}

void controller::handle_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_desc_stats_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid
          << " msg: " << msg;

  // TODO evaluate switch here?

  nb->switch_state_notification(nbi::SWITCH_STATE_UP);
  dpt.send_port_desc_stats_request(rofl::cauxid(0), 0, 2);
}

void controller::handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                  rofl::openflow::cofmsg_packet_in &msg) {
  VLOG(2) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  VLOG(3) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " received: " << std::endl
          << msg;

  // all packets go up
  send_packet_in_to_cpu(dpt, msg);
}

void controller::handle_flow_removed(rofl::crofdpt &dpt,
                                     const rofl::cauxid &auxid,
                                     rofl::openflow::cofmsg_flow_removed &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  VLOG(3) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

  switch (msg.get_table_id()) {
  case OFDPA_FLOW_TABLE_ID_BRIDGING:
    handle_bridging_table_rm(msg);
    break;
  default:
    LOG(WARNING) << __FUNCTION__ << ": unhandled flow removal in table_id="
                 << unsigned(msg.get_table_id());
    break;
  }
}

void controller::handle_port_status(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_port_status &msg) {
  VLOG(2) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  VLOG(3) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

  std::deque<nbi::port_notification_data> ntfys;
  auto port = msg.get_port();

  bool status = (!(port.get_config() & rofl::openflow13::OFPPC_PORT_DOWN) &&
                 !(port.get_state() & rofl::openflow13::OFPPS_LINK_DOWN));
  uint32_t speed = port.get_ethernet().get_curr_speed();
  uint8_t duplex = get_duplex(port.get_ethernet().get_curr());

  ntfys.emplace_back(nbi::port_notification_data{
      (nbi::port_event)msg.get_reason(), port.get_port_no(), port.get_hwaddr(),
      msg.get_port().get_name(), status, speed, duplex});
  nb->port_notification(ntfys);
}

void controller::handle_error_message(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid,
                                      rofl::openflow::cofmsg_error &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;
  LOG(WARNING) << __FUNCTION__ << ": "
               << (((uint32_t)msg.get_err_type() << 16) | msg.get_err_code());
  VLOG(1) << __FUNCTION__ << msg;
}

void controller::handle_port_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_port_desc_stats_reply &msg) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
  VLOG(1) << __FUNCTION__ << ": dpid=" << dpt.get_dpid()
          << " pkt received: " << std::endl
          << msg;

  using rofl::openflow::cofport;

  std::deque<struct nbi::port_notification_data> notifications;

  for (auto i : msg.get_ports().keys()) {
    const cofport &port = msg.get_ports().get_port(i);
    uint32_t port_no = port.get_port_no();

    if (nbi::get_port_type(port_no) == nbi::port_type_lag) {
      ofdpa->OfdpaTrunkDelete(port_no);
      continue;
    }

    if (nbi::get_port_type(port_no) == nbi::port_type_vxlan) {
      // should not exist anymore - warn about?
      continue;
    }

    bool status = (!(port.get_config() & rofl::openflow13::OFPPC_PORT_DOWN) &&
                   !(port.get_state() & rofl::openflow13::OFPPS_LINK_DOWN));
    uint32_t speed = port.get_ethernet().get_curr_speed();
    uint8_t duplex = get_duplex(port.get_ethernet().get_curr());

    notifications.emplace_back(nbi::port_notification_data{
        nbi::PORT_EVENT_TABLE, port.get_port_no(), port.get_hwaddr(),
        port.get_name(), status, speed, duplex});
  }

  /* init 1:1 port mapping */
  try {
    nb->port_notification(notifications);
    LOG(INFO) << "ports initialized";

    // Let systemd know baseboxd is ready
    sd_notify(0, "READY=1\nSTATUS=Active");

    bb_thread.add_timer(
        this, TIMER_port_stats_request,
        rofl::ctimespec().expire_in(port_stats_request_interval));

  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << ": unknown error " << e.what();
  }
}

void controller::handle_port_desc_stats_reply_timeout(rofl::crofdpt &dpt,
                                                      uint32_t xid) {
  VLOG(1) << __FUNCTION__ << ": dpt=" << dpt << " xid=" << xid;

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
  VLOG(2) << __FUNCTION__ << ": dpt=" << dpt << " on auxid=" << auxid;
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
          this, TIMER_port_stats_request,
          rofl::ctimespec().expire_in(port_stats_request_interval));
      if (connected)
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

void controller::send_packet_in_to_cpu(rofl::crofdpt &dpt,
                                       rofl::openflow::cofmsg_packet_in &msg) {
  packet *pkt = nullptr;
  uint32_t port_id;

  try {
    port_id =
        dpt.get_ports().get_port(msg.get_match().get_in_port()).get_port_no();
  } catch (std::out_of_range &e) {
    LOG(ERROR) << __FUNCTION__ << ": invalid range";
    return;
  } catch (std::exception &e) {
    LOG(ERROR) << __FUNCTION__ << " exception: " << e.what();
    return;
  }

  const rofl::cpacket &pkt_in = msg.get_packet();
  pkt = (packet *)std::malloc(sizeof(std::size_t) + pkt_in.length());

  if (pkt == nullptr) {
    LOG(ERROR) << __FUNCTION__ << ": no mem left";
    return;
  }

  pkt->len = pkt_in.length();
  std::memcpy(pkt->data, pkt_in.soframe(), pkt_in.length());

  nb->enqueue(port_id, pkt);
}

void controller::handle_bridging_table_rm(
    rofl::openflow::cofmsg_flow_removed &msg) {
  rofl::caddress_ll eth_dst;
  uint16_t vid = 0;

  // we only care about flows being timed out
  if (msg.get_reason() != rofl::openflow13::OFPRR_IDLE_TIMEOUT)
    return;

  try {
    eth_dst = msg.get_match().get_eth_dst();
    vid = msg.get_match().get_vlan_vid() & 0xfff;
  } catch (rofl::openflow::eOxmNotFound &e) {
    LOG(ERROR) << __FUNCTION__ << ": failed to get eth_dst or vid";
    return;
  }

  // TODO this has to be improved -> function in rofl-ofdpa
  uint32_t port_no = msg.get_cookie() & 0xffffffff;

  nb->fdb_timeout(port_no, vid, eth_dst);
}

int controller::enqueue(uint32_t port_id, packet *pkt) noexcept {
  using rofl::openflow::cofport;
  using std::map;
  int rv = 0;

  assert(pkt && "invalid enque");
  auto *eth = (struct ethhdr *)pkt->data;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (not dpt.is_established()) {
      LOG(WARNING) << __FUNCTION__ << " not connected, dropping packet";
      rv = -ENOTCONN;
      goto errout;
    }

    // XXX TODO resolve lag port for now?

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
                 << std::showbase << std::hex << port_id;
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
int controller::sai_learn_mode_to_flags(sai_bridge_port_fdb_learning_t mode,
                                        uint32_t *flags) {
  uint32_t hw_flags;

  switch (mode) {
  case SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DROP:
    hw_flags = ofdpa_client::SRC_MAC_LEARN_NONE;
    break;
  case SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DISABLE:
    hw_flags = ofdpa_client::SRC_MAC_LEARN_FWD;
    break;
  case SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW:
    hw_flags =
        ofdpa_client::SRC_MAC_LEARN_FWD | ofdpa_client::SRC_MAC_LEARN_ARL;
    break;
  case SAI_BRIDGE_PORT_FDB_LEARNING_MODE_CPU_TRAP:
    hw_flags = ofdpa_client::SRC_MAC_LEARN_CPU;
    break;
  case SAI_BRIDGE_PORT_FDB_LEARNING_MODE_CPU_LOG:
    hw_flags =
        ofdpa_client::SRC_MAC_LEARN_FWD | ofdpa_client::SRC_MAC_LEARN_CPU;
    break;
  case SAI_BRIDGE_PORT_FDB_LEARNING_MODE_FDB_NOTIFICATION:
    hw_flags = ofdpa_client::SRC_MAC_LEARN_CPU |
               ofdpa_client::SRC_MAC_LEARN_ARL |
               ofdpa_client::SRC_MAC_LEARN_PENDING;
    break;
  case SAI_BRIDGE_PORT_FDB_LEARNING_MODE_FDB_LOG_NOTIFICATION:
    hw_flags =
        ofdpa_client::SRC_MAC_LEARN_FWD | ofdpa_client::SRC_MAC_LEARN_CPU |
        ofdpa_client::SRC_MAC_LEARN_ARL | ofdpa_client::SRC_MAC_LEARN_PENDING;
    break;
  default:
    return -EINVAL;
  }

  *flags = hw_flags;

  return 0;
}

int controller::port_set_learn(
    uint32_t port_id, sai_bridge_port_fdb_learning_t l2_learn) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }

  uint32_t flags;
  int rv = sai_learn_mode_to_flags(l2_learn, &flags);
  if (rv)
    return rv;

  return ofdpa->ofdpaPortSourceMacLearningSet(port_id, flags);
}

int controller::port_set_move_learn(
    uint32_t port_id, sai_bridge_port_fdb_learning_t l2_learn) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }

  uint32_t flags;
  int rv = sai_learn_mode_to_flags(l2_learn, &flags);
  if (rv)
    return rv;

  return ofdpa->ofdpaPortSourceMacMoveLearningSet(port_id, flags);
}

int controller::lag_create(uint32_t *lag_id, std::string name,
                           uint8_t mode) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }

  int rv = 0;

  // XXX TODO this needs to be improved, we could use the maximum number of
  // ports as lag_id and have an efficient lookup which id is free. for now
  // 2^16-1 lag ids should be sufficient.
  static uint16_t _lag_id = 1;
  std::set<uint32_t> empty;
  auto _rv = lag.emplace(std::make_pair(_lag_id, empty));

  if (!_rv.second) {
    LOG(ERROR) << __FUNCTION__ << ": maximum number of lags were created.";
    return -EINVAL;
  }

  assert(lag_id);
  *lag_id = nbi::combine_port_type(_lag_id, nbi::port_type_lag);
  _lag_id++;

  rv = ofdpa->OfdpaTrunkCreate(*lag_id, name, mode);
  return rv;
}

int controller::lag_remove(uint32_t lag_id) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }
  auto rv = lag.erase(nbi::get_port_num(lag_id));
  if (rv != 1) {
    LOG(WARNING) << __FUNCTION__ << ": rv=" << rv
                 << " entries in lag map were removed";
  }

  return ofdpa->OfdpaTrunkDelete(lag_id);
}

int controller::lag_add_member(uint32_t lag_id, uint32_t port_id) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }

  int rv = 0;
  if (nbi::get_port_type(lag_id) != nbi::port_type_lag) {
    LOG(ERROR) << __FUNCTION__ << ": invalid lag_id " << std::showbase
               << std::hex << lag_id;
    return -EINVAL;
  }

  auto it = lag.find(nbi::get_port_num(lag_id));
  if (it == lag.end()) {
    LOG(ERROR) << __FUNCTION__ << ": lag_id does not exist " << std::showbase
               << std::hex << lag_id;
    return -ENODATA;
  }

  it->second.emplace(port_id);

  rv = ofdpa->OfdpaPortTrunkGroupSet(port_id, lag_id);
  return rv;
}

int controller::lag_remove_member(uint32_t lag_id, uint32_t port_id) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }
  int rv = 0;

  if (nbi::get_port_type(lag_id) != nbi::port_type_lag) {
    LOG(ERROR) << __FUNCTION__ << ": invalid lag_id " << std::showbase
               << std::hex << lag_id;
    return -EINVAL;
  }

  auto it = lag.find(nbi::get_port_num(lag_id));
  if (it == lag.end()) {
    LOG(ERROR) << __FUNCTION__ << ": lag_id does not exist " << std::showbase
               << std::hex << lag_id;
    return -ENODATA;
  }

  size_t num_erased = it->second.erase(port_id);

  if (!num_erased) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove port_id=" << std::showbase
               << std::hex << port_id << " from lag_id=" << lag_id;
    return -EINVAL;
  }

  rv = ofdpa->OfdpaPortTrunkGroupSet(port_id, 0);
  return rv;
}

int controller::lag_set_member_active(uint32_t lag_id, uint32_t port_id,
                                      uint8_t active) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }
  int rv = 0;

  if (lag_id == 0) {
    LOG(ERROR) << __FUNCTION__ << ": invalid lag_id";
    return -EINVAL;
  }

  if (port_id == 0) {
    LOG(ERROR) << __FUNCTION__ << ": invalid port_id";
    return -EINVAL;
  }

  rv = ofdpa->OfdpaTrunkPortMemberActiveSet(port_id, lag_id, active);
  return rv;
}

int controller::lag_set_mode(uint32_t lag_id, uint8_t mode) noexcept {
  if (!connected) {
    VLOG(1) << __FUNCTION__ << ": not connected";
    return -EAGAIN;
  }
  int rv = 0;

  rv = ofdpa->ofdpaTrunkPortPSCSet(lag_id, mode);
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

int controller::overlay_tunnel_remove(uint32_t tunnel_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_overlay_tunnel(dpt.get_version(), tunnel_id));
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
int controller::l2_set_idle_timeout(uint16_t idle_timeout) noexcept {
  default_idle_timeout = idle_timeout;
  return 0;
};

int controller::l2_addr_remove_all_in_vlan(uint32_t port,
                                           uint16_t vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.remove_bridging_unicast_vlan_all(
                                  dpt.get_version(), port, vid));
    VLOG(2) << __FUNCTION__ << ": port=" << port << ", vid=" << vid;
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
                            const rofl::caddress_ll &mac, bool filtered,
                            bool permanent, bool update) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    bool lag = nbi::get_port_type(port) == nbi::port_type_lag;

    if (update) {
      // the port is part of the cookie, so we would need to update the cookie,
      // but we cannot update the cookie, so we will need to replace it with a
      // new flow entry.
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.remove_bridging_unicast_vlan(
                                    dpt.get_version(), 0, vid, mac));
      dpt.send_barrier_request(rofl::cauxid(0));
    }

    // XXX have the knowlege here about filtered/unfiltered?
    auto fm = fm_driver.add_bridging_unicast_vlan(dpt.get_version(), port, vid,
                                                  mac, filtered, lag);
    if (!permanent && default_idle_timeout > 0) {
      fm.set_idle_timeout(default_idle_timeout);
      fm.set_flags(rofl::openflow::OFPFF_SEND_FLOW_REM);
    }

    dpt.send_flow_mod_message(rofl::cauxid(0), fm);

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
                               const rofl::caddress_ll &mac) noexcept {
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

int controller::l2_overlay_addr_add(uint32_t lport, uint32_t tunnel_id,
                                    const rofl::cmacaddr &mac,
                                    bool permanent) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    auto fm = fm_driver.add_bridging_unicast_overlay(dpt.get_version(), lport,
                                                     tunnel_id, mac);
    if (!permanent && default_idle_timeout > 0) {
      fm.set_idle_timeout(default_idle_timeout);
      fm.set_flags(rofl::openflow::OFPFF_SEND_FLOW_REM);
    }

    dpt.send_flow_mod_message(rofl::cauxid(0), fm);

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

int controller::l2_overlay_addr_remove(uint32_t tunnel_id, uint32_t lport_id,
                                       const rofl::cmacaddr &mac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (lport_id) {
      dpt.send_flow_mod_message(
          rofl::cauxid(0), fm_driver.remove_bridging_unicast_overlay_all_lport(
                               dpt.get_version(), lport_id));
      dpt.send_barrier_request(rofl::cauxid(0));
    } else {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.remove_bridging_unicast_overlay(
                                    dpt.get_version(), tunnel_id, mac));
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

int controller::l2_multicast_group_join(
    uint32_t port, uint16_t vid, const rofl::caddress_ll &mc_group) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    int index = 1;
    struct multicast_entry n_mcast;
    n_mcast.key = std::make_tuple(mc_group, vid);
    n_mcast.index = index;

    // find grp/vlan combination
    auto it = std::find(mc_groups.begin(), mc_groups.end(), n_mcast);
    uint32_t group_id = (nbi::get_port_type(port) == nbi::port_type_lag)
                            ? fm_driver.group_id_l2_trunk_interface(port, vid)
                            : fm_driver.group_id_l2_interface(port, vid);

    if (it != mc_groups.end()) { // if mmac does exist, update
      it->l2_interface.emplace(group_id);
      // in case it was disabled previously
      it->disabled_l2_interface.erase(group_id);
      index = it->index;
    } else { // add
      for (auto i : mc_groups) {
        if (std::get<1>(i.key) == vid) {

          if (index == i.index)
            index++;
        }
      }

      n_mcast.index = index;
      n_mcast.l2_interface.emplace(group_id);
      it = mc_groups.insert(mc_groups.end(), n_mcast);
    }

    dpt.send_group_mod_message(
        rofl::cauxid(0), fm_driver.enable_group_l2_multicast(
                             dpt.get_version(), index, vid, it->l2_interface,
                             (it->l2_interface.size() > 1)));

    // create flow when creating group
    if (it->l2_interface.size() == 1)
      dpt.send_flow_mod_message(
          rofl::cauxid(0), fm_driver.add_bridging_multicast_vlan(
                               dpt.get_version(), it->index, vid, mc_group));
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

int controller::l2_multicast_group_leave(uint32_t port, uint16_t vid,
                                         const rofl::caddress_ll &mc_group,
                                         bool disable_only) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    struct multicast_entry n_mcast;
    n_mcast.key = std::make_tuple(mc_group, vid);

    // find grp/vlan combination
    auto it = std::find(mc_groups.begin(), mc_groups.end(), n_mcast);
    if (it == mc_groups.end()) {
      LOG(ERROR) << __FUNCTION__ << ": multicast group not found";
      return -EINVAL;
    }
    uint32_t group_id = (nbi::get_port_type(port) == nbi::port_type_lag)
                            ? fm_driver.group_id_l2_trunk_interface(port, vid)
                            : fm_driver.group_id_l2_interface(port, vid);

    if (it->l2_interface.erase(group_id) == 0) {
      if (disable_only) {
        // either already disabled or never existed
        LOG(ERROR) << __FUNCTION__ << ": interface group not found";
        return -EINVAL;
      }

      if (it->disabled_l2_interface.erase(group_id) == 0) {
        LOG(ERROR) << __FUNCTION__ << ": disabled interface group not found";
        return -EINVAL;
      }

      // if no in-kernel mdb entries left, we can delete the the group reference
      if (it->l2_interface.size() == 0 && it->disabled_l2_interface.size() == 0)
        mc_groups.erase(it);

      return 0;
    }

    if (disable_only)
      it->disabled_l2_interface.emplace(group_id);

    if (it->l2_interface.size() != 0) {
      dpt.send_group_mod_message(
          rofl::cauxid(0),
          // send update without port
          fm_driver.enable_group_l2_multicast(dpt.get_version(), it->index, vid,
                                              it->l2_interface, true));

      dpt.send_barrier_request(rofl::cauxid(0));
    } else {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.remove_bridging_multicast_vlan(
                                    dpt.get_version(), port, vid, mc_group));

      dpt.send_barrier_request(rofl::cauxid(0));

      dpt.send_group_mod_message(rofl::cauxid(0),
                                 fm_driver.disable_group_l2_multicast(
                                     dpt.get_version(), it->index, vid));
      // only delete the group if there are no in-kernel mdb entries
      if (it->disabled_l2_interface.size() == 0)
        mc_groups.erase(it);
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

int controller::l2_multicast_group_rejoin_all_in_vlan(uint32_t port,
                                                      uint16_t vid) noexcept {
  int rv = 0;
  uint32_t group_id = (nbi::get_port_type(port) == nbi::port_type_lag)
                          ? fm_driver.group_id_l2_trunk_interface(port, vid)
                          : fm_driver.group_id_l2_interface(port, vid);

  for (auto it = mc_groups.begin(); it != mc_groups.end(); it++) {
    if (std::get<1>(it->key) == vid &&
        it->disabled_l2_interface.count(group_id) != 0) {
      rv = l2_multicast_group_join(port, vid, std::get<0>(it->key));
      if (rv != 0)
        LOG(ERROR) << ": failed to rejoin L2 multicast group";
    }
  }

  return rv;
}

int controller::l2_multicast_group_leave_all_in_vlan(uint32_t port,
                                                     uint16_t vid) noexcept {
  int rv = 0;
  uint32_t group_id = (nbi::get_port_type(port) == nbi::port_type_lag)
                          ? fm_driver.group_id_l2_trunk_interface(port, vid)
                          : fm_driver.group_id_l2_interface(port, vid);

  for (auto it = mc_groups.begin(); it != mc_groups.end(); it++) {
    if (std::get<1>(it->key) == vid && it->l2_interface.count(group_id) != 0) {
      rv = l2_multicast_group_leave(port, vid, std::get<0>(it->key), true);
      if (rv != 0)
        LOG(ERROR) << ": failed to leave L2 multicast group";
    }
  }

  return rv;
}

int controller::l3_termination_add(uint32_t sport, uint16_t vid,
                                   const rofl::caddress_ll &dmac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (dmac.is_multicast()) {
      rv = dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.enable_tmac_ipv4_multicast_mac(dpt.get_version()));
    } else {
      rv = dpt.send_flow_mod_message(rofl::cauxid(0),
                                     fm_driver.enable_tmac_ipv4_unicast_mac(
                                         dpt.get_version(), sport, vid, dmac));
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

int controller::l3_termination_add_v6(uint32_t sport, uint16_t vid,
                                      const rofl::caddress_ll &dmac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (dmac.is_multicast()) {
      rv = dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.enable_tmac_ipv6_multicast_mac(dpt.get_version()));
    } else {
      rv = dpt.send_flow_mod_message(rofl::cauxid(0),
                                     fm_driver.enable_tmac_ipv6_unicast_mac(
                                         dpt.get_version(), sport, vid, dmac));
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

int controller::l3_termination_remove(uint32_t sport, uint16_t vid,
                                      const rofl::caddress_ll &dmac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.disable_tmac_ipv4_unicast_mac(
                                  dpt.get_version(), sport, vid, dmac));

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

int controller::l3_termination_remove_v6(
    uint32_t sport, uint16_t vid, const rofl::caddress_ll &dmac) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.disable_tmac_ipv6_unicast_mac(
                                  dpt.get_version(), sport, vid, dmac));

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
    uint32_t group_id = (nbi::get_port_type(port) == nbi::port_type_lag)
                            ? fm_driver.group_id_l2_trunk_interface(port, vid)
                            : fm_driver.group_id_l2_interface(port, vid);

    dpt.send_group_mod_message(rofl::cauxid(0),
                               fm_driver.enable_group_l3_unicast(
                                   dpt.get_version(), _egress_interface_id,
                                   src_mac, dst_mac, group_id, false));
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

int controller::l3_egress_update(uint32_t port, uint16_t vid,
                                 const rofl::caddress_ll &src_mac,
                                 const rofl::caddress_ll &dst_mac,
                                 uint32_t *l3_interface_id) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    // TODO unfiltered interface

    uint32_t group_id = (nbi::get_port_type(port) == nbi::port_type_lag)
                            ? fm_driver.group_id_l2_trunk_interface(port, vid)
                            : fm_driver.group_id_l2_interface(port, vid);

    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_group_l3_unicast(dpt.get_version(), *l3_interface_id,
                                          src_mac, dst_mac, group_id, true));
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

int controller::l3_egress_remove(uint32_t l3_interface_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_barrier_request(rofl::cauxid(0));
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

  freed_egress_interfaces_ids.insert(l3_interface_id);

  return rv;
}

int controller::l3_unicast_host_add(const rofl::caddress_in4 &ipv4_dst,
                                    uint32_t l3_interface_id, bool is_ecmp,
                                    bool update_route,
                                    uint16_t vrf_id) noexcept {
  int rv = 0;

  if (l3_interface_id > 0x0fffffff)
    return -EINVAL;

  if (is_ecmp && l3_interface_id == 0)
    return -EINVAL;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    if (l3_interface_id) {
      if (is_ecmp)
        l3_interface_id = fm_driver.group_id_l3_ecmp(l3_interface_id);
      else
        l3_interface_id = fm_driver.group_id_l3_unicast(l3_interface_id);
    }

    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_ipv4_unicast_host(
                                  dpt.get_version(), ipv4_dst, l3_interface_id,
                                  update_route, vrf_id));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound : dptid : " << dptid;
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

int controller::l3_unicast_host_add(const rofl::caddress_in6 &ipv6_dst,
                                    uint32_t l3_interface_id, bool is_ecmp,
                                    bool update_route,
                                    uint16_t vrf_id) noexcept {
  int rv = 0;

  if (l3_interface_id > 0x0fffffff)
    return -EINVAL;

  if (is_ecmp && l3_interface_id == 0)
    return -EINVAL;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    if (l3_interface_id) {
      if (is_ecmp)
        l3_interface_id = fm_driver.group_id_l3_ecmp(l3_interface_id);
      else
        l3_interface_id = fm_driver.group_id_l3_unicast(l3_interface_id);
    }

    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_ipv6_unicast_host(
                                  dpt.get_version(), ipv6_dst, l3_interface_id,
                                  update_route, vrf_id));
  } catch (rofl::eRofBaseNotFound &e) {
    LOG(ERROR) << ": caught rofl::eRofBaseNotFound : dptid : " << dptid;
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

int controller::l3_unicast_host_remove(const rofl::caddress_in4 &ipv4_dst,
                                       uint16_t vrf_id) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_flow_mod_message(
        rofl::cauxid(0), fm_driver.disable_ipv4_unicast_host(dpt.get_version(),
                                                             ipv4_dst, vrf_id));
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

int controller::l3_unicast_host_remove(const rofl::caddress_in6 &ipv6_dst,
                                       uint16_t vrf_id) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_ipv6_unicast_host(dpt.get_version(), ipv6_dst));
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

int controller::l3_unicast_route_add(const rofl::caddress_in4 &ipv4_dst,
                                     const rofl::caddress_in4 &mask,
                                     uint32_t l3_interface_id, bool is_ecmp,
                                     bool update_route,
                                     uint16_t vrf_id) noexcept {
  int rv = 0;

  if (l3_interface_id > 0x0fffffff)
    return -EINVAL;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    if (l3_interface_id) {
      if (is_ecmp)
        l3_interface_id = fm_driver.group_id_l3_ecmp(l3_interface_id);
      else
        l3_interface_id = fm_driver.group_id_l3_unicast(l3_interface_id);
    }

    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_ipv4_unicast_lpm(
                                  dpt.get_version(), ipv4_dst, mask,
                                  l3_interface_id, update_route, vrf_id));

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

int controller::l3_unicast_route_add(const rofl::caddress_in6 &ipv6_dst,
                                     const rofl::caddress_in6 &mask,
                                     uint32_t l3_interface_id, bool is_ecmp,
                                     bool update_route,
                                     uint16_t vrf_id) noexcept {
  int rv = 0;

  if (l3_interface_id > 0x0fffffff)
    return -EINVAL;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    if (l3_interface_id) {
      if (is_ecmp)
        l3_interface_id = fm_driver.group_id_l3_ecmp(l3_interface_id);
      else
        l3_interface_id = fm_driver.group_id_l3_unicast(l3_interface_id);
    }

    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_ipv6_unicast_lpm(
                                  dpt.get_version(), ipv6_dst, mask,
                                  l3_interface_id, update_route, vrf_id));

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

int controller::l3_ecmp_add(uint32_t *l3_ecmp_id,
                            const std::set<uint32_t> &l3_interfaces) noexcept {
  int rv = 0;
  uint32_t _ecmp_interface_id;
  bool increment = false;

  if (freed_ecmp_interfaces_ids.size()) {
    _ecmp_interface_id = *freed_ecmp_interfaces_ids.begin();
  } else {
    _ecmp_interface_id = ecmp_interface_id;
    increment = true;
  }

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    std::set<uint32_t> l3_interface_groups;
    std::transform(
        l3_interfaces.begin(), l3_interfaces.end(),
        std::inserter(l3_interface_groups, l3_interface_groups.end()),
        [&](uint32_t id) { return fm_driver.group_id_l3_unicast(id); });

    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_group_l3_ecmp(
            dpt.get_version(), fm_driver.group_id_l3_ecmp(_ecmp_interface_id),
            l3_interface_groups, false));
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
    ecmp_interface_id++;
  } else {
    freed_ecmp_interfaces_ids.erase(freed_ecmp_interfaces_ids.begin());
  }

  *l3_ecmp_id = _ecmp_interface_id;
  return rv;
}

int controller::l3_ecmp_update(
    uint32_t l3_ecmp_id, const std::set<uint32_t> &l3_interfaces) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    std::set<uint32_t> l3_interface_groups;
    std::transform(
        l3_interfaces.begin(), l3_interfaces.end(),
        std::inserter(l3_interface_groups, l3_interface_groups.end()),
        [&](uint32_t id) { return fm_driver.group_id_l3_unicast(id); });

    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_group_l3_ecmp(dpt.get_version(),
                                       fm_driver.group_id_l3_ecmp(l3_ecmp_id),
                                       l3_interface_groups, true));
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

int controller::l3_ecmp_remove(uint32_t l3_ecmp_id) noexcept {
  int rv = 0;

  if (l3_ecmp_id > 0x0fffffff)
    return -EINVAL;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_group_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_group_l3_ecmp(
            dpt.get_version(), fm_driver.group_id_l3_ecmp(l3_ecmp_id)));
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

  freed_ecmp_interfaces_ids.insert(l3_ecmp_id);

  return rv;
}

int controller::l3_unicast_route_remove(const rofl::caddress_in4 &ipv4_dst,
                                        const rofl::caddress_in4 &mask,
                                        uint16_t vrf_id) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.disable_ipv4_unicast_lpm(
                                  dpt.get_version(), ipv4_dst, mask, vrf_id));
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

int controller::l3_unicast_route_remove(const rofl::caddress_in6 &ipv6_dst,
                                        const rofl::caddress_in6 &mask,
                                        uint16_t vrf_id) noexcept {
  int rv = 0;

  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.disable_ipv6_unicast_lpm(
                                  dpt.get_version(), ipv6_dst, mask, vrf_id));
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

int controller::ingress_port_vlan_add(uint32_t port, uint16_t vid, bool pvid,
                                      uint16_t vrf_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (pvid) {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.enable_port_vid_ingress(
                                    dpt.get_version(), port, vid, vrf_id));
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.enable_port_pvid_ingress(dpt.get_version(), port, vid));
    } else {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.enable_port_vid_ingress(
                                    dpt.get_version(), port, vid, vrf_id));
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

int controller::ingress_port_pvid_add(uint32_t port, uint16_t pvid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_port_pvid_ingress(dpt.get_version(), port, pvid));

    uint32_t xid = 0;
    dpt.send_barrier_request(rofl::cauxid(0), 1, &xid);
    VLOG(2) << __FUNCTION__ << ": sent barrier with xid=" << (unsigned)xid;
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

int controller::ingress_port_pvid_remove(uint32_t port,
                                         uint16_t pvid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);

    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_port_pvid_ingress(dpt.get_version(), port, pvid));

    uint32_t xid = 0;
    dpt.send_barrier_request(rofl::cauxid(0), 1, &xid);
    VLOG(2) << __FUNCTION__ << ": sent barrier with xid=" << (unsigned)xid;
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

int controller::ingress_port_vlan_remove(uint32_t port, uint16_t vid, bool pvid,
                                         uint16_t vrf_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (pvid) {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.disable_port_vid_ingress(
                                    dpt.get_version(), port, vid, vrf_id));
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.disable_port_pvid_ingress(dpt.get_version(), port, vid));
    } else {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.disable_port_vid_ingress(
                                    dpt.get_version(), port, vid, vrf_id));
    }
    uint32_t xid = 0;
    dpt.send_barrier_request(rofl::cauxid(0), 1, &xid);
    VLOG(2) << __FUNCTION__ << ": sent barrier with xid=" << (unsigned)xid;
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

int controller::egress_port_vlan_add(uint32_t port, uint16_t vid, bool untagged,
                                     bool update) noexcept {
  int rv = 0;
  try {
    // create filtered egress interface
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    rofl::openflow::cofgroupmod gm;

    if (nbi::get_port_type(port) == nbi::port_type_lag) {
      gm = fm_driver.enable_group_l2_trunk_interface(dpt.get_version(), port,
                                                     vid, untagged, update);
    } else {
      gm = fm_driver.enable_group_l2_interface(dpt.get_version(), port, vid,
                                               untagged, update);
    }
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
    rofl::openflow::cofgroupmod gm;

    if (nbi::get_port_type(port) == nbi::port_type_lag) {
      gm = fm_driver.disable_group_l2_trunk_interface(dpt.get_version(), port,
                                                      vid);
    } else {
      gm = fm_driver.disable_group_l2_interface(dpt.get_version(), port, vid);
    }
    dpt.send_group_mod_message(rofl::cauxid(0), gm);
    uint32_t xid = 0;
    dpt.send_barrier_request(rofl::cauxid(0), 1, &xid);
    VLOG(2) << __FUNCTION__ << ": sent barrier with xid=" << (unsigned)xid;
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

int controller::add_l2_overlay_flood(uint32_t tunnel_id,
                                     uint32_t lport_id) noexcept {
  int rv = 0;
  try {
    auto tunnel_dlf_it = tunnel_dlf_flood.find(tunnel_id);

    if (tunnel_dlf_it == tunnel_dlf_flood.end()) {
      std::set<uint32_t> empty_set;
      tunnel_dlf_it = tunnel_dlf_flood.emplace(tunnel_id, empty_set).first;
    }

    tunnel_dlf_it->second.insert(lport_id);

    rofl::crofdpt &dpt = set_dpt(dptid, true);

    // create/update new L2 flooding group
    LOG(INFO) << __FUNCTION__
              << ": create group enable_group_l2_overlay_flood tunnel_id="
              << tunnel_id << ", #dlf=" << tunnel_dlf_it->second.size();

    for (auto lport : tunnel_dlf_it->second)
      LOG(INFO) << __FUNCTION__ << ": lport=" << lport;

    dpt.send_group_mod_message(rofl::cauxid(0),
                               fm_driver.enable_group_l2_overlay_flood(
                                   dpt.get_version(), tunnel_id, tunnel_id,
                                   tunnel_dlf_it->second,
                                   (tunnel_dlf_it->second.size() > 1)));

    dpt.send_barrier_request(rofl::cauxid(0));

    //   if (tunnel_dlf_it->second.size() == 1) {
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.add_bridging_dlf_overlay(
            dpt.get_version(), tunnel_id,
            fm_driver.group_id_l2_overlay_flood(tunnel_id, tunnel_id)));
    //   }

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

int controller::del_l2_overlay_flood(uint32_t tunnel_id,
                                     uint32_t lport_id) noexcept {
  int rv = 0;
  try {
    auto tunnel_dlf_it = tunnel_dlf_flood.find(tunnel_id);

    if (tunnel_dlf_it == tunnel_dlf_flood.end()) {
      // nothing in this group
      VLOG(1) << __FUNCTION__ << ": invalid tunnel_id=" << tunnel_id;
      return 0;
    }

    auto elements_erased = tunnel_dlf_it->second.erase(lport_id);
    if (elements_erased == 0) {
      // nothing to do, lport already deleted
      VLOG(1) << __FUNCTION__ << ": lport_id=" << lport_id
              << " not existing in tunnel_id=" << tunnel_id;
      return 0;
    }

    rofl::crofdpt &dpt = set_dpt(dptid, true);

    if (tunnel_dlf_it->second.size()) {
      // create/update new L2 flooding group
      dpt.send_group_mod_message(rofl::cauxid(0),
                                 fm_driver.enable_group_l2_overlay_flood(
                                     dpt.get_version(), tunnel_id, tunnel_id,
                                     tunnel_dlf_it->second, true));
    } else {
      dpt.send_flow_mod_message(
          rofl::cauxid(0),
          fm_driver.remove_bridging_dlf_overlay(dpt.get_version(), tunnel_id));
      dpt.send_barrier_request(rofl::cauxid(0));
      dpt.send_group_mod_message(rofl::cauxid(0),
                                 fm_driver.disable_group_l2_overlay_flood(
                                     dpt.get_version(), tunnel_id, tunnel_id));
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

int controller::egress_bridge_port_vlan_add(uint32_t port, uint16_t vid,
                                            bool untagged) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    std::set<uint32_t> l2_dom_set;

    // create filtered egress interface
    rv = egress_port_vlan_add(port, vid, untagged);
    if (nbi::get_port_type(port) == nbi::port_type_lag)
      return 0;

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

    uint32_t xid = 0;
    dpt.send_barrier_request(rofl::cauxid(0), 1, &xid);
    VLOG(2) << __FUNCTION__ << ": sent barrier with xid=" << (unsigned)xid;

    // remove filtered egress interface
    rv = egress_port_vlan_remove(port, vid);

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
int controller::ingress_port_stacked_vlan_enable(uint32_t port,
                                                 uint16_t vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0), fm_driver.enable_port_vid_ingress(dpt.get_version(),
                                                           port, vid, 0, true));
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
int controller::ingress_port_stacked_vlan_disable(uint32_t port,
                                                  uint16_t vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.disable_port_vid_ingress(
                                  dpt.get_version(), port, vid, 0, true));
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
int controller::ingress_port_pop_vlan_add(uint32_t port, uint16_t outer_vid,
                                          uint16_t inner_vid,
                                          uint16_t vrf_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_port_pop_tag_ingress(dpt.get_version(), port,
                                              inner_vid, outer_vid, vrf_id));
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
int controller::ingress_port_pop_vlan_remove(uint32_t port, uint16_t outer_vid,
                                             uint16_t inner_vid,
                                             uint16_t vrf_id) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.disable_port_pop_tag_ingress(dpt.get_version(), port,
                                               inner_vid, outer_vid, vrf_id));
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

int controller::egress_port_push_vlan_add(uint32_t port, uint16_t vid,
                                          uint16_t push_vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_vlan_egress_push_tag(
                                  dpt.get_version(), port, vid, push_vid));
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
int controller::egress_port_push_vlan_remove(uint32_t port, uint16_t vid,
                                             uint16_t push_vid) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.disable_vlan_egress_push_tag(
                                  dpt.get_version(), port, vid, push_vid));
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

int controller::set_egress_tpid(uint32_t port) noexcept {
  int rv = 0;
  try {

    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.set_port_tpid(dpt.get_version(), port));

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

int controller::delete_egress_tpid(uint32_t port) noexcept {
  int rv = 0;
  try {

    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_flow_mod_message(
        rofl::cauxid(0), fm_driver.remove_port_tpid(dpt.get_version(), port));

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

int controller::port_set_config(uint32_t port, const rofl::caddress_ll &mac,
                                bool up) noexcept {
  int rv = 0;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    dpt.send_port_mod_message(rofl::cauxid(0), port, mac,
                              up ? 0 : rofl::openflow13::OFPPC_PORT_DOWN,
                              rofl::openflow13::OFPPC_PORT_DOWN, 0);
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
  this->flags = this->flags | flags;
  try {
    rofl::crofdpt &dpt = set_dpt(dptid, true);
    if (flags & switch_interface::SWIF_ARP) {
      dpt.send_flow_mod_message(rofl::cauxid(0),
                                fm_driver.enable_policy_arp(dpt.get_version()));
    }
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_tmac_bpdu_multicast_mac(
                                  dpt.get_version(),
                                  rofl::caddress_ll("01:80:c2:00:00:00"),
                                  rofl::caddress_ll("ff:ff:ff:ff:ff:f0")));

    /* Add flowmods to always copy IS-IS PDUs (identified by eth_dst mac) to
     * controller and clear them from the pipeline to avoid duplication
     * 01-80-C2-00-00-14 ("all L1 intermediate systems")
     * 01-80-C2-00-00-15 ("All L2 intermediate systems")
     * 09-00-2B-00-00-04 ("all end systems")
     * 09-00-2B-00-00-05 ("all intermediate systems")
     */
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_tmac_bpdu_multicast_mac(
                                  dpt.get_version(),
                                  rofl::caddress_ll("01:80:C2:00:00:14"),
                                  rofl::caddress_ll("ff:ff:ff:ff:ff:fe")));
    dpt.send_flow_mod_message(rofl::cauxid(0),
                              fm_driver.enable_tmac_bpdu_multicast_mac(
                                  dpt.get_version(),
                                  rofl::caddress_ll("09:00:2B:00:00:04"),
                                  rofl::caddress_ll("ff:ff:ff:ff:ff:fe")));

    // Adding policy entry so that the multicast packets reach the switch
    // The ff02:: address is a permanent multicast address with a link scope
    dpt.send_flow_mod_message(
        rofl::cauxid(0), fm_driver.enable_policy_ipv6_multicast(
                             dpt.get_version(), rofl::caddress_in6("ff02::"),
                             rofl::build_mask_in6(16)));
    dpt.send_flow_mod_message(
        rofl::cauxid(0), fm_driver.enable_policy_ipv4_multicast(
                             dpt.get_version(), rofl::caddress_in4("224.0.0.0"),
                             rofl::build_mask_in4(4)));

    // Enable BOOTP/DHCP client -> server to CONTROLLER
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_policy_udp(dpt.get_version(), ETH_P_IP, 67, 68));
    // Enable BOOTP/DHCP server -> client to CONTROLLER
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_policy_udp(dpt.get_version(), ETH_P_IP, 68, 67));

    // Enable DHCPv6 client -> server to CONTROLLER
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_policy_udp(dpt.get_version(), ETH_P_IPV6, 546, 547));
    // Enable DHCPv6 server -> client to CONTROLLER
    dpt.send_flow_mod_message(
        rofl::cauxid(0),
        fm_driver.enable_policy_udp(dpt.get_version(), ETH_P_IPV6, 547, 546));
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
      counters[i] = 0; // tx_errors counters are not supported in OF-DPA
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

int controller::tunnel_tenant_delete(uint32_t tunnel_id) noexcept {
  return ofdpa->ofdpaTunnelTenantDelete(tunnel_id);
}

int controller::tunnel_next_hop_create(uint32_t next_hop_id, uint64_t src_mac,
                                       uint64_t dst_mac, uint32_t physical_port,
                                       uint16_t vlan_id) noexcept {
  return ofdpa->ofdpaTunnelNextHopCreate(next_hop_id, src_mac, dst_mac,
                                         physical_port, vlan_id);
}

int controller::tunnel_next_hop_modify(uint32_t next_hop_id, uint64_t src_mac,
                                       uint64_t dst_mac, uint32_t physical_port,
                                       uint16_t vlan_id) noexcept {
  return ofdpa->ofdpaTunnelNextHopModify(next_hop_id, src_mac, dst_mac,
                                         physical_port, vlan_id);
}

int controller::tunnel_next_hop_delete(uint32_t next_hop_id) noexcept {
  return ofdpa->ofdpaTunnelNextHopDelete(next_hop_id);
}

int controller::tunnel_access_port_create(uint32_t port_id,
                                          const std::string &port_name,
                                          uint32_t physical_port,
                                          uint16_t vlan_id,
                                          bool untagged) noexcept {
  return ofdpa->ofdpaTunnelAccessPortCreate(port_id, port_name, physical_port,
                                            vlan_id, untagged);
}

int controller::tunnel_port_delete(uint32_t port_id) noexcept {
  return ofdpa->ofdpaTunnelPortDelete(port_id);
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

int controller::tunnel_port_tenant_add(uint32_t lport_id,
                                       uint32_t tunnel_id) noexcept {
  int rv = ofdpa->ofdpaTunnelPortTenantAdd(lport_id, tunnel_id);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add port " << lport_id
               << " to tenant " << tunnel_id;
  }

  return rv;
}

int controller::tunnel_port_tenant_remove(uint32_t lport_id,
                                          uint32_t tunnel_id) noexcept {
  int rv;
  int cnt = 0;
  do {
    // XXX TODO this is totally crap even if it works for now
    using namespace std::chrono_literals;

    rv = ofdpa->ofdpaTunnelPortTenantDelete(lport_id, tunnel_id);

    VLOG(2) << __FUNCTION__ << ": rv=" << rv << ", cnt=" << cnt
            << ", lport_id=" << lport_id << ", tunnel_id=" << tunnel_id;

    cnt++;
    std::this_thread::sleep_for(10ms);
  } while (rv < 0 && cnt < 50);

  assert(rv == 0);

  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to remove port " << lport_id
               << " from tenant " << tunnel_id;
  }

  return rv;
}

int controller::lookup_stpid(uint32_t vlan_id) noexcept {
  auto it = vlan_to_stg.find(vlan_id);

  return (it == vlan_to_stg.end()) ? 0 : it->second;
}

int controller::find_free_stgid() noexcept {
  // 512 groups, 2 reserved (0 and 1)
  if (vlan_to_stg.size() == (512 - 2))
    return -ENOSPC;

  while (stg_in_use[current_stg]) {
    current_stg++;

    if (current_stg == 512)
      current_stg = 2;
  }

  return current_stg;
}

int controller::ofdpa_stg_destroy(uint16_t vlan_id) noexcept {
  int rv;
  int stg_id = lookup_stpid(vlan_id);

  if (stg_id == 0)
    return 0;

  rv = ofdpa->ofdpaStgDestroy(stg_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to destroy the STP group";
    return rv;
  }

  vlan_to_stg.erase(vlan_id);
  stg_in_use[stg_id] = false;

  return rv;
}

int controller::ofdpa_stg_state_port_set(uint32_t port_id, uint16_t vlan_id,
                                         uint8_t state) noexcept {
  std::string bcm_state;
  int rv;
  int stg_id = lookup_stpid(vlan_id);
  if (stg_id == 0)
    stg_id = 1;

  switch (state) {
  case BR_STATE_FORWARDING:
    bcm_state = "forward";
    break;
  case BR_STATE_BLOCKING:
    bcm_state = "block";
    break;
  case BR_STATE_DISABLED:
    bcm_state = "disable";
    break;
  case BR_STATE_LISTENING:
    bcm_state = "listen";
    break;
  case BR_STATE_LEARNING:
    bcm_state = "learn";
    break;
  default:
    return -EINVAL;
  }

  rv = ofdpa->ofdpaStgStatePortSet(port_id, bcm_state, stg_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to set the STP state";
  }

  return rv;
}

int controller::ofdpa_stg_create(uint16_t vlan_id) noexcept {
  int rv;
  int stg_id = lookup_stpid(vlan_id);

  if (stg_id != 0)
    return stg_id;

  stg_id = find_free_stgid();
  if (stg_id < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to allocate STG ID: " << stg_id;
    return stg_id;
  }

  rv = ofdpa->ofdpaStgCreate(stg_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to create the STP group";
    return rv;
  }

  rv = ofdpa->ofdpaStgVlanAdd(vlan_id, stg_id);
  if (rv < 0) {
    LOG(ERROR) << __FUNCTION__ << ": failed to add VLAN=" << vlan_id
               << " to the STP group=" << stg_id;
    return rv;
  }

  vlan_to_stg.emplace(std::make_pair(vlan_id, stg_id));
  stg_in_use[current_stg] = true;

  return rv;
}

} // namespace basebox
