#include "basebox_api.h"
#include <glog/logging.h>

void ApiServer::runGRPCServer() {
  std::string server_address("0.0.0.0:5000");
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&topology);
  builder.RegisterService(&stats);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  LOG(INFO) << "gRPC server listening on " << server_address;
  server->Wait();
}

int ApiServer::addNode(const std::string &nodeInfo, const std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>> &ports) {

  topology.addNode(nodeInfo, ports);
  stats.addStatistics(nodeInfo, ports);

  return 0;
}

int ApiServer::addLink(const std::string &nodeSrc, const std::string &portSrc, const std::string &nodeDst, const std::string &portDst) {
  topology.addLink(nodeSrc, portSrc, nodeDst, portDst);
  return 0;
}

void ApiServer::flush() {
  topology.flush();
  stats.flush();
}

void ApiServer::initStructures() {
  std::pair<std::string, rofl::openflow::cofport_stats_reply> pair;
    rofl::openflow::cofport_stats_reply temp;
    temp.set_tx_packets(64);
    temp.set_rx_packets(128);
  pair = std::make_pair("2", temp);

  std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>> ports;
  ports.push_back(pair);

  topology.addNode("2", ports);
  topology.addLink("2", "1", "3", "1");
  stats.addStatistics("2", ports);
}
