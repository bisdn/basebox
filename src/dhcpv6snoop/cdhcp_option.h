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

#include <rofl/common/cmemory.h>

namespace dhcpv6snoop
{

class eDhcpOptionBase : public std::exception {};
class eDhcpOptionInval : public eDhcpOptionBase {};
class eDhcpOptionSyntax : public eDhcpOptionBase {};
class eDhcpOptionTooShort : public eDhcpOptionBase {};

class cdhcp_option :
		public rofl::cmemory
{
	struct dhcp_option_hdr_t {
		uint16_t 	code;
		uint16_t 	len;
		uint8_t 	data[0];
	};

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
};

}; // end of namespace

#endif /* CDHCP_OPTION_H_ */
