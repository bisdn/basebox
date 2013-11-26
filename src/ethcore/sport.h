/*
 * port.h
 *
 *  Created on: 26.11.2013
 *      Author: andreas
 */

#ifndef PORT_H_
#define PORT_H_

#include <iostream>
#include <exception>

#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>
#ifdef __cplusplus
}
#endif


#include <rofl/common/logging.h>

namespace ethercore
{
class eSportBase : public std::exception {};
class eSportExists : public eSportBase {};

class sport
{
	static std::map<uint64_t, std::map<uint32_t, sport*> > sports;
private:
	enum sport_pvid_t {
		SPORT_DEFAULT_PVID = 1,
	};

	uint64_t dpid;
	uint32_t portno;
	uint16_t pvid; // (1<<12) untagged
	std::map <uint16_t, bool> memberships;
public:
	/**
	 * @brief	Returns or creates an sport instance
	 *
	 * @param dpid OF data path identifier
	 * @param portno OF port number
	 * @param pvid default VLAN identifier (VID)
	 *
	 * @return reference to the existing or new sport instance
	 */
	static
	sport&
	get_sport(uint64_t dpid, uint32_t portno, uint16_t pvid = SPORT_DEFAULT_PVID);

	/**
	 * @brief	Constructor for switch-port, defining dpid, portno and pvid
	 *
	 * Adds this sport instance to static map sport::sports.
	 * @param dpid data path identifier of OF data path instance the port is located on
	 * @param portno OF port number assigned to corresponding node
	 * @param pvid default port vid used when sending/receiving an untagged packet
	 *
	 * @throw eSportExists when an sport instance for the specified [dpid,portno] pair already exists
	 */
	sport(uint64_t dpid, uint32_t portno, uint16_t pvid);

	/**
	 * @brief	Destructor for switch-port
	 *
	 * Removes this sport instance from static map sport::sports.
	 */
	virtual
	~sport();

	/**
	 * @brief	Returns the data path identifier of the associated dpt this port is located on
	 */
	uint64_t
	get_dpid() const { return dpid; };

	/**
	 * @brief	Returns the port's number assigned by OpenFlow
	 */
	uint32_t
	get_portno() const { return portno; };

	/**
	 * @brief	Returns the default VID assigned to this port (pvid)
	 */
	uint16_t
	get_pvid() const { return pvid; };

	/**
	 * @brief	Add a VLAN membership to this port.
	 *
	 * @param vid VID this node becomes member of
	 * @param tagged boolean variable indicating whether to add this port in tagged/untagged mode
	 */
	void
	add_membership(uint16_t vid, bool tagged = true);

	/**
	 * @brief	Drop a VLAN membership from this port.
	 *
	 * @param vid VID this node drops membership of
	 */
	void
	drop_membership(uint16_t vid);

public:

	friend std::ostream&
	operator<< (std::ostream& os, sport const& sp) {
		os << "<sport ";
			os << "dpid:" << (unsigned long long)sp.dpid << " ";
			os << "portno:" << (unsigned long)sp.portno << " ";
			os << "pvid:" << (int)sp.pvid << " ";
			os << "memberships: " << " ";
			for (std::map<uint16_t, bool>::const_iterator
					it = sp.memberships.begin(); it != sp.memberships.end(); ++it) {
				os << (int)(it->first) << (it->second ? "(t)":"(u)") << " ";
			}
		os << ">";
		return os;
	};
};

}; // end of namespace


#endif /* PORT_H_ */
