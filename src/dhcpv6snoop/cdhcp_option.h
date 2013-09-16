/*
 * cdhcp_option.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef CDHCP_OPTION_H_
#define CDHCP_OPTION_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <map>
#include <exception>
#include <iostream>

#include <rofl/common/cmemory.h>

namespace dhcpv6snoop
{

class eDhcpOptionBase 		: public std::exception {};
class eDhcpOptionInval 		: public eDhcpOptionBase {};
class eDhcpOptionBadSyntax 	: public eDhcpOptionBase {};
class eDhcpOptionTooShort 	: public eDhcpOptionBase {};
class eDhcpOptionNotFound 	: public eDhcpOptionBase {};

class cdhcp_option :
		public rofl::cmemory
{
protected:

	std::map<uint16_t, cdhcp_option*> options;

public:

	enum dhcp_option_type_t {
		DHCP_OPTION_CLIENTID		= 1,
		DHCP_OPTION_SERVERID		= 2,
		DHCP_OPTION_IA_NA			= 3,
		DHCP_OPTION_IA_TA			= 4,
		DHCP_OPTION_IAADDR			= 5,
		DHCP_OPTION_ORO				= 6,
		DHCP_OPTION_PREFERENCE		= 7,
		DHCP_OPTION_ELAPSED_TIME	= 8,
		DHCP_OPTION_RELAY_MSG		= 9,
		DHCP_OPTION_AUTH			= 11,
		DHCP_OPTION_UNICAST			= 12,
		DHCP_OPTION_STATUS_CODE		= 13,
		DHCP_OPTION_RAPID_COMMIT	= 14,
		DHCP_OPTION_USER_CLASS		= 15,
		DHCP_OPTION_VENDOR_CLASS	= 16,
		DHCP_OPTION_VENDOR_OPTS		= 17,
		DHCP_OPTION_INTERFACE_ID	= 18,
		DHCP_OPTION_RECONF_MSG		= 19,
		DHCP_OPTION_RECONF_ACCEPT	= 20,
		DHCP_OPTION_IA_PD			= 25,
		DHCP_OPTION_IA_PREFIX		= 26,
	};

	struct dhcp_option_hdr_t {
		uint16_t 	code;
		uint16_t 	len;
		uint8_t 	data[0];
	};

private:

	struct dhcp_option_hdr_t* hdr;

public:

	cdhcp_option(size_t len);

	virtual ~cdhcp_option();

	cdhcp_option(uint8_t *buf, size_t buflen);

	cdhcp_option(const cdhcp_option& opt);

	cdhcp_option& operator= (const cdhcp_option& opt);

	virtual cdhcp_option* clone();

public:

	virtual void validate();

	virtual uint8_t* resize(size_t len);

	virtual void pack(uint8_t *buf, size_t buflen);

	virtual void unpack(uint8_t *buf, size_t buflen);

	virtual size_t length();

public:

	uint16_t get_code() const;

	void set_code(uint16_t code);

	uint16_t get_length() const;

	void set_length(uint16_t len);

	cdhcp_option&
	get_option(uint16_t code);

protected:

	size_t
	options_length();

	void
	pack_options(uint8_t *buf, size_t buflen);

	void
	unpack_options(uint8_t *buf, size_t buflen);

	void
	purge_options();

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdhcp_option& opt) {
		os << "<dhcp-option: ";
			os << "code: " << opt.get_code() << " ";
			os << "length: " << opt.get_length() << " ";
			os << "options: ";
			for (std::map<uint16_t, cdhcp_option*>::const_iterator
						it = opt.options.begin(); it != opt.options.end(); ++it) {
				os << *(it->second) << " ";
			}
		os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* CDHCP_OPTION_H_ */
