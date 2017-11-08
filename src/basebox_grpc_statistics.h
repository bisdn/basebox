#pragma once
#include <grpc++/grpc++.h>

#include "services-definition.grpc.pb.h"
#include "rofl/common/openflow/cofportstats.h"

namespace basebox {

class NetworkStats final : public ::api::NetworkStatistics::Service {
public:
  typedef ::api::Empty Empty;

  NetworkStats() { NetworkStats::netstats = new openconfig_interfaces::Interfaces(); }
  
  ::grpc::Status GetStatistics(::grpc::ServerContext *context, const Empty *request,
                       openconfig_interfaces::Interfaces *response);
  void addStatistics(
      const std::string &nodeInfo,
      const std::list<
          std::pair<std::string, rofl::openflow::cofport_stats_reply>> &ports);
  void flush();
  virtual ~NetworkStats() { delete netstats; }

private:
  openconfig_interfaces::Interfaces *netstats;
  void fakeStatistic();
};

} // namespace basebox
