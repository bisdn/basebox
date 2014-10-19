/*
 * ctapdev.h
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#ifndef CTAPDEV_H_
#define CTAPDEV_H_ 1


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

#ifdef __cplusplus
}
#endif

#include <list>

#include <roflibs/netlink/cnetdev.hpp>



namespace rofcore {

class eTapDevBase 			: public eNetDevBase {};
class eTapDevSysCallFailed 	: public eTapDevBase {};
class eTapDevOpenFailed		: public eTapDevSysCallFailed {};
class eTapDevIoctlFailed	: public eTapDevSysCallFailed {};
class eTapDevNotFound		: public eTapDevBase {};

class ctapdev : public cnetdev
{
	int 							fd; 			// tap device file descriptor
	std::list<rofl::cpacket*> 		pout_queue;		// queue of outgoing packets
	std::string						devname;
	uint16_t						pvid;
	rofl::cmacaddr					hwaddr;
	rofl::ctimerid					port_open_timer_id;

	enum ctapdev_timer_t {
		CTAPDEV_TIMER_OPEN_PORT = 1,
	};

public:


	/**
	 *
	 * @param netdev_owner
	 * @param devname
	 * @param hwaddr
	 */
	ctapdev(
			cnetdev_owner *netdev_owner,
			std::string const& devname,
			uint16_t pvid,
			rofl::cmacaddr const& hwaddr);


	/**
	 *
	 */
	virtual ~ctapdev();


	/**
	 * @brief	Enqueues a single rofl::cpacket instance on cnetdev.
	 *
	 * rofl::cpacket instance must have been allocated on heap and must
	 * be removed
	 */
	virtual void enqueue(rofl::cpacket *pkt);

	virtual void enqueue(std::vector<rofl::cpacket*> pkts);

	/**
	 *
	 */
	uint16_t
	get_pvid() const { return pvid; };

protected:

	/**
	 * @brief	open tapX device
	 */
	void
	tap_open(std::string const& devname, rofl::cmacaddr const& hwaddr);


	/**
	 * @brief	close tapX device
	 *
	 */
	void
	tap_close();


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

	class ctapdev_find_by_devname {
		std::string devname;
	public:
		ctapdev_find_by_devname(const std::string& devname) :
			devname(devname) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return (p.second->devname == devname);
		};
	};

	class ctapdev_find_by_hwaddr {
		rofl::caddress_ll hwaddr;
	public:
		ctapdev_find_by_hwaddr(const rofl::caddress_ll& hwaddr) :
			hwaddr(hwaddr) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return (p.second->hwaddr == hwaddr);
		};
	};

	class ctapdev_find_by_pvid {
		uint16_t pvid;
	public:
		ctapdev_find_by_pvid(uint16_t pvid) :
			pvid(pvid) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return (p.second->pvid == pvid);
		};
	};

};

}; // end of namespace rofcore

#endif /* CTAPDEV_H_ */
