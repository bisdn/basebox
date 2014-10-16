/*
 * cportconf.hpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#ifndef CPORTCONF_HPP_
#define CPORTCONF_HPP_

#include <inttypes.h>
#include <exception>
#include <ostream>
#include <set>
#include <map>
#include <rofl/common/cdpid.h>
#include <rofl/common/caddress.h>
#include <roflibs/netlink/clogging.hpp>

namespace roflibs {
namespace ethernet {


class ePortDBBase : public std::runtime_error {
public:
	ePortDBBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class ePortDBNotFound : public ePortDBBase {
public:
	ePortDBNotFound(const std::string& __arg) : ePortDBBase(__arg) {};
};

class cportentry {
public:

	/**
	 *
	 */
	cportentry(uint32_t portno = 0,
			const std::string& devname = std::string(""),
			const rofl::caddress_ll& hwaddr = rofl::caddress_ll("00:00:00:00:00:00"),
			uint16_t port_vid = 1) :
		portno(portno),
		devname(devname),
		hwaddr(hwaddr),
		port_vid(port_vid) {};

	/**
	 *
	 */
	~cportentry() {};

	/**
	 *
	 */
	cportentry(const cportentry& port) {
		*this = port;
	};

	/**
	 *
	 */
	cportentry&
	operator= (const cportentry& port) {
		if (this == &port)
			return *this;
		portno = port.portno;
		devname = port.devname;
		hwaddr = port.hwaddr;
		port_vid = port.port_vid;
		tagged_vids.clear();
		for (std::set<uint16_t>::const_iterator
				it = port.tagged_vids.begin(); it != port.tagged_vids.end(); ++it) {
			tagged_vids.insert(*it);
		}
		return *this;
	};

public:

	/**
	 *
	 */
	void
	set_portno(uint32_t portno) { this->portno = portno; };

	/**
	 *
	 */
	uint32_t
	get_portno() const { return portno; };

	/**
	 *
	 */
	void
	set_devname(const std::string& devname) { this->devname = devname; };

	/**
	 *
	 */
	const std::string&
	get_devname() const { return devname; };

	/**
	 *
	 */
	void
	set_hwaddr(const rofl::caddress_ll& hwaddr) { this->hwaddr = hwaddr; };

	/**
	 *
	 */
	const rofl::caddress_ll&
	get_hwaddr() const { return hwaddr; };

	/**
	 *
	 */
	void
	set_port_vid(uint16_t port_vid) { this->port_vid = port_vid; };

	/**
	 *
	 */
	uint16_t
	get_port_vid() const { return port_vid; };

	/**
	 *
	 */
	const std::set<uint16_t>&
	get_tagged_vids() const { return tagged_vids; };

	/**
	 *
	 */
	void
	add_vid(uint16_t vid) {
		tagged_vids.insert(vid);
	};

	/**
	 *
	 */
	void
	drop_vid(uint16_t vid) {
		tagged_vids.erase(vid);
	};

	/**
	 *
	 */
	void
	clear_vids() {
		tagged_vids.clear();
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cportentry& port) {
		// TODO
		return os;
	};

private:

	uint32_t 			portno;
	std::string	 		devname;
	rofl::caddress_ll	hwaddr;
	uint16_t			port_vid;
	std::set<uint16_t>	tagged_vids;
};

class cportdb {
public:

	/**
	 *
	 */
	static cportdb&
	get_portdb(const std::string& name);

public:

	/**
	 * @brief	return default pvid assigned to the port for untagged traffic
	 */
	uint16_t
	get_pvid(const rofl::cdpid& dpid, uint32_t portno) {
		return get_port_entry(dpid, portno).get_port_vid();
	};

	/**
	 * @brief	return std::set of tagged memberships for this port
	 */
	const std::set<uint16_t>&
	get_vlan_memberships(const rofl::cdpid& dpid, uint32_t portno) {
		return get_port_entry(dpid, portno).get_tagged_vids();
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cportdb& portconf) {
		// TODO
		return os;
	};

public:

	/**
	 *
	 */
	cportentry&
	add_port_entry(const rofl::cdpid& dpid, uint32_t portno,
					const std::string& devname,
					const rofl::caddress_ll& hwaddr,
					uint16_t port_vid) {
		if (portentries[dpid].find(portno) != portentries[dpid].end()) {
			portentries[dpid].erase(portno);
		}
		return (portentries[dpid][portno] = cportentry(portno, devname, hwaddr, port_vid));
	};

	/**
	 *
	 */
	cportentry&
	set_port_entry(const rofl::cdpid& dpid, uint32_t portno,
					const std::string& devname,
					const rofl::caddress_ll& hwaddr,
					uint16_t port_vid) {
		if (portentries[dpid].find(portno) == portentries[dpid].end()) {
			(void)portentries[dpid][portno];
		}
		return (portentries[dpid][portno] = cportentry(portno, devname, hwaddr, port_vid));
	};

	/**
	 *
	 */
	cportentry&
	set_port_entry(const rofl::cdpid& dpid, uint32_t portno) {
		if (portentries[dpid].find(portno) == portentries[dpid].end()) {
			(void)portentries[dpid][portno];
		}
		return (portentries[dpid][portno]);
	};

	/**
	 *
	 */
	const cportentry&
	get_port_entry(const rofl::cdpid& dpid, uint32_t portno) const {
		if (portentries.at(dpid).find(portno) == portentries.at(dpid).end()) {
			throw ePortDBNotFound("cportdb::get_port_entry() portno not found");
		}
		return portentries.at(dpid).at(portno);
	};

	/**
	 *
	 */
	void
	drop_port_entry(const rofl::cdpid& dpid, uint32_t portno) {
		if (portentries[dpid].find(portno) == portentries[dpid].end()) {
			return;
		}
		portentries[dpid].erase(portno);
	};

	/**
	 *
	 */
	bool
	has_port_entry(const rofl::cdpid& dpid, uint32_t portno) const {
		return (not (portentries.at(dpid).find(portno) == portentries.at(dpid).end()));
	};

private:

	std::map<rofl::cdpid, std::map<uint32_t, cportentry> >	portentries;
	static std::map<std::string, cportdb*> 					portdbs;
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CPORTCONF_HPP_ */
