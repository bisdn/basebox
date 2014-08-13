/*
 * clink.hpp
 *
 *  Created on: 28.06.2013
 *      Author: andreas
 */

#ifndef CLINK_HPP_
#define CLINK_HPP_ 1

#include <inttypes.h>

#include <bitset>

#include <rofl/common/logging.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/cofflowmod.h>

#include "ctapdev.h"
#include "cnetlink.h"
#include "cpacketpool.h"
#include "caddr.hpp"
#include "cneigh.hpp"

namespace ipcore {

class eLinkBase 				: public std::runtime_error {
public:
	eLinkBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
//class eLinkCritical 			: public eLinkBase {};
class eLinkNoDptAttached		: public eLinkBase {
public:
	eLinkNoDptAttached(const std::string& __arg) : eLinkBase(__arg) {};
};
class eLinkTapDevNotFound		: public eLinkBase {
public:
	eLinkTapDevNotFound(const std::string& __arg) : eLinkBase(__arg) {};
};
class eLinkNotFound				: public eLinkBase {
public:
	eLinkNotFound(const std::string& __arg) : eLinkBase(__arg) {};
};
class eAddrBase 				: public std::runtime_error {
public:
	eAddrBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eAddrNotFound				: public eAddrBase {
public:
	eAddrNotFound(const std::string& __arg) : eAddrBase(__arg) {};
};
class eNeighBase 				: public std::runtime_error {
public:
	eNeighBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eNeighNotFound			: public eNeighBase {
public:
	eNeighNotFound(const std::string& __arg) : eNeighBase(__arg) {};
};


class clink :
		public rofcore::cnetlink_common_observer,
		public rofcore::cnetdev_owner
{
public:

	/**
	 *
	 */
	clink();

	/**
	 *
	 */
	clink(uint32_t ofp_port_no, const std::string& devname, const rofl::caddress_ll& hwaddr);

	/**
	 *
	 */
	virtual
	~clink();

	/**
	 *
	 */
	clink(const clink& link) { *this = link; };

	/**
	 *
	 */
	clink&
	operator= (const clink& link) {
		if (this == &link)
			return *this;
		clear_addrs();
		for (std::map<uint32_t, caddr_in4>::const_iterator
				it = link.addrs_in4.begin(); it != link.addrs_in4.end(); ++it) {
			add_addr_in4(it->first) = it->second;
		}
		for (std::map<uint32_t, caddr_in6>::const_iterator
				it = link.addrs_in6.begin(); it != link.addrs_in6.end(); ++it) {
			add_addr_in6(it->first) = it->second;
		}
		clear_neighs();
		for (std::map<uint32_t, cneigh_in4>::const_iterator
				it = link.neighs_in4.begin(); it != link.neighs_in4.end(); ++it) {
			add_neigh_in4(it->first) = it->second;
		}
		for (std::map<uint32_t, cneigh_in6>::const_iterator
				it = link.neighs_in6.begin(); it != link.neighs_in6.end(); ++it) {
			add_neigh_in6(it->first) = it->second;
		}
		return *this;
	};

public:

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

	/**
	 *
	 */
	void
	set_dptid(const rofl::cdptid& dptid) {
		this->dptid = dptid;
		//addrtable.set_dptid(dptid);
		//neightable.set_dptid(dptid);
	};

	/**
	 *
	 */
	uint32_t
	get_ofp_port_no() const { return ofp_port_no; };

	/**
	 *
	 */
	void
	set_ofp_port_no(uint32_t ofp_port_no) { this->ofp_port_no = ofp_port_no; };

	/**
	 *
	 */
	const rofl::openflow::cofport&
	get_ofp_port() const { return rofl::crofdpt::get_dpt(dptid).get_ports().get_port(ofp_port_no); };

	/**
	 *
	 */
	std::string
	get_devname() const { return get_ofp_port().get_name(); };

	/**
	 *
	 */
	rofl::cmacaddr
	get_hwaddr() const { return get_ofp_port().get_hwaddr(); };

	/**
	 *
	 */
	unsigned int
	get_ifindex() const { return ifindex; }

	/**
	 *
	 */
	void
	tap_open();

	/**
	 *
	 */
	void
	tap_close();

public:

	/**
	 *
	 */
	void
	clear_addrs() { addrs_in4.clear(); addrs_in6.clear(); };

	/**
	 *
	 */
	caddr_in4&
	add_addr_in4(unsigned int adindex) {
		if (addrs_in4.find(adindex) != addrs_in4.end()) {
			addrs_in4.erase(adindex);
		}
		addrs_in4[adindex] = caddr_in4(ifindex, adindex, dptid, /*table_id=*/0);
		if (STATE_ATTACHED == state) {
			addrs_in4[adindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
		}
		return addrs_in4[adindex];
	};

	/**
	 *
	 */
	caddr_in4&
	set_addr_in4(unsigned int adindex) {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			addrs_in4[adindex] = caddr_in4(ifindex, adindex, dptid, /*table_id=*/0);
			if (STATE_ATTACHED == state) {
				addrs_in4[adindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		}
		return addrs_in4[adindex];
	};

	/**
	 *
	 */
	const caddr_in4&
	get_addr_in4(unsigned int adindex) const {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			throw eAddrNotFound("clink::get_addr_in4() adindex not found");
		}
		return addrs_in4.at(adindex);
	};

	/**
	 *
	 */
	void
	drop_addr_in4(unsigned int adindex) {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			return;
		}
		addrs_in4.erase(adindex);
	};

	/**
	 *
	 */
	bool
	has_addr_in4(unsigned int adindex) const {
		return (not (addrs_in4.find(adindex) == addrs_in4.end()));
	};


	/**
	 *
	 */
	caddr_in6&
	add_addr_in6(unsigned int adindex) {
		if (addrs_in6.find(adindex) != addrs_in6.end()) {
			addrs_in6.erase(adindex);
		}
		addrs_in6[adindex] = caddr_in6(ifindex, adindex, dptid, /*table_id=*/0);
		if (STATE_ATTACHED == state) {
			addrs_in6[adindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
		}
		return addrs_in6[adindex];
	};

	/**
	 *
	 */
	caddr_in6&
	set_addr_in6(unsigned int adindex) {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			addrs_in6[adindex] = caddr_in6(ifindex, adindex, dptid, /*table_id=*/0);
			if (STATE_ATTACHED == state) {
				addrs_in6[adindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		}
		return addrs_in6[adindex];
	};

	/**
	 *
	 */
	const caddr_in6&
	get_addr_in6(unsigned int adindex) const {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			throw eAddrNotFound("clink::get_addr_in6() adindex not found");
		}
		return addrs_in6.at(adindex);
	};

	/**
	 *
	 */
	void
	drop_addr_in6(unsigned int adindex) {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			return;
		}
		addrs_in6.erase(adindex);
	};

	/**
	 *
	 */
	bool
	has_addr_in6(unsigned int adindex) const {
		return (not (addrs_in6.find(adindex) == addrs_in6.end()));
	};

public:

	/**
	 *
	 */
	void
	clear_neighs() { neighs_in4.clear(); neighs_in6.clear(); };

	/**
	 *
	 */
	cneigh_in4&
	add_neigh_in4(unsigned int nbindex) {
		if (neighs_in4.find(nbindex) != neighs_in4.end()) {
			neighs_in4.erase(nbindex);
		}
		neighs_in4[nbindex] = cneigh_in4(ifindex, nbindex, dptid, /*table_id=*/3, get_ofp_port_no());
		if (STATE_ATTACHED == state) {
			neighs_in4[nbindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
		}
		return neighs_in4[nbindex];
	};

	/**
	 *
	 */
	cneigh_in4&
	set_neigh_in4(unsigned int nbindex) {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			neighs_in4[nbindex] = cneigh_in4(ifindex, nbindex, dptid, /*table_id=*/3, get_ofp_port_no());
			if (STATE_ATTACHED == state) {
				neighs_in4[nbindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		}
		return neighs_in4[nbindex];
	};

	/**
	 *
	 */
	const cneigh_in4&
	get_neigh_in4(unsigned int nbindex) const {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() nbindex not found");
		}
		return neighs_in4.at(nbindex);
	};

	/**
	 *
	 */
	void
	drop_neigh_in4(unsigned int nbindex) {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			return;
		}
		neighs_in4.erase(nbindex);
	};

	/**
	 *
	 */
	bool
	has_neigh_in4(unsigned int nbindex) const {
		return (not (neighs_in4.find(nbindex) == neighs_in4.end()));
	};

	/**
	 *
	 */
	const cneigh_in4&
	get_neigh_in4(const rofl::caddress_in4& dst) const {
		std::map<unsigned int, cneigh_in4>::const_iterator it;
		if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
				cneigh_in4_find_by_dst(dst))) == neighs_in4.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() dst not found");
		}
		return it->second;
	};

