/*
 * cbpdumsg.cpp
 *
 *  Created on: 03.01.2015
 *      Author: andreas
 */

#include "cbpdumsg.hpp"

using namespace roflibs::eth::rstp;

void
cbpdumsg::pack(
		uint8_t* buf,
		size_t buflen)
{
	if (buflen < cbpdumsg::length()) {
		throw exception::eBpduInval("cbpdumsg::pack() buflen too short");
	}

	bpdu_common_hdr_t* hdr = (bpdu_common_hdr_t*)buf;

	eth_dst.pack(hdr->dl_dst, BPDU_LLC_ETH_ALEN);
	eth_src.pack(hdr->dl_src, BPDU_LLC_ETH_ALEN);
	hdr->dl_len = htobe16(length()); // overloaded method
	hdr->llc_ssap = get_llc_ssap();
	hdr->llc_dsap = get_llc_dsap();
	hdr->llc_control = get_llc_control();
	hdr->bpdu_protid = htobe16(get_bpdu_protid());
	hdr->bpdu_version = get_bpdu_version();
	hdr->bpdu_type = get_bpdu_type();
}



void
cbpdumsg::unpack(
		uint8_t* buf,
		size_t buflen)
{
	if (buflen < cbpdumsg::length()) {
		throw exception::eBpduInval("cbpdumsg::unpack() buflen too short");
	}

	bpdu_common_hdr_t* hdr = (bpdu_common_hdr_t*)buf;

	if (be16toh(hdr->dl_len) < cbpdumsg::length()) {
		throw exception::eBpduInval("cbpdumsg::unpack() dl_len too short");
	}

	eth_dst.unpack(hdr->dl_dst, BPDU_LLC_ETH_ALEN);
	eth_src.unpack(hdr->dl_src, BPDU_LLC_ETH_ALEN);
	set_llc_ssap(hdr->llc_ssap);
	set_llc_dsap(hdr->llc_dsap);
	set_llc_control(hdr->llc_control);
	set_bpdu_protid(be16toh(hdr->bpdu_protid));
	set_bpdu_version(hdr->bpdu_version);
	set_bpdu_type(hdr->bpdu_version);
}


