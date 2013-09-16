/*
 * cdhcpmsg_relay.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcpmsg_relay.h"

using namespace dhcpv6snoop;



cdhcpmsg_relay::cdhcpmsg_relay() :
		cdhcpmsg((size_t)sizeof(struct dhcpmsg_relay_hdr_t)),
		hdr((struct dhcpmsg_relay_hdr_t*)somem())
{

}



cdhcpmsg_relay::~cdhcpmsg_relay()
{

}



cdhcpmsg_relay::cdhcpmsg_relay(const cdhcpmsg_relay& msg) :
		cdhcpmsg(msg)
{
	*this = msg;
}



cdhcpmsg_relay&
cdhcpmsg_relay::operator= (const cdhcpmsg_relay& msg)
{
	if (this == &msg)
		return *this;

	cdhcpmsg::operator= (msg);

	hdr = (struct dhcpmsg_relay_hdr_t*)somem();

	return *this;
}



cdhcpmsg_relay::cdhcpmsg_relay(uint8_t *buf, size_t buflen) :
		cdhcpmsg(buflen)
{
	unpack(buf, buflen);
}



size_t
cdhcpmsg_relay::length()
{
	return (memlen() + options_length());
}



void
cdhcpmsg_relay::pack(uint8_t *buf, size_t buflen)
{
	if (buflen < length())
		throw eDhcpMsgTooShort();

	memcpy(buf, somem(), sizeof(struct dhcpmsg_relay_hdr_t));

	// append options
	pack_options(buf + sizeof(struct dhcpmsg_relay_hdr_t), buflen - sizeof(dhcpmsg_relay_hdr_t));
}



void
cdhcpmsg_relay::unpack(uint8_t *buf, size_t buflen)
{
	if (buflen < sizeof(struct dhcpmsg_relay_hdr_t))
		throw eDhcpMsgBadSyntax();

	resize(buflen);

	cdhcpmsg::unpack(buf, sizeof(struct dhcpmsg_relay_hdr_t));

	hdr = (struct dhcpmsg_relay_hdr_t*)somem();

	// parse options
	unpack_options(buf + sizeof(struct dhcpmsg_relay_hdr_t), buflen - sizeof(dhcpmsg_relay_hdr_t));
}



uint8_t*
cdhcpmsg_relay::resize(size_t len)
{
	cdhcpmsg::resize(len);

	hdr = (struct dhcpmsg_relay_hdr_t*)somem();

	return somem();
}



void
cdhcpmsg_relay::validate()
{
	if (memlen() < sizeof(struct dhcpmsg_relay_hdr_t))
		throw eDhcpMsgBadSyntax();
}



uint8_t
cdhcpmsg_relay::get_hop_count() const
{
	return hdr->hop_count;
}



void
cdhcpmsg_relay::set_hop_count(uint8_t hop_count)
{
	hdr->hop_count = hop_count;
}



rofl::caddress
cdhcpmsg_relay::get_link_address() const
{
	rofl::caddress addr(AF_INET6, "::");

	memcpy(addr.ca_s6addr->sin6_addr.s6_addr, hdr->link_address, 16);

	return addr;
}



void
cdhcpmsg_relay::set_link_address(rofl::caddress const& link_address)
{
	if (AF_INET6 != link_address.get_family())
		throw eDhcpMsgInval();

	memcpy(hdr->link_address, link_address.ca_s6addr->sin6_addr.s6_addr, 16);
}



rofl::caddress
cdhcpmsg_relay::get_peer_address() const
{
	rofl::caddress addr(AF_INET6, "::");

	memcpy(addr.ca_s6addr->sin6_addr.s6_addr, hdr->peer_address, 16);

	return addr;
}



void
cdhcpmsg_relay::set_peer_address(rofl::caddress const& peer_address)
{
	if (AF_INET6 != peer_address.get_family())
		throw eDhcpMsgInval();

	memcpy(hdr->peer_address, peer_address.ca_s6addr->sin6_addr.s6_addr, 16);
}








