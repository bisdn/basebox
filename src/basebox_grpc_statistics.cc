/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <glog/logging.h>
#include <vector>

#include "basebox_grpc_statistics.h"
#include "sai.hpp"
#include "netlink/tap_manager.hpp"

namespace basebox {

using openconfig_interfaces::Interface_State;
using openconfig_interfaces::Interface_State_Counters;
using openconfig_interfaces::Interfaces;
using openconfig_interfaces::Interfaces_Interface;

NetworkStats::NetworkStats(std::shared_ptr<switch_interface> swi,
                           std::shared_ptr<tap_manager> tap_man)
    : swi(swi), tap_man(tap_man) {}

::grpc::Status NetworkStats::GetStatistics(
    __attribute__((unused))::grpc::ServerContext *context,
    __attribute__((unused)) const Empty *request, Interfaces *response) {
  VLOG(2) << __FUNCTION__ << ": received grpc call";

  static const std::vector<switch_interface::sai_port_stat_t> counter_ids = {
      switch_interface::SAI_PORT_STAT_RX_PACKETS,
      switch_interface::SAI_PORT_STAT_TX_PACKETS,
      switch_interface::SAI_PORT_STAT_RX_BYTES,
      switch_interface::SAI_PORT_STAT_TX_BYTES,
      switch_interface::SAI_PORT_STAT_RX_DROPPED,
      switch_interface::SAI_PORT_STAT_TX_DROPPED,
      switch_interface::SAI_PORT_STAT_RX_ERRORS,
      switch_interface::SAI_PORT_STAT_TX_ERRORS,
      switch_interface::SAI_PORT_STAT_RX_FRAME_ERR,
      switch_interface::SAI_PORT_STAT_RX_OVER_ERR,
      switch_interface::SAI_PORT_STAT_RX_CRC_ERR,
      switch_interface::SAI_PORT_STAT_COLLISIONS};
  auto ports = tap_man->get_registered_ports();

  for (const auto &port : ports) {
    std::vector<uint64_t> stats(counter_ids.size());
    int rv = swi->get_statistics(port.second, counter_ids.size(),
                                 counter_ids.data(), stats.data());
    if (rv != 0)
      continue;

    ::openconfig_interfaces::Interface_State *state =
        response->add_interface()->mutable_state();
    state->set_name(port.first);

    state->mutable_counters()->set_in_discards(stats[4]);
    state->mutable_counters()->set_in_errors(stats[6] + stats[9]);
    state->mutable_counters()->set_in_fcs_errors(stats[8] + stats[7]);
    state->mutable_counters()->set_in_octets(stats[2]);
    state->mutable_counters()->set_in_unicast_pkts(stats[0]);

    state->mutable_counters()->set_out_discards(stats[5]);
    state->mutable_counters()->set_out_errors(stats[7] + stats[11]);
    state->mutable_counters()->set_out_octets(stats[3]);
    state->mutable_counters()->set_out_unicast_pkts(stats[1]);
  }

  return ::grpc::Status::OK;
}

} // namespace basebox
