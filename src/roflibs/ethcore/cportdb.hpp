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
#include <string>
#include <set>
#include <map>
#include <rofl/common/cdpid.h>
#include <rofl/common/caddress.h>
#include <roflibs/netlink/clogging.hpp>

namespace roflibs {
namespace eth {


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
	cportentry(
			uint32_t portno = 0,
			uint16_t port_vid = 1) :
		portno(portno),
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
		os << rofcore::indent(0) << "<cportentry ";
		os << "portno:" << (unsigned int)port.get_portno() << " ";
		os << "pvid:" << (int)port.get_port_vid() << " ";
		os << "tagged:";
		for (std::set<uint16_t>::const_iterator
				it = port.tagged_vids.begin(); it != port.tagged_vids.end(); ++it) {
			os << (int)(*it) << ",";
		}
		os << " >" << std::endl;
		return os;
	};

private:

	uint32_t 			portno;
	uint16_t			port_vid;
	std::set<uint16_t>	tagged_vids;
};


class cethentry {
public:

	/**
	 *
	 */
	cethentry(
			const std::string& devname = std::string(""),
			const rofl::caddress_ll& hwaddr = rofl::caddress_ll("00:00:00:00:00:00"),
			uint16_t port_vid = 1) :
		devname(devname),
		hwaddr(hwaddr),
		port_vid(port_vid) {};

	/**
	 *
	 */
	~cethentry() {};

	/**
	 *
	 */
	cethentry(const cethentry& port) {
		*this = port;
	};

	/**
	 *
	 */
	cethentry&
	operator= (const cethentry& port) {
		if (this == &port)
			return *this;
		devname 	= port.devname;
		hwaddr 		= port.hwaddr;
		port_vid 	= port.port_vid;
		return *this;
	};

public:

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

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cethentry& port) {
		os << rofcore::indent(0) << "<cethentry ";
		os << "devname:" << port.get_devname() << " ";
		os << "hwaddr:" << port.get_hwaddr().str() << " ";
		os << "pvid:" << (int)port.get_port_vid() << " ";
		os << ">" << std::endl;
		return os;
	};

private:

	std::string	 		devname;
	rofl::caddress_ll	hwaddr;
	uint16_t			port_vid;
};

class cportdb {
public:

	/**
	 *
	 */
	static cportdb&
	get_portdb(const std::string& name);

	/**
	 *
	 */
	virtual
	~cportdb() {};

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

	/**
	 *
	 */
	const std::set<uint32_t>&
	get_port_entries(const rofl::cdpid& dpid) const {
		return portnames[dpid];
	};

	/**
	 *
	 */
	cportentry&
	add_port_entry(const rofl::cdpid& dpid, uint32_t portno, uint16_t port_vid) {
		if (portentries[dpid].find(portno) != portentries[dpid].end()) {
			portentries[dpid].erase(portno);
		}
		portnames[dpid].insert(portno);
		return (portentries[dpid][portno] = cportentry(portno, port_vid));
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
			portnames[dpid].insert(portno);
		}
		return (portentries[dpid][portno] = cportentry(portno, port_vid));
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
		if ((portentries.find(dpid) == portentries.end()) ||
				(portentries.at(dpid).find(portno) == portentries.at(dpid).end())) {
			throw ePortDBNotFound("cportdb::get_port_entry() portno not found");
		}
		return portentries.at(dpid).at(portno);
	};

	/**
	 *
	 */
	void
	drop_port_entry(const rofl::cdpid& dpid, uint32_t portno) {
		if ((portentries.find(dpid) == portentries.end()) ||
				(portentries.at(dpid).find(portno) == portentries.at(dpid).end())) {
			return;
		}
		portnames[dpid].erase(portno);
		portentries[dpid].erase(portno);
	};

	/**
	 *
	 */
	bool
	has_port_entry(const rofl::cdpid& dpid, uint32_t portno) const {
		if (portentries.find(dpid) == portentries.end()) {
			return false;
		}
		return (not (portentries.at(dpid).find(portno) == portentries.at(dpid).end()));
	};