	/**
	 *
	 */
	cneigh_in6&
	add_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) != neighs_in6.end()) {
			neighs_in6.erase(nbindex);
		}
		neighs_in6[nbindex] = cneigh_in6(ifindex, nbindex, dptid, /*table_id=*/3, get_ofp_port_no());
		if (STATE_ATTACHED == state) {
			neighs_in6[nbindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
		}
		return neighs_in6[nbindex];
	};

	/**
	 *
	 */
	cneigh_in6&
	set_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			neighs_in6[nbindex] = cneigh_in6(ifindex, nbindex, dptid, /*table_id=*/3, get_ofp_port_no());
			if (STATE_ATTACHED == state) {
				neighs_in6[nbindex].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		}
		return neighs_in6[nbindex];
	};

	/**
	 *
	 */
	const cneigh_in6&
	get_neigh_in6(unsigned int nbindex) const {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() nbindex not found");
		}
		return neighs_in6.at(nbindex);
	};

	/**
	 *
	 */
	void
	drop_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			return;
		}
		neighs_in6.erase(nbindex);
	};

	/**
	 *
	 */
	bool
	has_neigh_in6(unsigned int nbindex) const {
		return (not (neighs_in6.find(nbindex) == neighs_in6.end()));
	};

	/**
	 *
	 */
	const cneigh_in6&
	get_neigh_in6(const rofl::caddress_in6& dst) const {
		std::map<unsigned int, cneigh_in6>::const_iterator it;
		if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
				cneigh_in6_find_by_dst(dst))) == neighs_in6.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() dst not found");
		}
		return it->second;
	};

