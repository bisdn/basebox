/*
 * ctundev.h
 *
 *  Created on: 25.08.2014
 *      Author: andreas
 */

#ifndef CTUNDEV_H_
#define CTUNDEV_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>

#ifdef __cplusplus
}
#endif

#include <list>
#include <iostream>

#include <roflibs/netlink/cnetdev.hpp>
#include <roflibs/netlink/clogging.hpp>
#include <roflibs/netlink/cprefix.hpp>
#include <roflibs/netlink/cnetlink.hpp>

namespace rofcore {

class eTunDevBase 			: public eNetDevBase {};
class eTunDevSysCallFailed 	: public eTunDevBase {};
class eTunDevOpenFailed		: public eTunDevSysCallFailed {};
class eTunDevIoctlFailed	: public eTunDevSysCallFailed {};
class eTunDevNotFound		: public eTunDevBase {};

class ctundev : public cnetdev {
public:

	/**
	 *
	 */
	ctundev(
			cnetdev_owner *netdev_owner,
			std::string const& devname);


	/**
	 *
	 */
	virtual
	~ctundev();


	/**
	 * @brief	Enqueues a single rofl::cpacket instance on cnetdev.
	 *
	 * rofl::cpacket instance must have been allocated on heap and must
	 * be removed
	 */
	virtual void
	enqueue(rofl::cpacket *pkt);

	virtual void
	enqueue(std::vector<rofl::cpacket*> pkts);

protected:

	/**
	 * @brief	open tapX device
	 */
	void
	tun_open(std::string const& devname);


	/**
	 * @brief	close tapX device
	 *
	 */
	void
	tun_close();


	/**
	 * @brief	handle read events on file descriptor
	 */
	virtual void
	handle_revent(int fd);


	/**
	 * @brief	handle write events on file descriptor
	 */
	virtual void
	handle_wevent(int fd);

private:

	/**
	 * @brief	reschedule opening of port in case of failure
	 */
	virtual void
	handle_timeout(int opaque, void* data = (void*)0);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const ctundev& tundev) {
		os << rofcore::indent(0) << "<ctundev "
				<< "devname: " << tundev.devname << " >" << std::endl;
		return os;
	};

	class ctundev_find_by_devname {
		std::string devname;
	public:
		ctundev_find_by_devname(const std::string& devname) :
			devname(devname) {};
		bool operator() (const std::pair<uint32_t, ctundev*>& p) const {
			return (p.second->devname == devname);
		};
	};

private:

	int 							fd; 			// tap device file descriptor
	std::list<rofl::cpacket*> 		pout_queue;		// queue of outgoing packets
	std::string						devname;
	rofl::ctimerid					port_open_timer_id;

	enum ctundev_timer_t {
		CTUNDEV_TIMER_OPEN_PORT = 1,
	};
};

}; // end of namespace rofcore

#endif /* CTUNDEV_H_ */
