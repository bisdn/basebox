/*
 * cdhcpmsg.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef CDHCPMSG_H_
#define CDHCPMSG_H_

#include <exception>
#include <iostream>

#include <rofl/common/cmemory.h>

#include "cdhcp_option.h"
#include "cdhcp_option_clientid.h"
#include "cdhcp_option_ia_pd.h"
#include "cdhcp_option_ia_prefix.h"

namespace dhcpv6snoop
{

class eDhcpMsgBase 		: public std::exception {};
class eDhcpMsgInval 	: public eDhcpMsgBase {};
class eDhcpMsgTooShort	: public eDhcpMsgBase {};
class eDhcpMsgBadSyntax	: public eDhcpMsgBase {};
class eDhcpMsgNotFound	: public eDhcpMsgBase {};

class cdhcpmsg :
		public rofl::cmemory
{
public:

	enum dhcp_msg_type_t {
		SOLICIT 			= 1,
		ADVERTISE			= 2,
		REQUEST				= 3,
		CONFIRM 			= 4,
		RENEW				= 5,
		REBIND				= 6,
		REPLY				= 7,
		RELEASE				= 8,
		DECLINE				= 9,
		RECONFIGURE			= 10,
		INFORMATION_REQUEST	= 11,
		RELAY_FORW			= 12,
		RELAY_REPLY			= 13,
	};

private:

	struct dhcpmsg_hdr_t {
		uint8_t msg_type;
		uint8_t data[0];
	};

	struct dhcpmsg_hdr_t *hdr;

protected:

	std::map<uint16_t, cdhcp_option*> options;

public:

	cdhcpmsg(size_t len);

	virtual ~cdhcpmsg();

	cdhcpmsg(const cdhcpmsg& msg);

	cdhcpmsg& operator= (const cdhcpmsg& msg);

	cdhcpmsg(uint8_t *buf, size_t buflen);

public:

	virtual size_t
	length();

	virtual void
	pack(uint8_t *buf, size_t buflen);

	virtual void
	unpack(uint8_t *buf, size_t buflen);

	virtual uint8_t*
	resize(size_t len);

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

	uint8_t
	get_msg_type() const;

	void
	set_msg_type(uint8_t msg_type);

	cdhcp_option&
	get_option(uint16_t opt_type);

private:

	virtual void
	validate();

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdhcpmsg& msg) {
		os << "<cdhcpmsg: ";
			os << "msg_type: " ;
			switch (msg.get_msg_type()) {
			case SOLICIT: 				os << "=SOLICIT= "; 			break;
			case ADVERTISE: 			os << "=ADVERTISE= "; 			break;
			case REQUEST: 				os << "=REQUEST= "; 			break;
			case CONFIRM: 				os << "=CONFIRM= "; 			break;
			case RENEW: 				os << "=RENEW= "; 				break;
			case REBIND: 				os << "=REBIND= "; 				break;
			case REPLY: 				os << "=REPLY= "; 				break;
			case RELEASE: 				os << "=RELEASE= ";				break;
			case DECLINE: 				os << "=DECLINE= "; 			break;
			case RECONFIGURE:			os << "=RECONFIGURE= ";			break;
			case INFORMATION_REQUEST:	os << "=INFORMATION-REQUEST= "; break;
			case RELAY_FORW: 			os << "=RELAY-FORW= ";			break;
			case RELAY_REPLY:			os << "=RELAY_REPLY= "; 		break;
			default:					os << "=UNKNOWN= ";				break;
			}
			os << "(" << msg.get_msg_type() << ") ";
			os << "length: " << msg.memlen() << " ";
			for (std::map<uint16_t, cdhcp_option*>::const_iterator
					it = msg.options.begin(); it != msg.options.end(); ++it) {
				os << *(it->second) << " ";
			}
			os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* CDHCPMSG_H_ */
