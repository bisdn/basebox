#pragma once

#include <grpc++/grpc++.h>

#include "rofl/common/openflow/cofportstats.h"
#include "sai.hpp"
#include "services-definition.grpc.pb.h"

namespace basebox {

class NetworkStats final : public ::api::NetworkStatistics::Service {
public:
  typedef ::api::Empty Empty;

  NetworkStats(std::shared_ptr<switch_interface> swi);
  virtual ~NetworkStats();

  ::grpc::Status
  GetStatistics(::grpc::ServerContext *context, const Empty *request,
                openconfig_interfaces::Interfaces *response) override;

private:
  std::shared_ptr<switch_interface> swi;
  openconfig_interfaces::Interfaces *ifaces; // XXX really needed?
};

} // namespace basebox
