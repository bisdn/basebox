#pragma once
#include <grpc++/grpc++.h>

#include "services-definition.grpc.pb.h"
#include "rofl/common/openflow/cofportstats.h"

using namespace openconfig_interfaces;
using namespace grpc;
using ::google::protobuf::RepeatedPtrField;

class NetworkStats final : public ::api::NetworkStatistics::Service {
public:
  typedef ::api::Empty Empty;
  NetworkStats() {}
  Status GetStatistics(ServerContext *context, const Empty *request, ServerWriter<Interfaces_Interface> *writer);
  void addStatistics(const std::string &nodeInfo, const std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>> &ports);
  void flush();
  virtual ~NetworkStats() {}

private:
  static Interfaces *netstats;
  void fakeStatistic();
  std::mutex stats_mutex;
};
