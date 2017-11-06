#include "basebox_api.h"
#include <glog/logging.h>

void ApiServer::runGRPCServer() {
  std::string server_address("0.0.0.0:5000");
  ::grpc::ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&topology);
  builder.RegisterService(&stats);
  server = builder.BuildAndStart();
  LOG(INFO) << "gRPC server listening on " << server_address;
  server->Wait();
}

int ApiServer::addNode(
    const std::string &nodeInfo,
    const std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>>
        &ports) {

  topology.addNode(nodeInfo, ports);
  stats.addStatistics(nodeInfo, ports);

  return 0;
}

int ApiServer::addLink(const std::string &nodeSrc, const std::string &portSrc,
                       const std::string &nodeDst, const std::string &portDst) {
  topology.addLink(nodeSrc, portSrc, nodeDst, portDst);
  return 0;
}

void ApiServer::flush() {
  topology.flush();
  stats.flush();
}

void ApiServer::initStructures() {

//  std::pair<std::string, rofl::openflow::cofport_stats_reply> pair;
//  rofl::openflow::cofport_stats_reply temp;
//  temp.set_tx_packets(0);
//  temp.set_rx_packets(0);
//  pair = std::make_pair("", NULL);
//
//  std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>> ports;
//  ports.push_back(pair);

//  topology.addNode("", {});
//  topology.addLink("", "", "", "");
//  stats.addStatistics("", {});
}

void ApiServer::shutdown() {
    LOG(INFO) << "Requested Shutdown"; 
    server->Shutdown();    
}

