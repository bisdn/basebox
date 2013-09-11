/*
 * cdhcp_option_ia_pd.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcp_option_ia_pd.h"

using namespace dhcpv6snoop;


cdhcp_option_ia_pd::cdhcp_option_ia_pd(uint8_t *buf, size_t buflen) :
		cdhcp_option(buf, buflen),
		hdr((struct dhcp_option_ia_pd_hdr_t*)somem())
{

}



cdhcp_option_ia_pd::~cdhcp_option_ia_pd()
{

}



cdhcp_option_ia_pd::cdhcp_option_ia_pd(const cdhcp_option_ia_pd& opt) :
		cdhcp_option(opt)
{
	*this = opt;
}



cdhcp_option_ia_pd&
cdhcp_option_ia_pd::operator= (const cdhcp_option_ia_pd& opt)
{
	if (this == &opt)
		return *this;

	cdhcp_option::operator= (opt);

	hdr = (struct dhcp_option_ia_pd_hdr_t*)somem();

	return *this;
}



void
cdhcp_option_ia_pd::validate()
{
	cdhcp_option::validate();

	// TODO
}



uint8_t*
cdhcp_option_ia_pd::resize(size_t len)
{
	cdhcp_option::resize(len);

	hdr = (struct dhcp_option_ia_pd_hdr_t*)somem();

	return somem();
}



void
cdhcp_option_ia_pd::pack(uint8_t *buf, size_t buflen)
{
	if (buflen < length())
		throw eDhcpOptionTooShort();

	// pack the fix part of IA_PD
	cdhcp_option::pack(buf, buflen);

	// pack the list of variable IA_PD options
	// TODO: ...
}



void
cdhcp_option_ia_pd::unpack(uint8_t *buf, size_t buflen)
{
	if (buflen < sizeof(struct dhcp_option_ia_pd_hdr_t))
		throw eDhcpOptionSyntax();

	cdhcp_option::unpack(buf, sizeof(struct dhcp_option_ia_pd_hdr_t));

	// the memory assignment operation above may have invalidated the old memory area,
	// so reset the hdr pointer as well
	hdr = (struct dhcp_option_ia_pd_hdr_t*)somem();

	// unpack the variable size list of IA_PD options appended to fix header
	// TODO: ...
}



size_t
cdhcp_option_ia_pd::length()
{
	return (memlen() /* TODO: + ia_pd_options_list.length() */);
}



uint32_t
cdhcp_option_ia_pd::get_iaid() const
{
	return be32toh(hdr->iaid);
}



void
cdhcp_option_ia_pd::set_iaid(uint32_t iaid)
{
	hdr->iaid = htobe32(iaid);
}



uint32_t
cdhcp_option_ia_pd::get_t1() const
{
	return be32toh(hdr->t1);
}



void
cdhcp_option_ia_pd::set_t1(uint32_t t1)
{
	hdr->t1 = htobe32(t1);
}



uint32_t
cdhcp_option_ia_pd::get_t2() const
{
	return be32toh(hdr->t2);
}



void
cdhcp_option_ia_pd::set_t2(uint32_t t2)
{
	hdr->t2 = htobe32(t2);
}


