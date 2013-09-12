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
public:

	struct dhcp_option_hdr_t {
		/* LITTLE ENDIAN */
		uint16_t 	len;
		uint16_t 	code;
		/* BIG ENDIAN */
#if 0
		uint16_t 	code;
		uint16_t 	len;
#endif
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

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdhcp_option& opt) {
		os << "<dhcp-option: ";
			os << "code: " << opt.get_code() << " ";
			os << "length: " << opt.get_length() << " ";
		os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* CDHCP_OPTION_H_ */
