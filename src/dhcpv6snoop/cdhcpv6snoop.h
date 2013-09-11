/*
 * dhcpv6snoop.h
 *
 *  Created on: 11.09.2013
 *      Author: andreas
 */

#ifndef DHCPV6SNOOP_H_
#define DHCPV6SNOOP_H_

#include <rofl/common/ciosrv.h>
#include <rofl/common/csocket.h>

namespace dhcpv6snoop
{

class cdhcpv6snoop :
		public rofl::ciosrv,
		public rofl::csocket_owner
{
public:

	virtual
	~cdhcpv6snoop();

	virtual void
	handle_accepted(rofl::csocket *socket, int newsd, rofl::caddress const& ra);

	virtual void
	handle_connected(rofl::csocket *socket, int sd);

	virtual void
	handle_connect_refused(rofl::csocket *socket, int sd);

	virtual void
	handle_read(rofl::csocket *socket, int sd);

	virtual void
	handle_closed(rofl::csocket *socket, int sd);
};

}; // end of namespace

#endif /* DHCPV6SNOOP_H_ */
