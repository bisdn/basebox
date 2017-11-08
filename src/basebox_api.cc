#include "basebox_api.h"
#include <glog/logging.h>

namespace basebox {

void ApiServer::runGRPCServer() {
  std::string server_address("0.0.0.0:5000");
  ::grpc::ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(topology);
  builder.RegisterService(stats);
  server = builder.BuildAndStart();
  LOG(INFO) << "gRPC server listening on " << server_address;
  server->Wait();
}

int ApiServer::addNode(
    const std::string &nodeInfo,
    const std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>>
        &ports) {

  topology->addNode(nodeInfo, ports);
  stats->addStatistics(nodeInfo, ports);

  return 0;
}

int ApiServer::addLink(const std::string &nodeSrc, const std::string &portSrc,
                       const std::string &nodeDst, const std::string &portDst) {
  topology->addLink(nodeSrc, portSrc, nodeDst, portDst);
  return 0;
}

void ApiServer::initStructures() {
    topology = new NetworkImpl();
    stats = new NetworkStats();

    addNode("", {});
    addLink("", "", "", "");
}

void ApiServer::flush() {
  topology->flush();
  stats->flush();
}

} // namespace basebox
