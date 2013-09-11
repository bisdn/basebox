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

	struct dhcpmsg_hdr_t {
		uint8_t msg_type;
		uint8_t data[0];
	};

	struct dhcpmsg_hdr_t *hdr;

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

private:

	virtual void
	validate();

public:

	uint8_t
	get_msg_type() const;

	void
	set_msg_type(uint8_t msg_type);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdhcpmsg& msg) {
		os << "<cdhcpmsg: ";
			os << "msg_type: " << msg.get_msg_type() << " ";
			os << "length: " << msg.memlen() << " ";
			os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* CDHCPMSG_H_ */
