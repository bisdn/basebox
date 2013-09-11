/*
 * cdhcp_option_ia_pd.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcp_option_ia_pd.h"

using namespace dhcpv6snoop;



cdhcp_option_ia_pd::cdhcp_option_ia_pd() :
		cdhcp_option((size_t)sizeof(struct dhcp_option_ia_pd_hdr_t)),
		hdr((struct dhcp_option_ia_pd_hdr_t*)somem())
{
	set_code(DHCP_OPTION_IA_PD);
	set_length(sizeof(struct dhcp_option_ia_pd_hdr_t)); // Attention: DO NOT FORGET THE IA_PD-OPTIONS IN THE END!!!
}



cdhcp_option_ia_pd::cdhcp_option_ia_pd(uint8_t *buf, size_t buflen) :
		cdhcp_option(buf, buflen),
		hdr((struct dhcp_option_ia_pd_hdr_t*)somem())
{
	unpack(buf, buflen);
}



cdhcp_option_ia_pd::~cdhcp_option_ia_pd()
{
	purge_options();
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

	set_length(length());

	// pack the fix part of IA_PD
	cdhcp_option::pack(buf, sizeof(struct dhcp_option_ia_pd_hdr_t));

	// pack the list of variable IA_PD options
	pack_options(buf + sizeof(struct dhcp_option_ia_pd_hdr_t), buflen - sizeof(struct dhcp_option_ia_pd_hdr_t));
}



void
cdhcp_option_ia_pd::unpack(uint8_t *buf, size_t buflen)
{
	if (buflen < sizeof(struct dhcp_option_ia_pd_hdr_t))
		throw eDhcpOptionBadSyntax();

	cdhcp_option::unpack(buf, sizeof(struct dhcp_option_ia_pd_hdr_t));

	// the memory assignment operation above may have invalidated the old memory area,
	// so reset the hdr pointer as well
	hdr = (struct dhcp_option_ia_pd_hdr_t*)somem();

	// unpack the variable size list of IA_PD options appended to fix header
	unpack_options(buf + sizeof(struct dhcp_option_ia_pd_hdr_t), buflen - sizeof(struct dhcp_option_ia_pd_hdr_t));
}



size_t
cdhcp_option_ia_pd::length()
{
	return (memlen() + options_length());
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



cdhcp_option&
cdhcp_option_ia_pd::get_option(uint16_t code)
{
	if (options.find(code) == options.end())
		throw eDhcpOptionNotFound();

	return *(options[code]);
}



size_t
cdhcp_option_ia_pd::options_length()
{
	size_t opt_len = 0;
	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		opt_len += (*(it->second)).length();
	}
	return opt_len;
}



void
cdhcp_option_ia_pd::pack_options(uint8_t *buf, size_t buflen)
{
	if (buflen < options_length())
		throw eDhcpOptionTooShort();

	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		cdhcp_option *opt = (it->second);

		opt->pack(buf, opt->length());

		buf += opt->length();
		buflen -= opt->length();
	}
}



void
cdhcp_option_ia_pd::unpack_options(uint8_t *buf, size_t buflen)
{
	purge_options();

	while (buflen > 0) {

		if (buflen < sizeof(cdhcp_option::dhcp_option_hdr_t))
			throw eDhcpOptionBadSyntax();

		struct cdhcp_option::dhcp_option_hdr_t *hdr = (struct cdhcp_option::dhcp_option_hdr_t*)buf;

		size_t opt_len = sizeof(struct cdhcp_option::dhcp_option_hdr_t) + be16toh(hdr->len);

		// avoid duplicates => potential memory leak
		if (options.find(be16toh(hdr->code)) != options.end()) {
			delete options[be16toh(hdr->code)];
		}

		switch (be16toh(hdr->code)) {
		case cdhcp_option_ia_pd_option_prefix::DHCP_OPTION_IA_PREFIX: {
			options[cdhcp_option_ia_pd_option_prefix::DHCP_OPTION_IA_PREFIX] = new cdhcp_option_ia_pd_option_prefix(buf, opt_len);
		} break;
		default: {
			options[be16toh(hdr->code)] = new cdhcp_option(buf, opt_len);
		} break;
		}

		buf += opt_len;
		buflen -= opt_len;
	}
}



void
cdhcp_option_ia_pd::purge_options()
{
	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		delete (it->second);
	}
	options.clear();
}


