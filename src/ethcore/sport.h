/*
 * port.h
 *
 *  Created on: 26.11.2013
 *      Author: andreas
 */

#ifndef PORT_H_
#define PORT_H_

#include <map>
#include <iostream>
#include <exception>

#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>
#include <inttypes.h>
#ifdef __cplusplus
}
#endif


#include <rofl/common/logging.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cflowentry.h>
#include <rofl/common/openflow/cofdpt.h>

namespace ethercore
{
class eSportBase 		: public std::exception {};
class eSportExists 		: public eSportBase {};
class eSportInval		: public eSportBase {};
class eSportNotFound	: public eSportBase {};

class sport_owner
{
public:
	virtual ~sport_owner() {};
	virtual rofl::crofbase* get_rofbase() = 0;
};

class sport
{
	static std::map<uint64_t, std::map<uint32_t, sport*> > sports;
private:
	enum sport_pvid_t {
		SPORT_DEFAULT_PVID = 1,
	};

	sport_owner	   *spowner;
	uint64_t 		dpid;
	uint32_t 		portno;
	uint16_t 		pvid;
	std::map <uint16_t, bool> memberships;
	uint8_t  		table_id;

public:
	/**
	 * @brief	Returns or creates an sport instance
	 *
	 * @param dpid OF data path identifier
	 * @param portno OF port number
	 *
	 * @return reference to the existing or new sport instance
	 */
	static
	sport&
	get_sport(uint64_t dpid, uint32_t portno);

	/**
	 * @brief	Constructor for switch-port, defining dpid, portno and pvid
	 *
	 * Adds this sport instance to static map sport::sports.
	 * @param dpid data path identifier of OF data path instance the port is located on
	 * @param portno OF port number assigned to corresponding node
	 * @param pvid default port vid used when sending/receiving an untagged packet
	 * @param table_id table_id for setting the ACLs for incoming packets
	 *
	 * @throw eSportExists when an sport instance for the specified [dpid,portno] pair already exists
	 */
	sport(sport_owner *spowner, uint64_t dpid, uint32_t portno, uint16_t pvid, uint8_t table_id);

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

	/**
	 * @brief	Output operator
	 */
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

private:

	/**
	 *
	 */
	void
	flow_mod_add(uint16_t vid, bool tagged);

	/**
	 *
	 */
	void
	flow_mod_delete(uint16_t vid, bool tagged);
};

}; // end of namespace


#endif /* PORT_H_ */
