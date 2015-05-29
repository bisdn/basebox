#ifndef SRC_ROFLIBS_OF_DPA_OFDPA_DATATYPES_H_
#define SRC_ROFLIBS_OF_DPA_OFDPA_DATATYPES_H_

/** Flow Table Id enumerator */
typedef enum
{
  OFDPA_FLOW_TABLE_ID_INGRESS_PORT      =  0,  /**< Ingress Port Table */
  OFDPA_FLOW_TABLE_ID_VLAN              = 10,  /**< VLAN Table */
  OFDPA_FLOW_TABLE_ID_TERMINATION_MAC   = 20,  /**< Termination MAC Table */
  OFDPA_FLOW_TABLE_ID_UNICAST_ROUTING   = 30,  /**< Unicast Routing Table */
  OFDPA_FLOW_TABLE_ID_MULTICAST_ROUTING = 40,  /**< Multicast Routing Table */
  OFDPA_FLOW_TABLE_ID_BRIDGING          = 50,  /**< Bridging Table */
  OFDPA_FLOW_TABLE_ID_ACL_POLICY        = 60,  /**< ACL Table */
} OFDPA_FLOW_TABLE_ID_t;


#endif /* SRC_ROFLIBS_OF_DPA_OFDPA_DATATYPES_H_ */
