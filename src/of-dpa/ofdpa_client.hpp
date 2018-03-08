#pragma once

#include <memory>

#include "ofdpa.grpc.pb.h"

namespace grpc {
class Channel;
}

namespace basebox {

class ofdpa_client {
public:
  ofdpa_client(std::shared_ptr<grpc::Channel> channel);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelTenantCreate(uint32_t tunnel_id, uint32_t vni);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelNextHopCreate(uint32_t next_hop_id, uint64_t src_mac,
                           uint64_t dst_mac, uint32_t physical_port,
                           uint16_t vlan_id);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelAccessPortCreate(uint32_t port_id, const std::string &port_name,
                              uint32_t physical_port, uint16_t vlan_id,
                              bool untagged);

  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaTunnelEndpointPortCreate(
      uint32_t port_id, const std::string &port_name, uint32_t remote_ipv4,
      uint32_t local_ipv4, uint32_t ttl, uint32_t next_hop_id,
      uint32_t terminator_udp_dst_port, uint32_t initiator_udp_dst_port,
      uint32_t udp_src_port_if_no_entropy, bool use_entropy);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelPortTenantAdd(uint32_t port_id, uint32_t tunnel_id);

private:
  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelPortCreate(const ::ofdpa::TunnelPortCreate &request);

  std::unique_ptr<ofdpa::OfdpaRpc::Stub> stub_;
};

} // namespace basebox
