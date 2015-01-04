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

#include <rofl/common/cdpid.h>
#include <roflibs/netlink/cnetdev.hpp>
#include <roflibs/netlink/clogging.hpp>



namespace rofcore {

class eTapDevBase 			: public eNetDevBase {
public:
	eTapDevBase(const std::string& __arg) : eNetDevBase(__arg) {};
};
class eTapDevSysCallFailed 	: public eTapDevBase {
public:
	eTapDevSysCallFailed(const std::string& __arg) : eTapDevBase(__arg) {};
};
class eTapDevOpenFailed		: public eTapDevSysCallFailed {
public:
	eTapDevOpenFailed(const std::string& __arg) : eTapDevSysCallFailed(__arg) {};
};
class eTapDevIoctlFailed	: public eTapDevSysCallFailed {
public:
	eTapDevIoctlFailed(const std::string& __arg) : eTapDevSysCallFailed(__arg) {};
};
class eTapDevNotFound		: public eTapDevBase {
public:
	eTapDevNotFound(const std::string& __arg) : eTapDevBase(__arg) {};
};



class ctapdev : public cnetdev {

	int 							fd; 			// tap device file descriptor
	std::list<rofl::cpacket*> 		pout_queue;		// queue of outgoing packets
	rofl::cdpid						dpid;
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
			const rofl::cdpid& dpid,
			std::string const& devname,
			uint16_t pvid,
			rofl::cmacaddr const& hwaddr,
			pthread_t tid = 0);


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
	rofl::cdpid
	get_dpid() const { return dpid; };

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
		rofl::cdpid dpid;
		std::string devname;
	public:
		ctapdev_find_by_devname(const rofl::cdpid& dpid, const std::string& devname) :
			dpid(dpid), devname(devname) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return ((p.second->dpid == dpid) && (p.second->devname == devname));
		};
	};

	class ctapdev_find_by_hwaddr {
		rofl::cdpid dpid;
		rofl::caddress_ll hwaddr;
	public:
		ctapdev_find_by_hwaddr(const rofl::cdpid& dpid, const rofl::caddress_ll& hwaddr) :
			dpid(dpid), hwaddr(hwaddr) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return ((p.second->dpid == dpid) && (p.second->hwaddr == hwaddr));
		};
	};

	class ctapdev_find_by_pvid {
		rofl::cdpid dpid;
		uint16_t pvid;
	public:
		ctapdev_find_by_pvid(const rofl::cdpid& dpid, uint16_t pvid) :
			dpid(dpid), pvid(pvid) {};
		bool operator() (const std::pair<std::string, ctapdev*>& p) const {
			return ((p.second->dpid == dpid) && (p.second->pvid == pvid));
		};
	};

};

}; // end of namespace rofcore

#endif /* CTAPDEV_H_ */
