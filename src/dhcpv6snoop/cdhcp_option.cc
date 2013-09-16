/*
 * cdhcp_option.cc
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#include "cdhcp_option.h"

using namespace dhcpv6snoop;

#include "cdhcp_option_clientid.h"
#include "cdhcp_option_ia_pd.h"
#include "cdhcp_option_ia_prefix.h"


cdhcp_option::cdhcp_option(size_t len) :
		rofl::cmemory(len),
		hdr((struct dhcp_option_hdr_t*)somem())
{

}



cdhcp_option::~cdhcp_option()
{
	purge_options();
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



cdhcp_option*
cdhcp_option::clone()
{
	return new cdhcp_option(*this);
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





cdhcp_option&
cdhcp_option::get_option(uint16_t code)
{
	if (options.find(code) == options.end())
		throw eDhcpOptionNotFound();

	return *(options[code]);
}



size_t
cdhcp_option::options_length()
{
	size_t opt_len = 0;
	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		opt_len += (*(it->second)).length();
	}
	return opt_len;
}



void
cdhcp_option::pack_options(uint8_t *buf, size_t buflen)
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
cdhcp_option::unpack_options(uint8_t *buf, size_t buflen)
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
		case DHCP_OPTION_CLIENTID: {
			options[DHCP_OPTION_CLIENTID] = new cdhcp_option_clientid(buf, opt_len);
		} break;
		case DHCP_OPTION_SERVERID: {
			options[DHCP_OPTION_SERVERID] = new cdhcp_option_serverid(buf, opt_len);
		} break;
		case DHCP_OPTION_IA_PD: {
			options[DHCP_OPTION_IA_PD] = new cdhcp_option_ia_pd(buf, opt_len);
		} break;
		case DHCP_OPTION_IA_PREFIX: {
			options[DHCP_OPTION_IA_PREFIX] = new cdhcp_option_ia_prefix(buf, opt_len);
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
cdhcp_option::purge_options()
{
	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		delete (it->second);
	}
	options.clear();
}


