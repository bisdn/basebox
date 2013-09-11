/*
 * cdhcp_option_ia_pd_option_prefix.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcp_option_ia_pd_option_prefix.h"

using namespace dhcpv6snoop;



cdhcp_option_ia_pd_option_prefix::cdhcp_option_ia_pd_option_prefix() :
		cdhcp_option((size_t)sizeof(struct dhcp_option_ia_pd_option_prefix_hdr_t)),
		hdr((struct dhcp_option_ia_pd_option_prefix_hdr_t*)somem())
{
	set_code(DHCP_OPTION_IA_PREFIX);
	set_length(sizeof(struct dhcp_option_ia_pd_option_prefix_hdr_t)); // ATTENTION: DO NOT FORGET THE IA-PREFIX-OPTIONS AT THE END!!!
}



cdhcp_option_ia_pd_option_prefix::cdhcp_option_ia_pd_option_prefix(
		uint8_t *buf, size_t buflen) :
		cdhcp_option(buf, buflen),
		hdr((struct dhcp_option_ia_pd_option_prefix_hdr_t*)somem())
{

}



cdhcp_option_ia_pd_option_prefix::~cdhcp_option_ia_pd_option_prefix()
{

}



cdhcp_option_ia_pd_option_prefix::cdhcp_option_ia_pd_option_prefix(
		const cdhcp_option_ia_pd_option_prefix& opt) :
				cdhcp_option(opt)
{
	*this = opt;
}



cdhcp_option_ia_pd_option_prefix&
cdhcp_option_ia_pd_option_prefix::operator= (
		const cdhcp_option_ia_pd_option_prefix& opt)
{
	if (this == &opt)
		return *this;

	cdhcp_option::operator= (opt);

	hdr = (struct dhcp_option_ia_pd_option_prefix_hdr_t*)somem();

	// TODO: assign options as well (are there any specified anywhere?)

	return *this;
}



void
cdhcp_option_ia_pd_option_prefix::validate()
{
	cdhcp_option::validate();

	// TODO
}



uint8_t*
cdhcp_option_ia_pd_option_prefix::resize(size_t len)
{
	cdhcp_option::resize(len);

	hdr = (struct dhcp_option_ia_pd_option_prefix_hdr_t*)somem();

	return somem();
}



void
cdhcp_option_ia_pd_option_prefix::pack(uint8_t *buf, size_t buflen)
{
	if (buflen < length())
		throw eDhcpOptionTooShort();

	// pack the fix part of IA_PD
	cdhcp_option::pack(buf, buflen);

	// pack the list of variable IA_PD options
	// TODO: ...
}



void
cdhcp_option_ia_pd_option_prefix::unpack(uint8_t *buf, size_t buflen)
{
	if (buflen < sizeof(struct dhcp_option_ia_pd_option_prefix_hdr_t))
		throw eDhcpOptionBadSyntax();

	cdhcp_option::unpack(buf, sizeof(struct dhcp_option_ia_pd_option_prefix_hdr_t));

	// the memory assignment operation above may have invalidated the old memory area,
	// so reset the hdr pointer as well
	hdr = (struct dhcp_option_ia_pd_option_prefix_hdr_t*)somem();

	// unpack the variable size list of IA_PD options appended to fix header
	// TODO: ...
}




size_t
cdhcp_option_ia_pd_option_prefix::length()
{
	return (memlen() /* TODO: + ia_pd_option_prefix_options_list.length() */);
}



uint32_t
cdhcp_option_ia_pd_option_prefix::get_preferred_lifetime() const
{
	return be32toh(hdr->preferred_lifetime);
}



void
cdhcp_option_ia_pd_option_prefix::set_preferred_lifetime(uint32_t preferred_lifetime)
{
	hdr->preferred_lifetime = htobe32(preferred_lifetime);
}



uint32_t
cdhcp_option_ia_pd_option_prefix::get_valid_lifetime() const
{
	return be32toh(hdr->valid_lifetime);
}



void
cdhcp_option_ia_pd_option_prefix::set_valid_lifetime(uint32_t valid_lifetime)
{
	hdr->valid_lifetime = htobe32(valid_lifetime);
}



uint8_t
cdhcp_option_ia_pd_option_prefix::get_prefixlen() const
{
	return hdr->prefixlen;
}



void
cdhcp_option_ia_pd_option_prefix::set_prefixlen(uint8_t prefixlen)
{
	hdr->prefixlen = prefixlen;
}



rofl::caddress
cdhcp_option_ia_pd_option_prefix::get_prefix() const
{
	rofl::caddress addr(AF_INET6, "::");

	memcpy(addr.ca_s6addr->sin6_addr.s6_addr, hdr->prefix, 16);

	return addr;
}



void
cdhcp_option_ia_pd_option_prefix::set_prefix(rofl::caddress const& prefix)
{
	if (AF_INET6 != prefix.get_family())
		throw eDhcpOptionInval();

	memcpy(hdr->prefix, prefix.ca_s6addr->sin6_addr.s6_addr, 16);
}