public:

	/**
	 *
	 */
	void
	clear() {
		clear_addrs(); clear_neighs();
	};

	/*
	 * from ctapdev
	 */
	virtual void
	enqueue(
			rofcore::cnetdev *netdev, rofl::cpacket* pkt);

	virtual void
	enqueue(
			rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts);


	/*
	 * from crofbase
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt);

	void
	handle_dpt_close(rofl::crofdpt& dpt);

	void
	handle_packet_in(
			rofl::cpacket const& pkt);

	void
	handle_port_status(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_port_status& msg);


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, clink const& link) {
		os << rofcore::indent(0) << "<dptlink: ifindex:" << (int)link.ifindex << " "
				<< "devname:" << link.get_devname() << " >" << std::endl;
		os << rofcore::indent(2) << "<hwaddr:"  << link.get_hwaddr() << " "
				<< "portno:"  << (int)link.ofp_port_no << " >" << std::endl;
		try {
			rofcore::indent i(2);
			os << rofcore::cnetlink::get_instance().get_links().get_link(link.ifindex);
		} catch (rofcore::eNetLinkNotFound& e) {
			os << rofl::indent(2) << "<no crtlink found >" << std::endl;
		}
		rofcore::indent i(2);
		for (std::map<unsigned int, caddr_in4>::const_iterator
				it = link.addrs_in4.begin(); it != link.addrs_in4.end(); ++it) {
			os << it->second;
		}
		for (std::map<unsigned int, caddr_in6>::const_iterator
				it = link.addrs_in6.begin(); it != link.addrs_in6.end(); ++it) {
			os << it->second;
		}
		for (std::map<unsigned int, cneigh_in4>::const_iterator
				it = link.neighs_in4.begin(); it != link.neighs_in4.end(); ++it) {
			os << it->second;
		}
		for (std::map<unsigned int, cneigh_in6>::const_iterator
				it = link.neighs_in6.begin(); it != link.neighs_in6.end(); ++it) {
			os << it->second;
		}
		return os;
	};


	/**
	 *
	 */
	class clink_by_ofp_port_no {
		uint32_t ofp_port_no;
	public:
		clink_by_ofp_port_no(uint32_t ofp_port_no) : ofp_port_no(ofp_port_no) {};
		bool operator() (const std::pair<unsigned int, clink>& p) {
			return (ofp_port_no == p.second.ofp_port_no);
		};
	};


	/**
	 *
	 */
	class clink_by_ifindex {
		int ifindex;
	public:
		clink_by_ifindex(int ifindex) : ifindex(ifindex) {};
		bool operator() (const std::pair<unsigned int, clink>& p) {
			return (ifindex == p.second.ifindex);
		};
	};


	/**
	 *
	 */
	class clink_by_devname {
		std::string devname;
	public:
		clink_by_devname(const std::string& devname) : devname(devname) {};
		bool operator() (const std::pair<unsigned int, clink>& p) {
			return (devname == p.second.devname);
		};
	};

private:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t					state;
	uint32_t			 		 		ofp_port_no;	// OpenFlow portno assigned to port on dpt mapped to this dptport instance
	std::string							devname;
	rofl::caddress_ll					hwaddr;
	int					 		 		ifindex;		// ifindex for tapdevice
	rofl::cdptid						dptid;
	uint8_t								table_id;
	rofcore::ctapdev					*tapdev;		// tap device emulating the mapped port on this system

	enum cdptlink_flag_t {
		FLAG_TAP_DEVICE_ACTIVE = 1,
		FLAG_FLOW_MOD_INSTALLED = 2,
	};

	std::bitset<32>						flags;

	std::map<unsigned int, caddr_in4> 	addrs_in4;		// key: addr ofp port-no
	std::map<unsigned int, caddr_in6> 	addrs_in6;		// key: addr ofp port-no
	std::map<unsigned int, cneigh_in4> 	neighs_in4;		// key: neigh ofp port-no
	std::map<unsigned int, cneigh_in6> 	neighs_in6;		// key: neigh ofp port-no
};

}; // end of namespace

#endif /* DPTPORT_H_ */
