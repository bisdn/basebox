#include "basebox_grpc_topology.h"
#include <glog/logging.h>

Networks_Network *NetworkImpl::networktopology = new Networks_Network();

Status NetworkImpl::GetTopology(ServerContext *context, const Empty *request,
                                Networks *response) {
  LOG(INFO) << __FUNCTION__ << " RECEIVED GRPC CALL";

  Networks_Network *temp = new Networks_Network();

  std::lock_guard<std::mutex> lock(topology_mutex);

  temp->CopyFrom(*networktopology);
  temp->set_network_id("0");

  RepeatedPtrField<Networks_Network> *netarg = response->mutable_network();
  netarg->AddAllocated(temp);

  return Status::OK;
}

void NetworkImpl::addNode(
    const std::string &nodeInfo,
    const std::list<std::pair<std::string, rofl::openflow::cofport_stats_reply>>
        &ports) {

  std::lock_guard<std::mutex> lock(topology_mutex);

  Network_Node *node = networktopology->add_node();
  node->set_node_id(nodeInfo);
  for (auto port : ports) {
    Network_Node_TerminationPoint *tp = node->add_termination_point();
    tp->set_tp_id(port.first);
  }
}

void NetworkImpl::addLink(const std::string &nodeSrc,
                          const std::string &portSrc,
                          const std::string &nodeDst,
                          const std::string &portDst) {

  std::lock_guard<std::mutex> lock(topology_mutex);

  Network_Link *link = networktopology->add_link();

  int linkid = networktopology->link_size() + 1;
  link->set_link_id(std::to_string(linkid));

  link->mutable_source()->set_source_node(nodeSrc);
  link->mutable_source()->set_source_tp(portSrc);

  link->mutable_destination()->set_dest_node(nodeDst);
  link->mutable_destination()->set_dest_tp(portDst);
}

void NetworkImpl::flush() {
  std::lock_guard<std::mutex> lock(topology_mutex);

  networktopology->clear_network_id();
  networktopology->clear_link();
  networktopology->clear_node();
}
