/*
 * cdhcpmsg.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcpmsg.h"

using namespace dhcpv6snoop;


cdhcpmsg::cdhcpmsg(size_t len) :
		rofl::cmemory(len),
		hdr((struct dhcpmsg_hdr_t*)somem())
{

}



cdhcpmsg::~cdhcpmsg()
{

}



cdhcpmsg::cdhcpmsg(const cdhcpmsg& msg)
{
	*this = msg;
}



cdhcpmsg&
cdhcpmsg::operator= (const cdhcpmsg& msg)
{
	if (this == &msg)
		return *this;

	rofl::cmemory::operator= (msg);

	hdr = (struct dhcpmsg_hdr_t*)somem();

	return *this;
}



cdhcpmsg::cdhcpmsg(uint8_t *buf, size_t buflen) :
		rofl::cmemory(buflen)
{
	unpack(buf, buflen);
}



uint8_t
cdhcpmsg::get_msg_type() const
{
	return hdr->msg_type;
}



void
cdhcpmsg::set_msg_type(uint8_t msg_type)
{
	hdr->msg_type = msg_type;
}



size_t
cdhcpmsg::length()
{
	return memlen();
}



void
cdhcpmsg::pack(uint8_t *buf, size_t buflen)
{
	rofl::cmemory::assign(buf, buflen);
}



void
cdhcpmsg::unpack(uint8_t *buf, size_t buflen)
{
	if (buflen < sizeof(struct dhcpmsg_hdr_t))
		throw eDhcpMsgTooShort();

	resize(buflen);

	rofl::cmemory::assign(buf, sizeof(struct dhcpmsg_hdr_t));

	hdr = (struct dhcpmsg_hdr_t*)somem();
}



uint8_t*
cdhcpmsg::resize(size_t len)
{
	rofl::cmemory::resize(len);

	hdr = (struct dhcpmsg_hdr_t*)somem();

	return somem();
}



void
cdhcpmsg::validate()
{
	if (memlen() < sizeof(struct dhcpmsg_hdr_t))
		throw eDhcpMsgBadSyntax();
}


