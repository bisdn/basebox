/*
 * cdhcp_option_ia_pd.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef CDHCP_OPTION_IA_PD_H_
#define CDHCP_OPTION_IA_PD_H_

/*
 * RFC 3633: Identity Association for Prefix Delegation
 */

#include "cdhcp_option.h"

namespace dhcpv6snoop
{

class cdhcp_option_ia_pd :
		public cdhcp_option
{
public:

	enum cdhcp_option_ia_pd_type {
		DHCP_OPTION_IA_PD = 25,
	};

	struct dhcp_option_ia_pd_hdr_t {
		struct dhcp_option_hdr_t header;
		uint32_t 	iaid;
		uint32_t 	t1;
		uint32_t 	t2;
		uint8_t 	data[0];
	};

private:

	struct dhcp_option_ia_pd_hdr_t *hdr;

public:

	cdhcp_option_ia_pd();

	cdhcp_option_ia_pd(uint8_t *buf, size_t buflen);

	virtual ~cdhcp_option_ia_pd();

	cdhcp_option_ia_pd(const cdhcp_option_ia_pd& opt);

	cdhcp_option_ia_pd& operator= (const cdhcp_option_ia_pd& opt);

public:

	virtual void validate();

	virtual uint8_t* resize(size_t len);

	virtual void pack(uint8_t *buf, size_t buflen);

	virtual void unpack(uint8_t *buf, size_t buflen);

	virtual size_t length();

public:

	uint32_t get_iaid() const;

	void set_iaid(uint32_t iaid);

	uint32_t get_t1() const;

	void set_t1(uint32_t t1);

	uint32_t get_t2() const;

	void set_t2(uint32_t t2);
};


}; // end of namespace

#endif /* CDHCP_OPTION_IA_PD_H_ */
