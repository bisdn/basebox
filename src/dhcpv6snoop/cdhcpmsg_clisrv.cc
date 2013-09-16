/*
 * cdhcpmsg_clisrv.cc
 *
 *  Created on: 13.09.2013
 *      Author: andreas
 */

#include "cdhcpmsg_clisrv.h"

using namespace dhcpv6snoop;


cdhcpmsg_clisrv::cdhcpmsg_clisrv() :
		cdhcpmsg((size_t)sizeof(struct dhcpmsg_clisrv_hdr_t)),
		hdr((struct dhcpmsg_clisrv_hdr_t*)somem())
{

}



cdhcpmsg_clisrv::~cdhcpmsg_clisrv()
{

}



cdhcpmsg_clisrv::cdhcpmsg_clisrv(const cdhcpmsg_clisrv& msg) :
		cdhcpmsg(msg)
{
	*this = msg;
}



cdhcpmsg_clisrv&
cdhcpmsg_clisrv::operator= (const cdhcpmsg_clisrv& msg)
{
	if (this == &msg)
		return *this;

	cdhcpmsg::operator= (msg);

	hdr = (struct dhcpmsg_clisrv_hdr_t*)somem();

	return *this;
}



cdhcpmsg_clisrv::cdhcpmsg_clisrv(uint8_t *buf, size_t buflen) :
		cdhcpmsg(buflen)
{
	unpack(buf, buflen);
}



size_t
cdhcpmsg_clisrv::length()
{
	return (memlen() + options_length());
}



void
cdhcpmsg_clisrv::pack(uint8_t *buf, size_t buflen)
{
	if (buflen < length())
		throw eDhcpMsgTooShort();

	memcpy(buf, somem(), sizeof(struct dhcpmsg_clisrv_hdr_t));

	// append options
	pack_options(buf + sizeof(struct dhcpmsg_clisrv_hdr_t), buflen - sizeof(dhcpmsg_clisrv_hdr_t));
}



void
cdhcpmsg_clisrv::unpack(uint8_t *buf, size_t buflen)
{
	if (buflen < sizeof(struct dhcpmsg_clisrv_hdr_t))
		throw eDhcpMsgBadSyntax();

	resize(buflen);

	cdhcpmsg::unpack(buf, sizeof(struct dhcpmsg_clisrv_hdr_t));

	hdr = (struct dhcpmsg_clisrv_hdr_t*)somem();

	// parse options
	unpack_options(buf + sizeof(struct dhcpmsg_clisrv_hdr_t), buflen - sizeof(dhcpmsg_clisrv_hdr_t));
}



uint8_t*
cdhcpmsg_clisrv::resize(size_t len)
{
	cdhcpmsg::resize(len);

	hdr = (struct dhcpmsg_clisrv_hdr_t*)somem();

	return somem();
}



void
cdhcpmsg_clisrv::validate()
{
	if (memlen() < sizeof(struct dhcpmsg_clisrv_hdr_t))
		throw eDhcpMsgBadSyntax();
}



uint32_t
cdhcpmsg_clisrv::get_transaction_id() const
{
	return ((hdr->transaction_id[0] << 16) +
			(hdr->transaction_id[1] <<  8) +
			(hdr->transaction_id[2] <<  0));
}



void
cdhcpmsg_clisrv::set_transaction_id(uint32_t transaction_id)
{
	hdr->transaction_id[0] = ((transaction_id & 0x00ff0000) >> 16);
	hdr->transaction_id[1] = ((transaction_id & 0x0000ff00) >>  8);
	hdr->transaction_id[2] = ((transaction_id & 0x000000ff) >>  0);
}


