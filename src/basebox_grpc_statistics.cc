#include "basebox_grpc_statistics.h"
#include <glog/logging.h>

namespace basebox {

using openconfig_interfaces::Interface_State;
using openconfig_interfaces::Interface_State_Counters;
using openconfig_interfaces::Interfaces;
using openconfig_interfaces::Interfaces_Interface;

NetworkStats::NetworkStats(std::shared_ptr<switch_interface> swi)
    : swi(swi), ifaces(new Interfaces()) {}

NetworkStats::~NetworkStats() { delete ifaces; }

::grpc::Status NetworkStats::GetStatistics(::grpc::ServerContext *context,
                                           const Empty *request,
                                           Interfaces *response) {
  VLOG(2) << __FUNCTION__ << ": received grpc call";

  // XXX get stats from sai
  // swi->get_statistics();

  response->CopyFrom(*ifaces);
  return ::grpc::Status::OK;
}

} // namespace basebox
