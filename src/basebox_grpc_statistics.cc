#include "basebox_grpc_statistics.h"
#include <glog/logging.h>

namespace basebox {

using openconfig_interfaces::Interface_State;
using openconfig_interfaces::Interface_State_Counters;
using openconfig_interfaces::Interfaces;
using openconfig_interfaces::Interfaces_Interface;

::grpc::Status NetworkStats::GetStatistics(::grpc::ServerContext *context, const Empty *request,
                                   Interfaces *response) {
  VLOG(2) << __FUNCTION__ << ": received grpc call";

  response->CopyFrom(*netstats);
  return ::grpc::Status::OK;
}

void NetworkStats::addStatistics(
    const std::string &nodeInfo,
    const std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>>
        &ports) {

  for (auto port : ports) {
    Interfaces_Interface *intf = netstats->add_interface();
    intf->set_name(nodeInfo);

    Interface_State *state = intf->mutable_state();
    state->set_name(port.first);
    Interface_State_Counters *counters = state->mutable_counters();

    counters->set_out_errors(port.second.get_tx_errors());
    counters->set_out_broadcast_pkts(port.second.get_tx_packets());
    counters->set_out_discards(port.second.get_tx_dropped());
    counters->set_out_octets(port.second.get_tx_bytes());

    counters->set_in_errors(port.second.get_rx_errors());
    counters->set_in_fcs_errors(port.second.get_rx_crc_err());
    counters->set_in_broadcast_pkts(port.second.get_rx_packets());
    counters->set_in_discards(port.second.get_rx_dropped());
    counters->set_in_octets(port.second.get_rx_bytes());
  }
}

void NetworkStats::flush() {
  netstats->clear_interface();
}

} // namespace basebox
