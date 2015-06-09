#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_DATATYPES_H_
#define SRC_ROFLIBS_OF_DPA_OFDPA_DATATYPES_H_

/** Flow Table Id enumerator */
typedef enum
{
  OFDPA_FLOW_TABLE_ID_INGRESS_PORT                      =    0,  /**< Ingress Port Table */
  OFDPA_FLOW_TABLE_ID_PORT_DSCP_TRUST                   =    5,  /**< Port DSCP Trust Table */
  OFDPA_FLOW_TABLE_ID_PORT_PCP_TRUST                    =    6,  /**< Port PCP Trust Table */
  OFDPA_FLOW_TABLE_ID_TUNNEL_DSCP_TRUST                 =    7,  /**< Tunnel DSCP Trust Table */
  OFDPA_FLOW_TABLE_ID_TUNNEL_PCP_TRUST                  =    8,  /**< Tunnel PCP Trust Table */
  OFDPA_FLOW_TABLE_ID_VLAN                              =   10,  /**< VLAN Table */
  OFDPA_FLOW_TABLE_ID_VLAN_1                            =   11,  /**< VLAN 1 Table */
  OFDPA_FLOW_TABLE_ID_MAINTENANCE_POINT                 =   12,  /**< Maintenance Point Flow Table */
  OFDPA_FLOW_TABLE_ID_MPLS_L2_PORT                      =   13,  /**< MPLS L2 Port Table */
  OFDPA_FLOW_TABLE_ID_MPLS_DSCP_TRUST                   =   15,  /**< MPLS QoS DSCP Trust Table */
  OFDPA_FLOW_TABLE_ID_MPLS_PCP_TRUST                    =   16,  /**< MPLS QoS PCP Trust Table */
  OFDPA_FLOW_TABLE_ID_TERMINATION_MAC                   =   20,  /**< Termination MAC Table */
  OFDPA_FLOW_TABLE_ID_MPLS_0                            =   23,  /**< MPLS 0 Table */
  OFDPA_FLOW_TABLE_ID_MPLS_1                            =   24,  /**< MPLS 1 Table */
  OFDPA_FLOW_TABLE_ID_MPLS_2                            =   25,  /**< MPLS 2 Table */
  OFDPA_FLOW_TABLE_ID_MPLS_MAINTENANCE_POINT            =   26,  /**< MPLS-TP Maintenance Point Flow Table */
  OFDPA_FLOW_TABLE_ID_BFD                               =   27,  /**< BFD Table */
  OFDPA_FLOW_TABLE_ID_UNICAST_ROUTING                   =   30,  /**< Unicast Routing Table */
  OFDPA_FLOW_TABLE_ID_MULTICAST_ROUTING                 =   40,  /**< Multicast Routing Table */
  OFDPA_FLOW_TABLE_ID_BRIDGING                          =   50,  /**< Bridging Table */
  OFDPA_FLOW_TABLE_ID_ACL_POLICY                        =   60,  /**< ACL Table */
  OFDPA_FLOW_TABLE_ID_EGRESS_VLAN                       =  210,  /**< Egress VLAN Table */
  OFDPA_FLOW_TABLE_ID_EGRESS_VLAN_1                     =  211,  /**< Egress VLAN 1 Table */
  OFDPA_FLOW_TABLE_ID_EGRESS_MAINTENANCE_POINT          =  226,  /**< Egress Maintenance Point Flow Table */
  OFDPA_FLOW_TABLE_ID_L2_INTF_DSCP_REMARK               =  237,  /**< L2 Interface DSCP Remark Table */
  OFDPA_FLOW_TABLE_ID_L2_INTF_8021P_PRI_REMARK          =  238,  /**< L2 Interface 802.1p PRI Remark Table */
  OFDPA_FLOW_TABLE_ID_MPLS_QOS                          =  239,  /**< MPLS QoS Table */
  OFDPA_FLOW_TABLE_ID_MPLS_VPN_LBL_EXP_REMARK           =  240,  /**< MPLS VPN Label EXP Remark Table */
  OFDPA_FLOW_TABLE_ID_MPLS_VPN_LBL_8021P_PRI_REMARK     =  241,  /**< MPLS VPN Label 802.1p PRI Remark Table */
  OFDPA_FLOW_TABLE_ID_MPLS_TUNNEL_LBL_EXP_REMARK        =  242,  /**< MPLS Tunnel Label EXP Remark Table */
  OFDPA_FLOW_TABLE_ID_MPLS_TUNNEL_LBL_8021P_PRI_REMARK  =  243,  /**< MPLS Tunnel Label 802.1p PRI Remark Table */

} OFDPA_FLOW_TABLE_ID_t;

/** Source MAC Lookup Table */
#define OFDPA_FLOW_TABLE_ID_SA_LOOKUP 254

#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_DATATYPES_H_ */
