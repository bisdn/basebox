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

#include "cnetlink.hpp"
#include "caddr.hpp"
#include "cneigh.hpp"

namespace roflibs {
namespace ip {

class eLinkBase 				: public std::runtime_error {
public:
	eLinkBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
//class eLinkCritical 			: public eLinkBase {};
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
		public rofcore::cnetlink_common_observer
{
public:

	/**
	 *
	 */
	clink();

	/**
	 *
	 */
	clink(
			const rofl::cdpid& dpid,
			int ifindex,
			const std::string& devname,
			const rofl::caddress_ll& hwaddr,
			uint8_t in_ofp_table_id,
			uint8_t out_ofp_table_id,
			bool tagged = false,
			uint16_t vid = 0xffff);

	/**
	 *
	 */
	virtual
	~clink();

public:

	/**
	 *
	 */
	const rofl::cdpid&
	get_dpid() const { return dpid; };

	/**
	 *
	 */
	void
	set_dpid(const rofl::cdpid& dpid) {
		this->dpid = dpid;
		//addrtable.set_dpid(dpid);
		//neightable.set_dpid(dpid);
	};


	/**
	 *
	 */
	std::string
	get_devname() const { return devname; };

	/**
	 *
	 */
	rofl::cmacaddr
	get_hwaddr() const { return hwaddr; };

	/**
	 *
	 */
	unsigned int
	get_ifindex() const { return ifindex; }

	/**
	 *
	 */
	bool
	get_vlan_tagged() const { return tagged; };

	/**
	 *
	 */
	uint16_t
	get_vlan_vid() const { return vid; };


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
		addrs_in4[adindex] = new caddr_in4(ifindex, adindex, dpid, in_ofp_table_id);
		if (STATE_ATTACHED == state) {
			addrs_in4[adindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return *(addrs_in4[adindex]);
	};

	/**
	 *
	 */
	caddr_in4&
	set_addr_in4(unsigned int adindex) {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			addrs_in4[adindex] = new caddr_in4(ifindex, adindex, dpid, in_ofp_table_id);
			if (STATE_ATTACHED == state) {
				addrs_in4[adindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return *(addrs_in4[adindex]);
	};

	/**
	 *
	 */
	const caddr_in4&
	get_addr_in4(unsigned int adindex) const {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			throw eAddrNotFound("clink::get_addr_in4() adindex not found");
		}
		return *(addrs_in4.at(adindex));
	};

	/**
	 *
	 */
	void
	drop_addr_in4(unsigned int adindex) {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			return;
		}
		delete addrs_in4[adindex];
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
		addrs_in6[adindex] = new caddr_in6(ifindex, adindex, dpid, in_ofp_table_id);
		if (STATE_ATTACHED == state) {
			addrs_in6[adindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return *(addrs_in6[adindex]);
	};

	/**
	 *
	 */
	caddr_in6&
	set_addr_in6(unsigned int adindex) {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			addrs_in6[adindex] = new caddr_in6(ifindex, adindex, dpid, in_ofp_table_id);
			if (STATE_ATTACHED == state) {
				addrs_in6[adindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return *(addrs_in6[adindex]);
	};

	/**
	 *
	 */
	const caddr_in6&
	get_addr_in6(unsigned int adindex) const {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			throw eAddrNotFound("clink::get_addr_in6() adindex not found");
		}
		return *(addrs_in6.at(adindex));
	};

	/**
	 *
	 */
	void
	drop_addr_in6(unsigned int adindex) {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			return;
		}
		delete addrs_in6[adindex];
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
		neighs_in4[nbindex] = new cneigh_in4(ifindex, nbindex, dpid, out_ofp_table_id);
		if (STATE_ATTACHED == state) {
			neighs_in4[nbindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return *(neighs_in4[nbindex]);
	};

	/**
	 *
	 */
	cneigh_in4&
	set_neigh_in4(unsigned int nbindex) {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			neighs_in4[nbindex] = new cneigh_in4(ifindex, nbindex, dpid, out_ofp_table_id);
			if (STATE_ATTACHED == state) {
				neighs_in4[nbindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return *(neighs_in4[nbindex]);
	};

	/**
	 *
	 */
	const cneigh_in4&
	get_neigh_in4(unsigned int nbindex) const {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() nbindex not found");
		}
		return *(neighs_in4.at(nbindex));
	};

	/**
	 *
	 */
	void
	drop_neigh_in4(unsigned int nbindex) {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			return;
		}
		delete neighs_in4[nbindex];
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
		std::map<unsigned int, cneigh_in4*>::const_iterator it;
		if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
				cneigh_in4_find_by_dst(dst))) == neighs_in4.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() dst not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	cneigh_in6&
	add_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) != neighs_in6.end()) {
			neighs_in6.erase(nbindex);
		}
		neighs_in6[nbindex] = new cneigh_in6(ifindex, nbindex, dpid, out_ofp_table_id);
		if (STATE_ATTACHED == state) {
			neighs_in6[nbindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
		}
		return *(neighs_in6[nbindex]);
	};

	/**
	 *
	 */
	cneigh_in6&
	set_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			neighs_in6[nbindex] = new cneigh_in6(ifindex, nbindex, dpid, out_ofp_table_id);
			if (STATE_ATTACHED == state) {
				neighs_in6[nbindex]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		}
		return *(neighs_in6[nbindex]);
	};

	/**
	 *
	 */
	const cneigh_in6&
	get_neigh_in6(unsigned int nbindex) const {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() nbindex not found");
		}
		return *(neighs_in6.at(nbindex));
	};

	/**
	 *
	 */
	void
	drop_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			return;
		}
		delete neighs_in6[nbindex];
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
		std::map<unsigned int, cneigh_in6*>::const_iterator it;
		if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
				cneigh_in6_find_by_dst(dst))) == neighs_in6.end()) {
			throw eNeighNotFound("clink::get_neigh_in4() dst not found");
		}
		return *(it->second);
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
	 * from crofbase
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt);

	void
	handle_dpt_close(rofl::crofdpt& dpt);


