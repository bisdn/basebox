/*
 	 	 * port.h
 *
 *  Created on: 26.11.2013
 *      Author: andreas
 */

#ifndef PORT_H_
#define PORT_H_

#include <map>
#include <bitset>
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


#include <rofl/common/ciosrv.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cflowentry.h>
#include <rofl/common/openflow/cofdpt.h>

#include "logging.h"

namespace ethercore
{
class eSportBase 		: public std::exception {};
class eSportExists 		: public eSportBase {};
class eSportInval		: public eSportBase {};
class eSportNotFound	: public eSportBase {};
class eSportExhausted	: public eSportBase {};
class eSportFailed		: public eSportBase {};
class eSportNoPvid		: public eSportBase {};
class eSportNotMember	: public eSportBase {};

class sport_owner
{
public:
	/**
	 * @brief	Destructor for vtable
	 */
	virtual ~sport_owner() {};
	/**
	 * @brief	Returns a handle to the underlying rofl::crofbase instance acting as OF endpoint
	 * @return Pointer to rofl::crofbase instance
	 */
	virtual rofl::crofbase* get_rofbase() = 0;
	/**
	 * @brief	Returns an idle OpenFlow group-id for creating a group table entry
	 * @return assigned OF group-id
	 * @throw eSportExhausted when no idle group-ids are available
	 */
	virtual uint32_t get_idle_group_id() = 0;
	/**
	 * @brief	Release a previously assigned group-id for later reuse
	 * @param group_id assigned OF group-id
	 */
	virtual void release_group_id(uint32_t group_id) = 0;
};

class sport :
		public rofl::ciosrv
{
	struct vlan_membership_t {
		uint16_t vid;
		bool tagged;
		uint32_t group_id;
	public:
		vlan_membership_t(uint16_t vid = 0, bool tagged = false, uint32_t group_id = 0) :
			vid(vid), tagged(tagged), group_id(group_id) {};
		vlan_membership_t(struct vlan_membership_t const& m) {
			*this = m;
		};
		struct vlan_membership_t&
		operator= (struct vlan_membership_t const& m) {
			if (this == &m)
				return *this;
			vid 		= m.vid;
			tagged 		= m.tagged;
			group_id 	= m.group_id;
			return *this;
		};
		uint16_t get_vid() const { return vid; };
		bool get_tagged() const { return tagged; };
		uint32_t get_group_id() const { return group_id; };
		friend std::ostream&
		operator<< (std::ostream& os, struct vlan_membership_t const& membership) {
			os << "<vlan_membership_t ";
				os << "vid:" << (int)membership.vid << (membership.tagged ? "(t)":"(u)") << " ";
				os << "group-id:" << (int)membership.group_id << " ";
			os << ">";
			return os;
		};
	};

	static std::map<uint64_t, std::map<uint32_t, sport*> > sports;
private:
	enum sport_pvid_t {
		SPORT_DEFAULT_PVID = 1,
	};

	enum sport_flag_t {
		SPORT_FLAG_PVID_VALID = (1 << 0),
	};

	sport_owner	   *spowner;
	uint64_t 		dpid;
	uint32_t 		portno;
	std::string		devname;
	std::bitset<64>	flags;
	uint16_t 		pvid;
	std::map <uint16_t, struct vlan_membership_t> memberships;
	uint8_t  		table_id;
	unsigned int	timer_dump_interval;

#define DEFAULT_SPORT_DUMP_INTERVAL 10

	enum sport_timer_t {
		SPORT_TIMER_DUMP = 1,
	};

	/**
	 * @brief	Suppress copy operations on sport instances (make copy constructor private)
	 */
	sport(sport const& sp) : spowner(NULL), dpid(0), portno(0), pvid(0), table_id(0) {};

public:
	/**
	 * @brief	Returns or creates an sport instance
	 *
	 * @param dpid OF data path identifier
	 * @param portno OF port number
	 *
	 * @return reference to the existing
	 * @throw eSportNotFound is thrown when no sport instance exists for the specified <dpid,portno> pair
	 */
	static
	sport&
	get_sport(uint64_t dpid, uint32_t portno);

	/**
	 * @brief	Returns or creates an sport instance
	 *
	 * @param dpid OF data path identifier
	 * @param devname port devname
	 *
	 * @return reference to the existing
	 * @throw eSportNotFound is thrown when no sport instance exists for the specified <dpid,portno> pair
	 */
	static
	sport&
	get_sport(uint64_t dpid, std::string const& devname);

	/**
	 * @brief	Destroys all sport instances associated with a specific dpid
	 *
	 * @param dpid
	 */
	static
	void
	destroy_sports(uint64_t dpid);

	/**
	 * @brief	Destroys all sport instances
	 *
	 */
	static
	void
	destroy_sports();

public:
	/**
	 * @brief	Constructor for switch-port, defining dpid, portno and pvid
	 *
	 * Adds this sport instance to static map sport::sports.
	 * @param dpid data path identifier of OF data path instance the port is located on
	 * @param portno OF port number assigned to corresponding node
	 * @param table_id table_id for setting the ACLs for incoming packets
	 *
	 * @throw eSportExists when an sport instance for the specified [dpid,portno] pair already exists
	 */
	sport(
			sport_owner *spowner,
			uint64_t dpid,
			uint32_t portno,
			std::string const& devname,
			uint8_t table_id,
			unsigned int timer_dump_interval = DEFAULT_SPORT_DUMP_INTERVAL);

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
	 * @brief	Returns the port's number assigned by OpenFlow
	 */
	std::string const&
	get_devname() const { return devname; };

	/**
	 * @brief	Returns the default VID assigned to this port (pvid)
	 */
	uint16_t
	get_pvid() const;

	/**
	 * @brief	Returns the group-id assigned to this port for a specific VID for sending packets out
	 * @param vid VLAN identifier this port is supposed to be member of
	 * @throw eSportInval is thrown for an invalid VID (port is not member of this VLAN)
	 */
	uint32_t
	get_groupid(uint16_t vid);

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

	/**
	 * @brief	Returns the group-table-entry-id used by this sport for a specific VLAN
	 *
	 * @param vid VID for which the group-id is sought for
	 */
	void
	get_group_id(uint16_t vid) const;

	/**
	 * @brief	Returns the tagged flag for the specified VLAN vid.
	 *
	 * @param vid VLAN identifier
	 * @throw SportNotMember is thrown when this port is not member of the specified VLAN.
	 */
	bool
	get_is_tagged(uint16_t vid);

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
		os << ">";
		for (std::map<uint16_t, struct vlan_membership_t>::const_iterator
				it = sp.memberships.begin(); it != sp.memberships.end(); ++it) {
			os << std::endl << "\t" << it->second << " ";
		}
		return os;
	};

private:

	/**
	 *
	 */
	virtual void
	handle_timeout(int opaque);

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

public:

	class sport_find_by_devname {
		std::string devname;
	public:
		sport_find_by_devname(std::string const& devname) :
			devname(devname) {};
		bool operator() (std::pair<uint32_t, sport*> const& p) {
			return (p.second->get_devname() == devname);
		};
	};
};

}; // end of namespace


#endif /* PORT_H_ */
