/*
 * cdhcp_option_clientid.cc
 *
 *  Created on: 13.09.2013
 *      Author: andreas
 */

#include "cdhcp_option_clientid.h"

using namespace dhcpv6snoop;


cdhcp_option_clientid::cdhcp_option_clientid() :
		cdhcp_option((size_t)sizeof(struct dhcp_option_client_id_hdr_t)),
		hdr((struct dhcp_option_client_id_hdr_t*)somem())
{
	set_code(DHCP_OPTION_CLIENT_ID);
	set_length(sizeof(struct dhcp_option_client_id_hdr_t));
}



cdhcp_option_clientid::cdhcp_option_clientid(uint8_t *buf, size_t buflen) :
		cdhcp_option(buf, buflen),
		hdr((struct dhcp_option_client_id_hdr_t*)somem())
{
	unpack(buf, buflen);
}



cdhcp_option_clientid::~cdhcp_option_clientid()
{

}



cdhcp_option_clientid::cdhcp_option_clientid(const cdhcp_option_clientid& opt) :
		cdhcp_option(opt)
{
	*this = opt;
}



cdhcp_option_clientid&
cdhcp_option_clientid::operator= (const cdhcp_option_clientid& opt)
{
	if (this == &opt)
		return *this;

	cdhcp_option::operator= (opt);

	hdr = (struct dhcp_option_client_id_hdr_t*)somem();

	duid = opt.duid;

	return *this;
}



void
cdhcp_option_clientid::validate()
{
	cdhcp_option::validate();

	// TODO
}



uint8_t*
cdhcp_option_clientid::resize(size_t len)
{
	cdhcp_option::resize(len);

	hdr = (struct dhcp_option_client_id_hdr_t*)somem();

	return somem();
}



void
cdhcp_option_clientid::pack(uint8_t *buf, size_t buflen)
{
	if (buflen < length())
		throw eDhcpOptionTooShort();

	set_length(length());

	// pack the fix part of CLIENTID
	cdhcp_option::pack(buf, sizeof(struct dhcp_option_client_id_hdr_t));

	memcpy(hdr->data, duid.somem(), duid.memlen());
}



void
cdhcp_option_clientid::unpack(uint8_t *buf, size_t buflen)
{
	if (buflen < sizeof(struct dhcp_option_client_id_hdr_t))
		throw eDhcpOptionBadSyntax();

	cdhcp_option::unpack(buf, sizeof(struct dhcp_option_client_id_hdr_t));

	// the memory assignment operation above may have invalidated the old memory area,
	// so reset the hdr pointer as well
	hdr = (struct dhcp_option_client_id_hdr_t*)somem();

	duid = rofl::cmemory(hdr->data, be16toh(hdr->header.len));
}



size_t
cdhcp_option_clientid::length()
{
	return (memlen() + duid.memlen());
}



rofl::cmemory
cdhcp_option_clientid::get_duid() const
{
	return duid;
}



void
cdhcp_option_clientid::set_duid(rofl::cmemory const& duid)
{
	this->duid = duid;
}




