/*
 * crstp.hpp
 *
 *  Created on: 10.09.2014
 *      Author: andreas
 */

#ifndef CRSTP_P_H_
#define CRSTP_P_H_

namespace roflibs {
namespace eth {
namespace rstp {

// IEEE 802.1D-2004, section 9.2.9
enum port_role_t {
  PORT_ROLE_UNKNOWN = 0,
  PORT_ROLE_ALTERNATE = 1,
  PORT_ROLE_ROOT = 2,
  PORT_ROLE_DESIGNATED = 3,
};

enum bpdu_protocol_identifier_t {
  BPDU_PROT_ID = 0x0000,
};

enum bpdu_type_t {
  BPDU_TYPE_CONFIGURATION = 0x00,
  BPDU_TYPE_TOPOLOGY_CHANGE_NOTIFICATION = 0x80,
  BPDU_TYPE_RAPID_SPANNING_TREE = 0x02,
};

const unsigned int BPDU_LLC_ETH_ALEN = 6;
const uint8_t BPDU_LLC_ADDRESS =
    0x42; // IEEE 802.1D-2004 section 7.12.3 table 7-9
const uint8_t BPDU_LLC_CONTROL = 0x03;
const uint16_t BPDU_PROTOCOL_IDENTIFIER = 0x0000;
const uint8_t BPDU_VERSION = 0x00;

/**
 * @brief Struct defining the common header of a bridge PDU including Ethernet
 * and 802.2 header.
 */
typedef struct {
  uint8_t dl_dst[BPDU_LLC_ETH_ALEN];
  uint8_t dl_src[BPDU_LLC_ETH_ALEN];
  uint16_t dl_len;
  uint8_t llc_dsap;
  uint8_t llc_ssap;
  uint8_t llc_control;
  uint16_t bpdu_protid; // protocol identifier (= 0x0000)
  uint8_t bpdu_version; // protocol version (= 0x00)
  uint8_t bpdu_type;    // BPDU type (= 0x00)
  uint8_t data[0];
} __attribute__((packed)) bpdu_common_hdr_t;

// IEEE 802.1D-2004, section 9.3.1 => configuration BPDU
typedef struct bpdu_cnf_body_s {
  // BPDU type (= 0x00)
  uint8_t flags;
  uint64_t rootid;
  uint32_t rootpathcost;
  uint64_t bridgeid;
  uint16_t portid; // port identifier (not protid :)!
  uint16_t msgage;
  uint16_t maxage;
  uint16_t hellotime;
  uint16_t fwddelay;
} __attribute__((packed)) bpdu_cnf_body_t;

// IEEE 802.1D-2004, section 9.3.2 => topology change notification BPDU
typedef struct bpdu_tcn_body_s {
  // BPDU type (= 0x80)
} __attribute__((packed)) bpdu_tcn_body_t;

// IEEE 802.1D-2004, section 9.3.3 => RSTP BPDU
typedef struct bpdu_rst_body_s {
  // BPDU type (= 0x02)
  uint8_t flags;
  uint64_t rootid;
  uint32_t rootpathcost;
  uint64_t bridgeid;
  uint16_t portid; // port identifier (not protid :)!
  uint16_t msgage;
  uint16_t maxage;
  uint16_t hellotime;
  uint16_t fwddelay;
  uint8_t version_1_len; // (= 0x00)
} __attribute__((packed)) bpdu_rst_body_t;

/*
 * bridge PDU frame structs (hdr + payload)
 */

typedef struct bpdu_cnf_frame_s {
  bpdu_common_hdr_t hdr;
  bpdu_cnf_body_t bpdu;
} __attribute__((packed)) bpdu_cnf_frame_t;

typedef struct bpdu_tcn_frame_s {
  bpdu_common_hdr_t hdr;
  bpdu_tcn_body_t bpdu;
} __attribute__((packed)) bpdu_tcn_frame_t;

typedef struct bpdu_rst_frame_s {
  bpdu_common_hdr_t hdr;
  bpdu_rst_body_t bpdu;
} __attribute__((packed)) bpdu_rst_frame_t;

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CRSTP_P_H_ */
