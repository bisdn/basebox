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
	purge_options();
}



cdhcpmsg::cdhcpmsg(const cdhcpmsg& msg)
{
	*this = msg;
}



void
cdhcpmsg::purge_options()
{
	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		delete (it->second);
	}
	options.clear();
}



cdhcpmsg&
cdhcpmsg::operator= (const cdhcpmsg& msg)
{
	if (this == &msg)
		return *this;

	rofl::cmemory::operator= (msg);

	purge_options();
	for (std::map<uint16_t, cdhcp_option*>::const_iterator
			it = msg.options.begin(); it != msg.options.end(); ++it) {
		options[it->first] = it->second->clone();
	}

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



cdhcp_option&
cdhcpmsg::get_option(uint16_t opt_type)
{
	if (options.find(opt_type) == options.end())
		throw eDhcpMsgNotFound();

	return *(options[opt_type]);
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



size_t
cdhcpmsg::options_length()
{
	size_t opt_len = 0;
	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		opt_len += (*(it->second)).length();
	}
	return opt_len;
}



void
cdhcpmsg::pack_options(uint8_t *buf, size_t buflen)
{
	if (buflen < options_length())
		throw eDhcpMsgTooShort();

	for (std::map<uint16_t, cdhcp_option*>::iterator
			it = options.begin(); it != options.end(); ++it) {
		cdhcp_option *opt = (it->second);

		opt->pack(buf, opt->length());

		buf += opt->length();
		buflen -= opt->length();
	}
}



void
cdhcpmsg::unpack_options(uint8_t *buf, size_t buflen)
{
	purge_options();

	while (buflen > 0) {

		if (buflen < sizeof(cdhcp_option::dhcp_option_hdr_t))
			throw eDhcpMsgBadSyntax();

		struct cdhcp_option::dhcp_option_hdr_t *hdr = (struct cdhcp_option::dhcp_option_hdr_t*)buf;

		size_t opt_len = sizeof(struct cdhcp_option::dhcp_option_hdr_t) + be16toh(hdr->len);

		if (opt_len > buflen)
			throw eDhcpMsgBadSyntax();

		// avoid duplicates => potential memory leak
		if (options.find(be16toh(hdr->code)) != options.end()) {
			delete options[be16toh(hdr->code)];
		}


		switch (be16toh(hdr->code)) {
		case cdhcp_option_clientid::DHCP_OPTION_CLIENTID: {
			options[cdhcp_option_clientid::DHCP_OPTION_CLIENTID] = new cdhcp_option_clientid(buf, opt_len);
		} break;
		case cdhcp_option_ia_prefix::DHCP_OPTION_IA_PREFIX: {
			options[cdhcp_option_ia_prefix::DHCP_OPTION_IA_PREFIX] = new cdhcp_option_ia_prefix(buf, opt_len);
		} break;
		default: {
			options[be16toh(hdr->code)] = new cdhcp_option(buf, opt_len);
		} break;
		}


		buf += opt_len;
		buflen -= opt_len;
	}
}



