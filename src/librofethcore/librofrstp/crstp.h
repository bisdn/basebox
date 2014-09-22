/*
 * crstp.hpp
 *
 *  Created on: 10.09.2014
 *      Author: andreas
 */

#ifndef CRSTP_HPP_
#define CRSTP_HPP_

namespace rofeth {
namespace rstp {

// IEEE 802.1D-2004, section 9.2.9
enum port_role_t {
	PORT_ROLE_UNKNOWN 		= 0,
	PORT_ROLE_ALTERNATE 	= 1,
	PORT_ROLE_ROOT 			= 2,
	PORT_ROLE_DESIGNATED 	= 3,
};

enum bpdu_protocol_identifier_t {
	BPDU_PROT_ID = 0x0000;
};

enum bpdu_type_t {
	BPDU_TYPE_CONFIGURATION 				= 0x00,
	BPDU_TYPE_TOPOLOGY_CHANGE_NOTIFICATION 	= 0x80,
	BPDU_TYPE_RAPID_SPANNING_TREE 			= 0x02,
};

#define BPDU_LLC_ETH_ALEN 	6
#define BPDU_ROOTID_LEN 	8
#define BPDU_BRIDGEID_LEN 	8
#define BPDU_LLC_ADDRESS	0x42

struct bpdu_eth_llc_hdr {
	uint8_t 	dl_dst[BPDU_LLC_ETH_ALEN];
	uint8_t 	dl_src[BPDU_LLC_ETH_ALEN];
	uint16_t 	dl_len;
	uint8_t 	dl_dsap;
	uint8_t 	dl_ssap;
	uint8_t 	dl_control;
	uint8_t 	dl_vendor_code[3];
	uint16_t 	dl_type;
	uint8_t 	data[0];
}__attribute__((packed));

// IEEE 802.1D-2004, section 9.3.1 => configuration BPDU
struct bpdu_config_frame {
	uint16_t 	protid;		// protocol identifier (= 0x0000)
	uint8_t 	version;	// protocol version (= 0x00)
	uint8_t 	type;		// BPDU type (= 0x00)
	uint8_t 	flags;
	uint8_t 	rootid[BPDU_ROOTID_LEN];
	uint32_t 	rootpathcost;
	uint8_t 	bridgeid[BPDU_BRIDGEID_LEN];
	uint16_t 	portid; 	// port identifier (not protid :)!
	uint16_t	msgage;
	uint16_t	maxage;
	uint16_t	hellotime;
	uint16_t	fwddelay;
}__attribute__((packed));

// IEEE 802.1D-2004, section 9.3.2 => topology change notification BPDU
struct bpdu_tcn_frame {
	uint16_t 	protid; 	// protocol identifier (= 0x0000)
	uint8_t 	version; 	// protocol version (= 0x00)
	uint8_t 	type;	 	// BPDU type (= 0x80)
}__attribute__((packed));

// IEEE 802.1D-2004, section 9.3.3 => RSTP BPDU
struct bpdu_rtsp_frame {
	uint16_t 	protid;		// protocol identifier (= 0x0000)
	uint8_t 	version;	// protocol version (= 0x00)
	uint8_t 	type;		// BPDU type (= 0x00)
	uint8_t 	flags;
	uint8_t 	rootid[BPDU_ROOTID_LEN];
	uint32_t 	rootpathcost;
	uint8_t 	bridgeid[BPDU_BRIDGEID_LEN];
	uint16_t 	portid; 	// port identifier (not protid :)!
	uint16_t	msgage;
	uint16_t	maxage;
	uint16_t	hellotime;
	uint16_t	fwddelay;
	uint8_t		version_1_len; // (= 0x00)
}__attribute__((packed));

struct {
	struct bpdu_eth_llc_hdr		llc;
	struct bpdu_config_frame	bpdu;
}__attribute__((packed)) bpdu_config_frame_t;

struct {
	struct bpdu_eth_llc_hdr		llc;
	struct bpdu_tcn_frame		bpdu;
}__attribute__((packed)) bpdu_tcn_frame_t;

struct {
	struct bpdu_eth_llc_hdr		llc;
	struct bpdu_rtsp_frame		bpdu;
}__attribute__((packed)) bpdu_rtsp_frame_t;

struct


}; // end of namespace rstp
}; // end of namespace rofeth

#endif /* CRSTP_HPP_ */