public:


	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, clink const& link) {
		os << rofcore::indent(0) << "<clink: "
				<< "ifindex:" << (int)link.ifindex << " "
				<< "devname:" << link.get_devname() << " "
				<< "vlan:{vid: " << (int)link.vid << ", tagged:" << (link.tagged ? "true":"false") << "} "
				<< " >" << std::endl;
		os << rofcore::indent(2) << "<hwaddr: >"  << std::endl;
		os << rofcore::indent(4) << link.get_hwaddr();
#if 0
		try {
			rofcore::indent i(2);
			os << rofcore::cnetlink::get_instance().get_links().get_link(link.ifindex);
		} catch (rofcore::eNetLinkNotFound& e) {
			os << rofl::indent(2) << "<no crtlink found >" << std::endl;
		} catch (rofcore::crtlink::eRtLinkNotFound& e) {
			os << rofl::indent(2) << "<no crtlink found >" << std::endl;
		}
#endif
		rofcore::indent i(2);
		for (std::map<unsigned int, caddr_in4*>::const_iterator
				it = link.addrs_in4.begin(); it != link.addrs_in4.end(); ++it) {
			os << *(it->second);
		}
		for (std::map<unsigned int, caddr_in6*>::const_iterator
				it = link.addrs_in6.begin(); it != link.addrs_in6.end(); ++it) {
			os << *(it->second);
		}
		for (std::map<unsigned int, cneigh_in4*>::const_iterator
				it = link.neighs_in4.begin(); it != link.neighs_in4.end(); ++it) {
			os << *(it->second);
		}
		for (std::map<unsigned int, cneigh_in6*>::const_iterator
				it = link.neighs_in6.begin(); it != link.neighs_in6.end(); ++it) {
			os << *(it->second);
		}
		return os;
	};


	/**
	 *
	 */
	class clink_by_ifindex {
		int ifindex;
	public:
		clink_by_ifindex(int ifindex) : ifindex(ifindex) {};
		bool operator() (const std::pair<unsigned int, clink*>& p) {
			return (ifindex == p.second->ifindex);
		};
	};


	/**
	 *
	 */
	class clink_by_devname {
		std::string devname;
	public:
		clink_by_devname(const std::string& devname) : devname(devname) {};
		bool operator() (const std::pair<unsigned int, clink*>& p) {
			return (devname == p.second->devname);
		};
	};

private:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t					state;
	std::string							devname;
	rofl::caddress_ll					hwaddr;
	int					 		 		ifindex;		// ifindex for tapdevice
	rofl::cdpid							dpid;
	uint8_t								in_ofp_table_id;
	uint8_t								out_ofp_table_id;
	bool								tagged;
	uint16_t							vid;

	enum cdptlink_flag_t {
		FLAG_TAP_DEVICE_ACTIVE = 1,
		FLAG_FLOW_MOD_INSTALLED = 2,
	};

	std::bitset<32>						flags;

	std::map<unsigned int, caddr_in4*> 	addrs_in4;		// key: addr ofp port-no
	std::map<unsigned int, caddr_in6*> 	addrs_in6;		// key: addr ofp port-no
	std::map<unsigned int, cneigh_in4*>	neighs_in4;		// key: neigh ofp port-no
	std::map<unsigned int, cneigh_in6*>	neighs_in6;		// key: neigh ofp port-no
};

}; // end of namespace ip
}; // end of namespace roflibs

#endif /* DPTPORT_H_ */
