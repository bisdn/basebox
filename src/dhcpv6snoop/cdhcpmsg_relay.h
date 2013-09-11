/*
 * cdhcpmsg_relay.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef CDHCPMSG_RELAY_H_
#define CDHCPMSG_RELAY_H_

#include <map>

#include <rofl/common/caddress.h>

#include "cdhcpmsg.h"
#include "cdhcp_option.h"
#include "cdhcp_option_ia_pd.h"

namespace dhcpv6snoop
{

class cdhcpmsg_relay :
		public cdhcpmsg
{

	struct dhcpmsg_relay_hdr_t {
		struct dhcpmsg_hdr_t header;
		uint8_t hop_count;
		uint8_t link_address[16];
		uint8_t peer_address[16];
		uint8_t data[0];
	};

	struct dhcpmsg_relay_hdr_t *hdr;

	std::map<uint16_t, cdhcp_option*> options;

public:

	cdhcpmsg_relay();

	virtual ~cdhcpmsg_relay();

	cdhcpmsg_relay(const cdhcpmsg_relay& msg);

	cdhcpmsg_relay& operator= (const cdhcpmsg_relay& msg);

	cdhcpmsg_relay(uint8_t *buf, size_t buflen);

public:

	virtual size_t
	length();

	virtual void
	pack(uint8_t *buf, size_t buflen);

	virtual void
	unpack(uint8_t *buf, size_t buflen);

public:

	uint8_t
	get_hop_count() const;

	void
	set_hop_count(uint8_t hop_count);

	rofl::caddress
	get_link_address() const;

	void
	set_link_address(rofl::caddress const& link_address);

	rofl::caddress
	get_peer_address() const;

	void
	set_peer_address(rofl::caddress const& peer_address);

	cdhcp_option&
	get_option(uint16_t opt_type);

private:

	virtual void
	validate();

	size_t
	options_length();

	void
	pack_options(uint8_t *buf, size_t buflen);

	void
	unpack_options(uint8_t *buf, size_t buflen);

	void
	purge_options();
};

}; // end of namespace

#endif /* CDHCPMSG_RELAY_H_ */