public:

	/**
	 *
	 */
	const std::set<std::string>&
	get_eth_entries(const rofl::cdpid& dpid) const {
		return ethnames[dpid];
	};

	/**
	 *
	 */
	cethentry&
	add_eth_entry(const rofl::cdpid& dpid,
					const std::string& devname,
					const rofl::caddress_ll& hwaddr,
					uint16_t port_vid) {
		if (ethentries[dpid].find(devname) != ethentries[dpid].end()) {
			ethentries[dpid].erase(devname);
		}
		ethnames[dpid].insert(devname);
		return (ethentries[dpid][devname] = cethentry(devname, hwaddr, port_vid));
	};

	/**
	 *
	 */
	cethentry&
	set_eth_entry(const rofl::cdpid& dpid,
					const std::string& devname,
					const rofl::caddress_ll& hwaddr,
					uint16_t port_vid) {
		if (ethentries[dpid].find(devname) == ethentries[dpid].end()) {
			(void)ethentries[dpid][devname];
			ethnames[dpid].insert(devname);
		}
		return (ethentries[dpid][devname] = cethentry(devname, hwaddr, port_vid));
	};

	/**
	 *
	 */
	cethentry&
	set_eth_entry(const rofl::cdpid& dpid, const std::string& devname) {
		if (ethentries[dpid].find(devname) == ethentries[dpid].end()) {
			(void)ethentries[dpid][devname];
		}
		return (ethentries[dpid][devname]);
	};

	/**
	 *
	 */
	const cethentry&
	get_eth_entry(const rofl::cdpid& dpid, const std::string& devname) const {
		if ((ethentries.find(dpid) == ethentries.end()) ||
				(ethentries.at(dpid).find(devname) == ethentries.at(dpid).end())) {
			throw ePortDBNotFound("cethdb::get_eth_entry() devname not found");
		}
		return ethentries.at(dpid).at(devname);
	};

	/**
	 *
	 */
	void
	drop_eth_entry(const rofl::cdpid& dpid, const std::string& devname) {
		if ((ethentries.find(dpid) == ethentries.end()) ||
				(ethentries.at(dpid).find(devname) == ethentries.at(dpid).end())) {
			return;
		}
		ethnames[dpid].erase(devname);
		ethentries[dpid].erase(devname);
	};

	/**
	 *
	 */
	bool
	has_eth_entry(const rofl::cdpid& dpid, const std::string& devname) const {
		if (ethentries.find(dpid) == ethentries.end()) {
			return false;
		}
		return (not (ethentries.at(dpid).find(devname) == ethentries.at(dpid).end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cportdb& portdb) {
		os << rofcore::indent(0) << "<cportdb >" << std::endl;
		rofcore::indent i(2);
		for (std::map<rofl::cdpid, std::map<uint32_t, cportentry> >::const_iterator
				it = portdb.portentries.begin(); it != portdb.portentries.end(); ++it) {
			os << rofcore::indent(0) << "<cdpid: " << it->first.str() << " >" << std::endl;
			rofcore::indent j(2);
			for (std::map<uint32_t, cportentry>::const_iterator
					jt = it->second.begin(); jt != it->second.end(); ++jt) {
				os << jt->second;
			}
		}
		for (std::map<rofl::cdpid, std::map<std::string, cethentry> >::const_iterator
				it = portdb.ethentries.begin(); it != portdb.ethentries.end(); ++it) {
			os << rofcore::indent(0) << "<cdpid: " << it->first.str() << " >" << std::endl;
			rofcore::indent j(2);
			for (std::map<std::string, cethentry>::const_iterator
					jt = it->second.begin(); jt != it->second.end(); ++jt) {
				os << jt->second;
			}
		}
		return os;
	};

private:

	std::map<rofl::cdpid, std::map<uint32_t, cportentry> >		portentries;
	mutable std::map<rofl::cdpid, std::set<uint32_t> >			portnames;
	std::map<rofl::cdpid, std::map<std::string, cethentry> >	ethentries;
	mutable std::map<rofl::cdpid, std::set<std::string> >		ethnames;
	static std::map<std::string, cportdb*> 						portdbs;
};

}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CPORTCONF_HPP_ */
