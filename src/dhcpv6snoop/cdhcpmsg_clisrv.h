/*
 * cdhcpmsg_clisrv.h
 *
 *  Created on: 13.09.2013
 *      Author: andreas
 */

#ifndef CDHCPMSG_CLISRV_H_
#define CDHCPMSG_CLISRV_H_

#include "cdhcpmsg.h"

namespace dhcpv6snoop
{

class cdhcpmsg_clisrv :
		public cdhcpmsg
{
	struct dhcpmsg_clisrv_hdr_t {
		struct dhcpmsg_hdr_t header;
		uint8_t transaction_id[3];
		uint8_t data[0];
	};

	struct dhcpmsg_clisrv_hdr_t *hdr;

public:

	cdhcpmsg_clisrv();

	virtual ~cdhcpmsg_clisrv();

	cdhcpmsg_clisrv(const cdhcpmsg_clisrv& msg);

	cdhcpmsg_clisrv& operator= (const cdhcpmsg_clisrv& msg);

	cdhcpmsg_clisrv(uint8_t *buf, size_t buflen);

public:

	virtual size_t
	length();

	virtual void
	pack(uint8_t *buf, size_t buflen);

	virtual void
	unpack(uint8_t *buf, size_t buflen);

	virtual uint8_t*
	resize(size_t len);

public:

	uint32_t
	get_transaction_id() const;

	void
	set_transaction_id(uint32_t transaction_id);

private:

	virtual void
	validate();

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cdhcpmsg_clisrv& msg) {
		os << "<cdhcpmsg-clisrv: ";
			os << "transaction-id: " << msg.get_transaction_id() << " ";
		os << ">";
		os << dynamic_cast<const cdhcpmsg&>( msg );
		return os;
	};
};

}; // end of namespace

#endif /* CDHCPMSG_CLISRV_H_ */
