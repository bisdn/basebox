/*
 * cdhcp_option_client_id.h
 *
 *  Created on: 13.09.2013
 *      Author: andreas
 */

#ifndef CDHCP_OPTION_CLIENT_ID_H_
#define CDHCP_OPTION_CLIENT_ID_H_

#include <iomanip>

#include <rofl/common/cmemory.h>

#include "cdhcp_option.h"

namespace dhcpv6snoop
{

class cdhcp_option_clientid :
		public cdhcp_option
{
public:

	enum cdhcp_option_client_id_type {
		DHCP_OPTION_CLIENT_ID = 1,
		DHCP_OPTION_SERVER_ID = 2,
	};

	struct dhcp_option_client_id_hdr_t {
		struct dhcp_option_hdr_t header;
		uint8_t 	data[0];
	};

private:

	struct dhcp_option_client_id_hdr_t *hdr;
	rofl::cmemory duid;

public:

	cdhcp_option_clientid();

	cdhcp_option_clientid(uint8_t *buf, size_t buflen);

	virtual ~cdhcp_option_clientid();

	cdhcp_option_clientid(const cdhcp_option_clientid& opt);

	cdhcp_option_clientid& operator= (const cdhcp_option_clientid& opt);

public:

	virtual void
	validate();

	virtual uint8_t*
	resize(size_t len);

	virtual void
	pack(uint8_t *buf, size_t buflen);

	virtual void
	unpack(uint8_t *buf, size_t buflen);

	virtual size_t
	length();

public:

	rofl::cmemory
	get_duid() const;

	void
	set_duid(rofl::cmemory const& duid);

	std::string
	get_s_duid() const;


public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdhcp_option_clientid& opt) {
		switch (opt.get_code()) {
		case cdhcp_option::DHCP_OPTION_CLIENTID: {
			os << "<dhcp-option-client-id: ";
				os << "duid: " << opt.get_s_duid() << " ";
			os << ">";

		} break;
		case cdhcp_option::DHCP_OPTION_SERVERID: {
			os << "<dhcp-option-server-id: ";
				os << "duid: " << opt.get_s_duid() << " ";
			os << ">";

		} break;
		}
		os << dynamic_cast<const cdhcp_option&>( opt );
		return os;
	};
};

typedef cdhcp_option_clientid cdhcp_option_serverid;

}


#endif /* CDHCP_OPTION_CLIENT_ID_H_ */
