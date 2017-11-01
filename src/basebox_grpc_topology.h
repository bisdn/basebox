#pragma once
#include <grpc++/grpc++.h>

#include "services-definition.grpc.pb.h"
#include "rofl/common/openflow/cofportstats.h"

using namespace ietf_network;
using namespace grpc;
using ::google::protobuf::RepeatedPtrField;

class NetworkImpl final : public ::api::NetworkDescriptor::Service {
public:
  typedef ::api::Empty Empty;
  NetworkImpl() {};
  Status GetTopology(ServerContext *context, const Empty *request, Networks *response) override;
  void addNode(const std::string &nodeInfo, const std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>> &ports);
  void addLink(const std::string &nodeSrc, const std::string &portSrc, const std::string &nodeDst, const std::string &portDst);
  void flush();
  virtual ~NetworkImpl() {};

private:
  static Networks_Network *networktopology;
  void fakeStatistic();
  std::mutex topology_mutex;
};
