#pragma once
#include <grpc++/grpc++.h>

#include "services-definition.grpc.pb.h"
#include "rofl/common/openflow/cofportstats.h"

using namespace openconfig_interfaces;

class NetworkStats final : public ::api::NetworkStatistics::Service {
public:
  typedef ::api::Empty Empty;

  NetworkStats() { NetworkStats::netstats = new Interfaces(); }
  
  ::grpc::Status GetStatistics(::grpc::ServerContext *context, const Empty *request,
                       Interfaces *response);
  void addStatistics(
      const std::string &nodeInfo,
      const std::list<
          std::pair<std::string, rofl::openflow::cofport_stats_reply>> &ports);
  void flush();
  virtual ~NetworkStats() { delete netstats; }

private:
  Interfaces *netstats;
  void fakeStatistic();
};
