#include "basebox_grpc_statistics.h"
#include <glog/logging.h>

::grpc::Status NetworkStats::GetStatistics(::grpc::ServerContext *context, const Empty *request,
                                   ::grpc::ServerWriter<Interfaces_Interface> *writer) {
  LOG(INFO) << __FUNCTION__ << " RECEIVED GRPC CALL";

  for (int i = 0; i < netstats->interface_size(); i++) {
    writer->Write(netstats->interface(i));
  }
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
/*
void NetworkImpl::fakeStatistic() {
    for (int i = 0; i < 5; i++) {
        Network_Node* node = networktopology->add_node();
        Network_Node_TerminationPoint* tp = node->add_termination_point();
        tp->set_tp_id(std::to_string(i));

        Interface_State_Counters *counters = tp->mutable_port_stats();
        counters->set_out_errors(8);
        counters->set_in_errors(8);
        counters->set_in_fcs_errors(8);
    }
}
*/

void NetworkStats::flush() {
  netstats->clear_interface();
}
