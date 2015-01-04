/*
 * cbpdumsg_rst.cpp
 *
 *  Created on: 03.01.2015
 *      Author: andreas
 */

#include "cbpdumsg_rst.hpp"

using namespace roflibs::eth::rstp;


void
cbpdumsg_rst::pack(
		uint8_t* buf,
		size_t buflen)
{
	if (buflen < cbpdumsg_rst::length()) {
		throw exception::eBpduInval("cbpdumsg_rst::pack() buflen too short");
	}

	cbpdumsg::pack(buf, buflen);

	set_bpdu_type(BPDU_TYPE_RAPID_SPANNING_TREE);

	bpdu_rst_frame_t* hdr = (bpdu_rst_frame_t*)buf;

	hdr->bpdu.flags = get_flags();
	hdr->bpdu.rootid = htobe64(get_rootid().get_bridge_id());
	hdr->bpdu.rootpathcost = htobe32(get_root_path_cost().get_rpcost());
	hdr->bpdu.bridgeid = htobe64(get_bridgeid().get_bridge_id());
	hdr->bpdu.portid = htobe16(get_portid().get_portid());
	hdr->bpdu.msgage = htobe16(get_message_age());
	hdr->bpdu.maxage = htobe16(get_max_age());
	hdr->bpdu.hellotime = htobe16(get_hello_time());
	hdr->bpdu.fwddelay = htobe16(get_forward_delay());
	hdr->bpdu.version_1_len = get_version1_length();
}



void
cbpdumsg_rst::unpack(
		uint8_t* buf,
		size_t buflen)
{
	if (buflen < cbpdumsg_rst::length()) {
		throw exception::eBpduInval("cbpdumsg_rst::unpack() buflen too short");
	}

	cbpdumsg::unpack(buf, buflen);

	if (BPDU_TYPE_RAPID_SPANNING_TREE != get_bpdu_type()) {
		throw exception::eBpduInval("cbpdumsg_rst::unpack() invalid BPDU type");
	}

	bpdu_rst_frame_t* hdr = (bpdu_rst_frame_t*)buf;

	set_flags(hdr->bpdu.flags);
	set_rootid(cbridgeid(be64toh(hdr->bpdu.rootid)));
	set_root_path_cost(crpcost(be32toh(hdr->bpdu.rootpathcost)));
	set_bridgeid(cbridgeid(be64toh(hdr->bpdu.bridgeid)));
	set_portid(cportid(be16toh(hdr->bpdu.portid)));
	set_message_age(be16toh(hdr->bpdu.msgage));
	set_max_age(be16toh(hdr->bpdu.maxage));
	set_hello_time(be16toh(hdr->bpdu.hellotime));
	set_forward_delay(be16toh(hdr->bpdu.fwddelay));
	set_version1_length(hdr->bpdu.version_1_len);
}





