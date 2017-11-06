#pragma once
#include <google/protobuf/repeated_field.h>
#include <grpc++/channel.h>
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>

#include "basebox_grpc_statistics.h"
#include "basebox_grpc_topology.h"

using namespace openconfig_interfaces;
using namespace ietf_network;
class ApiServer {
public:
  ApiServer() { initStructures(); }

  void runGRPCServer();

  int addNode(
      const std::string &nodeInfo,
      const std::list<
          std::pair<std::string, rofl::openflow::cofport_stats_reply>> &ports);
  int addLink(const std::string &nodeSrc, const std::string &portSrc,
              const std::string &nodeDst, const std::string &portDst);
  void flush();

  virtual ~ApiServer() {
    delete topology;
    delete stats;
  }

private:
  NetworkImpl* topology;
  NetworkStats* stats; 
  std::unique_ptr<::grpc::Server> server;
  void initStructures();
};
