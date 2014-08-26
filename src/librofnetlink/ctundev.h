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

#ifdef __cplusplus
}
#endif

#include <list>
#include <iostream>

#include "cnetdev.h"
#include "clogging.h"
#include "cprefix.hpp"

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
	 * @param netdev_owner
	 * @param devname
	 * @param hwaddr
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

	/**
	 *
	 */
	uint32_t
	get_ofp_port_no() const { return ofp_port_no; };

public:

	/**
	 *
	 */
	void
	add_prefix_in4(const cprefix_in4& prefix) {
		if (prefixes_in4.find(prefix) != prefixes_in4.end()) {
			return;
		}
		prefixes_in4.insert(prefix);
	};

	/**
	 *
	 */
	void
	drop_prefix_in4(const cprefix_in4& prefix) {
		if (prefixes_in4.find(prefix) == prefixes_in4.end()) {
			return;
		}
		prefixes_in4.erase(prefix);
	};

	/**
	 *
	 */
	bool
	has_prefix_in4(const cprefix_in4& prefix) const {
		return (not (prefixes_in4.find(prefix) == prefixes_in4.end()));
	};

public:

	/**
	 *
	 */
	void
	add_prefix_in6(const cprefix_in6& prefix) {
		if (prefixes_in6.find(prefix) != prefixes_in6.end()) {
			return;
		}
		prefixes_in6.insert(prefix);
	};

	/**
	 *
	 */
	void
	drop_prefix_in6(const cprefix_in6& prefix) {
		if (prefixes_in6.find(prefix) == prefixes_in6.end()) {
			return;
		}
		prefixes_in6.erase(prefix);
	};

	/**
	 *
	 */
	bool
	has_prefix_in6(const cprefix_in6& prefix) const {
		return (not (prefixes_in6.find(prefix) == prefixes_in6.end()));
	};

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
				<< "devname: " << tundev.devname << " "
				<< "#in4-prefixes: " << (int)tundev.prefixes_in4.size() << " "
				<< "#in6-prefixes: " << (int)tundev.prefixes_in6.size() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::set<cprefix_in4>::const_iterator
				it = tundev.prefixes_in4.begin(); it != tundev.prefixes_in4.end(); ++it) {
			os << *(it);
		}
		for (std::set<cprefix_in6>::const_iterator
				it = tundev.prefixes_in6.begin(); it != tundev.prefixes_in6.end(); ++it) {
			os << *(it);
		}
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

	class ctundev_find_by_ofp_port_no {
		uint32_t ofp_port_no;
	public:
		ctundev_find_by_ofp_port_no(uint32_t ofp_port_no) :
			ofp_port_no(ofp_port_no) {};
		bool operator() (const std::pair<uint32_t, ctundev*>& p) const {
			return (p.second->ofp_port_no == ofp_port_no);
		};
	};

private:

	int 							fd; 			// tap device file descriptor
	std::list<rofl::cpacket*> 		pout_queue;		// queue of outgoing packets
	std::string						devname;
	rofl::ctimerid					port_open_timer_id;
	uint32_t						ofp_port_no;

	enum ctundev_timer_t {
		CTUNDEV_TIMER_OPEN_PORT = 1,
	};

	std::set<cprefix_in4>			prefixes_in4;
	std::set<cprefix_in6>			prefixes_in6;
};

}; // end of namespace rofcore

#endif /* CTUNDEV_H_ */
