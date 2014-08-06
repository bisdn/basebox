/*
 * cdhcp_option_ia_pd_option_prefix.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef CDHCP_OPTION_IA_PREFIX_H_
#define CDHCP_OPTION_IA_PREFIX_H_

#include <rofl/common/caddress.h>

#include "cdhcp_option.h"

namespace dhcpv6snoop
{

class cdhcp_option_ia_prefix :
		public cdhcp_option
{
public:

	enum cdhcp_option_ia_prefix_type {
		DHCP_OPTION_IA_PREFIX = 26,
	};

	struct dhcp_option_ia_prefix_hdr_t {
		struct dhcp_option_hdr_t header;
		uint32_t preferred_lifetime;
		uint32_t valid_lifetime;
		uint8_t prefixlen;
		uint8_t prefix[16];
		uint8_t data[0];
	};

private:

	struct dhcp_option_ia_prefix_hdr_t *hdr;

public:

	cdhcp_option_ia_prefix();

	cdhcp_option_ia_prefix(uint8_t *buf, size_t buflen);

	virtual ~cdhcp_option_ia_prefix();

	cdhcp_option_ia_prefix(const cdhcp_option_ia_prefix& opt);

	cdhcp_option_ia_prefix& operator= (const cdhcp_option_ia_prefix& opt);

public:

	virtual void validate();

	virtual uint8_t* resize(size_t len);

	virtual void pack(uint8_t *buf, size_t buflen);

	virtual void unpack(uint8_t *buf, size_t buflen);

	virtual size_t length();

public:

	uint32_t
	get_preferred_lifetime() const;

	void
	set_preferred_lifetime(uint32_t preferred_lifetime);

	uint32_t
	get_valid_lifetime() const;

	void
	set_valid_lifetime(uint32_t valid_lifetime);

	uint8_t
	get_prefixlen() const;

	void
	set_prefixlen(uint8_t prefixlen);

	rofl::caddress_in6
	get_prefix() const;

	void
	set_prefix(rofl::caddress_in6 const& prefix);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdhcp_option_ia_prefix& opt) {
		os << dynamic_cast<const cdhcp_option&>( opt );
		os << "<dhcp-option-ia-pd-option-prefix: ";
			os << "preferred-lifetime: " << (unsigned int)opt.get_preferred_lifetime() << " ";
			os << "valid-lifetime: " << (unsigned int)opt.get_valid_lifetime() << " ";
			os << "prefixlen: " << (unsigned int)opt.get_prefixlen() << " ";
			os << "prefix: " << opt.get_prefix() << " ";
		os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* CDHCP_OPTION_IA_PD_OPTION_PREFIX_H_ */
