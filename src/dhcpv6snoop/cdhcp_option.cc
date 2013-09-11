/*
 * cdhcp_option.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcp_option.h"

using namespace dhcpv6snoop;



cdhcp_option::cdhcp_option(size_t len) :
		rofl::cmemory(len),
		hdr((struct dhcp_option_hdr_t*)somem())
{

}



cdhcp_option::~cdhcp_option()
{

}



cdhcp_option::cdhcp_option(uint8_t *buf, size_t buflen) :
		rofl::cmemory(buf, buflen),
		hdr((struct dhcp_option_hdr_t*)somem())
{

}



cdhcp_option::cdhcp_option(const cdhcp_option& opt) :
		rofl::cmemory((size_t)opt.memlen()),
		hdr((struct dhcp_option_hdr_t*)somem())
{
	*this = opt;
}



cdhcp_option&
cdhcp_option::operator= (const cdhcp_option& opt)
{
	if (this == &opt)
		return *this;

	rofl::cmemory::operator= (opt);

	hdr = (struct dhcp_option_hdr_t*)somem();

	return *this;
}



void
cdhcp_option::validate()
{

}



uint8_t*
cdhcp_option::resize(size_t len)
{
	rofl::cmemory::resize(len);

	hdr = (struct dhcp_option_hdr_t*)somem();

	return somem();
}



void
cdhcp_option::pack(uint8_t *buf, size_t buflen)
{
	if (buflen < length())
		throw eDhcpOptionTooShort();

	set_length(length());

	rofl::cmemory::pack(buf, buflen);
}



void
cdhcp_option::unpack(uint8_t *buf, size_t buflen)
{
	rofl::cmemory::assign(buf, buflen);

	// the memory assignment operation above may have invalidated the old memory area,
	// so reset the hdr pointer as well
	hdr = (struct dhcp_option_hdr_t*)somem();
}



size_t
cdhcp_option::length()
{
	return rofl::cmemory::memlen();
}



uint16_t
cdhcp_option::get_code() const
{
	return be16toh(hdr->code);
}



void
cdhcp_option::set_code(uint16_t code)
{
	hdr->code = htobe16(code);
}



uint16_t
cdhcp_option::get_length() const
{
	return be16toh(hdr->len);
}



void
cdhcp_option::set_length(uint16_t len)
{
	hdr->len = htobe16(len);
}


