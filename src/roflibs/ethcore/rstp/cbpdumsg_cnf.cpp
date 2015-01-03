/*
 * cbpdumsg_cnf.cpp
 *
 *  Created on: 03.01.2015
 *      Author: andreas
 */

#include "cbpdumsg_cnf.hpp"

using namespace roflibs::eth::rstp;


void
cbpdumsg_cnf::pack(
		uint8_t* buf,
		size_t buflen)
{
	if (buflen < cbpdumsg_cnf::length()) {
		throw exception::eBpduInval("cbpdumsg_cnf::pack() buflen too short");
	}

	cbpdumsg::pack(buf, buflen);

	bpdu_cnf_frame_t* hdr = (bpdu_cnf_frame_t*)buf;

	hdr->bpdu.flags = get_flags();
	hdr->bpdu.rootid = htobe64(get_rootid().get_bridge_id());
	hdr->bpdu.rootpathcost = htobe32(get_root_path_cost().get_rpcost());
	hdr->bpdu.bridgeid = htobe64(get_bridgeid().get_bridge_id());
	hdr->bpdu.portid = htobe16(get_portid().get_portid());
	hdr->bpdu.msgage = htobe16(get_message_age());
	hdr->bpdu.maxage = htobe16(get_max_age());
	hdr->bpdu.hellotime = htobe16(get_hello_time());
	hdr->bpdu.fwddelay = htobe16(get_forward_delay());
}



void
cbpdumsg_cnf::unpack(
		uint8_t* buf,
		size_t buflen)
{
	if (buflen < cbpdumsg_cnf::length()) {
		throw exception::eBpduInval("cbpdumsg_cnf::unpack() buflen too short");
	}

	cbpdumsg::unpack(buf, buflen);

	bpdu_cnf_frame_t* hdr = (bpdu_cnf_frame_t*)buf;

	set_flags(hdr->bpdu.flags);
	set_rootid(cbridgeid(be64toh(hdr->bpdu.rootid)));
	set_root_path_cost(crpcost(be32toh(hdr->bpdu.rootpathcost)));
	set_bridgeid(cbridgeid(be64toh(hdr->bpdu.bridgeid)));
	set_portid(cportid(be16toh(hdr->bpdu.portid)));
	set_message_age(be16toh(hdr->bpdu.msgage));
	set_max_age(be16toh(hdr->bpdu.maxage));
	set_hello_time(be16toh(hdr->bpdu.hellotime));
	set_forward_delay(be16toh(hdr->bpdu.fwddelay));
}


