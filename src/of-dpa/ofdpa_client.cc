// SPDX-FileCopyrightText: © 2018 BISDN GmbH
//
// SPDX-License-Identifier: MPL-2.0-no-copyleft-exception

#include <grpc++/grpc++.h>

#include "ofdpa_client.h"

using namespace ofdpa;
using namespace grpc;

namespace basebox {

ofdpa_client::ofdpa_client(std::shared_ptr<Channel> channel)
    : stub_(ofdpa::OfdpaRpc::NewStub(channel)) {}

OfdpaStatus::OfdpaStatusCode ofdpa_client::ofdpaTunnelReset() {
  ::Empty request;
  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelReset(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelTenantCreate(uint32_t tunnel_id, uint32_t vni) {
  // TODO maybe use ofdpa_datatypes as parameters

  ::TunnelTenantCreate request;
  request.set_tunnel_id(tunnel_id);

  ::OfdpaTunnelTenantConfig *config = request.mutable_config();
  config->set_proto(::OFDPA_TUNNEL_PROTO_VXLAN);
  config->set_virtual_network_id(vni);

  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelTenantCreate(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelTenantDelete(uint32_t tunnel_id) {

  ::ofdpa::TunnelId request;

  request.set_tunnel_id(tunnel_id);
  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelTenantDelete(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaPortSourceMacLearningSet(uint32_t port_num,
                                            uint32_t l2_learn) {
  ::ofdpa::PortSrcMacLearning request;
  ::ClientContext context;
  ::OfdpaStatus response;

  context.set_wait_for_ready(true);

  request.set_port_num(port_num);
  request.set_l2_learn(l2_learn);

  ::Status rv =
      stub_->ofdpaPortSourceMacLearningSet(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaPortSourceMacMoveLearningSet(uint32_t port_num,
                                                uint32_t l2_learn) {
  ::ofdpa::PortSrcMacLearning request;
  ::ClientContext context;
  ::OfdpaStatus response;

  context.set_wait_for_ready(true);

  request.set_port_num(port_num);
  request.set_l2_learn(l2_learn);

  ::Status rv =
      stub_->ofdpaPortSourceMacMoveLearningSet(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelNextHopCreate(uint32_t next_hop_id, uint64_t src_mac,
                                       uint64_t dst_mac, uint32_t physical_port,
                                       uint16_t vlan_id) {
  ::TunnelNextHopCreate request;
  request.set_next_hop_id(next_hop_id);
  ::OfdpaTunnelNextHopConfig *config = request.mutable_config();
  config->set_protocol(OFDPA_TUNNEL_PROTO_VXLAN);
  config->set_src_mac_addr(src_mac); // XXX validate?
  config->set_dst_mac_addr(dst_mac); // XXX validate?
  config->set_physical_port_num(physical_port);
  config->set_vlan_id(vlan_id); // XXX validate?

  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelNextHopCreate(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelNextHopDelete(uint32_t next_hop_id) {
  ::NextHopId request;
  request.set_next_hop_id(next_hop_id);

  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelNextHopDelete(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelNextHopModify(uint32_t next_hop_id, uint64_t src_mac,
                                       uint64_t dst_mac, uint32_t physical_port,
                                       uint16_t vlan_id) {
  ::TunnelNextHopCreate request;
  request.set_next_hop_id(next_hop_id);
  ::OfdpaTunnelNextHopConfig *config = request.mutable_config();
  config->set_protocol(OFDPA_TUNNEL_PROTO_VXLAN);
  config->set_src_mac_addr(src_mac); // XXX validate?
  config->set_dst_mac_addr(dst_mac); // XXX validate?
  config->set_physical_port_num(physical_port);
  config->set_vlan_id(vlan_id); // XXX validate?

  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelNextHopModify(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

TunnelPortCreate make_tunnel_port(uint32_t port_id,
                                  const std::string &port_name,
                                  OfdpaTunnelPortType type) {
  TunnelPortCreate tunnel_port;
  tunnel_port.set_port_num(port_id);
  tunnel_port.set_name(port_name);
  OfdpaTunnelPortConfig *config = tunnel_port.mutable_config();
  config->set_type(type);
  config->set_tunnel_proto(OFDPA_TUNNEL_PROTO_VXLAN);
  return tunnel_port;
}

OfdpaAccessPortConfig make_access_port_config(uint32_t physical_port,
                                              uint32_t vlan_id, bool untagged) {
  ::OfdpaAccessPortConfig config;
  config.set_pysical_port_num(physical_port);
  config.set_vlan_id(vlan_id);
  config.set_untagged(untagged);
  return config;
}

OfdpaEndpointConfig make_endpoint_config(uint32_t remote, uint32_t local,
                                         uint32_t ttl, uint32_t next_hop_id,
                                         uint32_t terminator_udp_dst_port,
                                         uint32_t initiator_udp_dst_port,
                                         uint32_t udp_src_port_if_no_entropy,
                                         bool use_entropy) {
  OfdpaEndpointConfig config;
  config.set_remote_endpoint(remote);
  config.set_local_endpoint(local);
  config.set_ttl(ttl);
  config.set_next_hop_id(next_hop_id);

  OfdpaVxlanProtoInfo *info = config.mutable_info()->mutable_vxlan_info();
  info->set_initiator_udp_dst_port(initiator_udp_dst_port);   /* remote port */
  info->set_terminator_udp_dst_port(terminator_udp_dst_port); /* local port;
                                                                 must be the
                                                                 same for all
                                                                 tunnels */
  info->set_udp_src_port_if_no_entropy(0);
  info->set_use_entropy(
      use_entropy); /* must be the same accross all vxlan tunnels */

  return config;
}

OfdpaStatus::OfdpaStatusCode ofdpa_client::ofdpaTunnelAccessPortCreate(
    uint32_t port_id, const std::string &port_name, uint32_t physical_port,
    uint16_t vlan_id, bool untagged) {
  // XXX TODO check parameters
  TunnelPortCreate request =
      make_tunnel_port(port_id, port_name, OFDPA_TUNNEL_PORT_TYPE_ACCESS);

  request.mutable_config()
      ->mutable_config()
      ->mutable_access_port_config()
      ->CopyFrom(make_access_port_config(physical_port, vlan_id, untagged));

  return ofdpaTunnelPortCreate(request);
}

OfdpaStatus::OfdpaStatusCode ofdpa_client::ofdpaTunnelEndpointPortCreate(
    uint32_t port_id, const std::string &port_name, uint32_t remote_ipv4,
    uint32_t local_ipv4, uint32_t ttl, uint32_t next_hop_id,
    uint32_t terminator_udp_dst_port, uint32_t initiator_udp_dst_port,
    uint32_t udp_src_port_if_no_entropy, bool use_entropy) {
  // XXX TODO check parameters
  TunnelPortCreate request =
      make_tunnel_port(port_id, port_name, OFDPA_TUNNEL_PORT_TYPE_ENDPOINT);
  request.mutable_config()
      ->mutable_config()
      ->mutable_endpoint_config()
      ->CopyFrom(make_endpoint_config(
          remote_ipv4, local_ipv4, ttl, next_hop_id, terminator_udp_dst_port,
          initiator_udp_dst_port, udp_src_port_if_no_entropy, use_entropy));

  return ofdpaTunnelPortCreate(request);
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelPortCreate(const ::ofdpa::TunnelPortCreate &request) {
  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelPortCreate(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelPortDelete(uint32_t lport_id) {
  ::PortNum request;
  request.set_port_num(lport_id);
  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelPortDelete(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelPortTenantAdd(uint32_t port_id, uint32_t tunnel_id) {
  ::TunnelPortTenantAdd request;

  request.set_port_num(port_id);
  request.set_tunnel_id(tunnel_id);

  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaTunnelPortTenantAdd(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTunnelPortTenantDelete(uint32_t port_id,
                                          uint32_t tunnel_id) {
  ::TunnelPortTenantAdd request;

  request.set_port_num(port_id);
  request.set_tunnel_id(tunnel_id);

  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv =
      stub_->ofdpaTunnelPortTenantDelete(&context, request, &response);

  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

OfdpaStatus::OfdpaStatusCode ofdpa_client::ofdpaStgReset() {
  ::Empty request;
  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  ::Status rv = stub_->ofdpaStgReset(&context, request, &response);

  if (not rv.ok()) {
    // LOG status
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaStgStatePortSet(uint32_t port_id, std::string state,
                                   uint32_t stg_id) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::StpInterfaceState request;

  context.set_wait_for_ready(true);

  request.set_port_num(port_id);
  request.set_port_state(state);
  request.mutable_stg_id()->set_id(stg_id);

  ::Status rv = stub_->ofdpaStgStatePortSet(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaStgCreate(uint16_t stg_id) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::StgId request;

  context.set_wait_for_ready(true);
  request.set_id(stg_id);

  ::Status rv = stub_->ofdpaStgCreate(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaStgDestroy(uint16_t stg_id) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::StgId request;

  context.set_wait_for_ready(true);
  request.set_id(stg_id);

  ::Status rv = stub_->ofdpaStgDestroy(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaStgVlanAdd(uint16_t vlanid, uint32_t stgid) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::StgVlan request;

  context.set_wait_for_ready(true);
  request.mutable_stg_id()->set_id(stgid);
  request.mutable_vlan_id()->set_id(vlanid);

  ::Status rv = stub_->ofdpaStgVlanAdd(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaStgVlanRemove(uint16_t vlan_id, uint32_t stg_id) {
  ::OfdpaStatus response;
  ::ClientContext context;

  context.set_wait_for_ready(true);

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::OfdpaTrunkCreate(uint32_t lag_id, std::string name,
                               uint8_t mode) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::TrunkCreate request;

  context.set_wait_for_ready(true);

  request.set_name(name);
  request.set_lag_id(lag_id);
  request.set_lag_type((::LagType)mode);

  ::Status rv = stub_->ofdpaTrunkCreate(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::OfdpaTrunkDelete(uint32_t lag_id) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::PortNum request;

  context.set_wait_for_ready(true);

  request.set_port_num(lag_id);

  ::Status rv = stub_->ofdpaTrunkDelete(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::OfdpaPortTrunkGroupSet(uint32_t port_id, uint32_t trunk_id) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::TrunkGroupSet request;

  context.set_wait_for_ready(true);

  request.set_lag_id(trunk_id);
  request.set_member(port_id);

  ::Status rv = stub_->ofdpaPortTrunkGroupSet(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::OfdpaTrunkPortMemberActiveSet(uint32_t port_id, uint32_t trunk_id,
                                            uint32_t active) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::PortMemberActiveSet request;

  context.set_wait_for_ready(true);

  request.set_lag_id(trunk_id);
  request.set_member(port_id);
  request.set_active(active);

  ::Status rv =
      stub_->ofdpaTrunkPortMemberActiveSet(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode
ofdpa_client::ofdpaTrunkPortPSCSet(uint32_t lag_id, uint8_t mode) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::PSC request;

  context.set_wait_for_ready(true);

  request.set_lag_id(lag_id);
  request.set_lag_type((::LagType)mode);

  ::Status rv = stub_->ofdpaTrunkPortPSCSet(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

ofdpa::OfdpaStatus::OfdpaStatusCode ofdpa_client::ofdpaRxRateSet(int32_t pps) {
  ::OfdpaStatus response;
  ::ClientContext context;
  ::Pps request;

  context.set_wait_for_ready(true);

  request.set_pps(pps);

  ::Status rv = stub_->ofdpaRxRateSet(&context, request, &response);
  if (not rv.ok()) {
    return ofdpa::OfdpaStatus::OFDPA_E_RPC;
  }

  return response.status();
}

} // namespace basebox
