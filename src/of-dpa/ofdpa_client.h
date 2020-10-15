/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <memory>

#include "api/ofdpa.grpc.pb.h"
#include "stp/openconfig-spanning-tree.pb.h"
#include "trunk/trunk-interfaces.pb.h"

using grpc::Channel;

namespace basebox {

class ofdpa_client {
public:
  ofdpa_client(std::shared_ptr<grpc::Channel> channel);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelTenantCreate(uint32_t tunnel_id, uint32_t vni);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelTenantDelete(uint32_t tunnel_id);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelNextHopCreate(uint32_t next_hop_id, uint64_t src_mac,
                           uint64_t dst_mac, uint32_t physical_port,
                           uint16_t vlan_id);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelNextHopDelete(uint32_t next_hop_id);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelNextHopModify(uint32_t next_hop_id, uint64_t src_mac,
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

  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaTunnelPortDelete(uint32_t lport_id);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelPortTenantAdd(uint32_t port_id, uint32_t tunnel_id);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelPortTenantDelete(uint32_t port_id, uint32_t tunnel_id);

#if 0
  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaStgCreate();
  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaStgDestroy();

  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaStgVlanAdd();
  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaStgVlanRemove();
#endif

  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaStgStatePortSet(uint32_t port_id,
                                                           std::string state);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  OfdpaTrunkCreate(uint32_t lag_id, std::string name, uint8_t mode);
  ofdpa::OfdpaStatus::OfdpaStatusCode OfdpaTrunkDelete(uint32_t lag_id);

  ofdpa::OfdpaStatus::OfdpaStatusCode
  OfdpaPortTrunkGroupSet(uint32_t port_id, uint32_t trunk_id);
  ofdpa::OfdpaStatus::OfdpaStatusCode
  OfdpaTrunkPortMemberActiveSet(uint32_t port_id, uint32_t trunk_id,
                                uint32_t active);

  ofdpa::OfdpaStatus::OfdpaStatusCode ofdpaTrunkPortPSCSet(uint32_t lag_id,
                                                           uint8_t mode);

private:
  ofdpa::OfdpaStatus::OfdpaStatusCode
  ofdpaTunnelPortCreate(const ::ofdpa::TunnelPortCreate &request);

  std::unique_ptr<ofdpa::OfdpaRpc::Stub> stub_;
};

} // namespace basebox
