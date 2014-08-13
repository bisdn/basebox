/*
 * clinktable.h
 *
 *  Created on: 02.07.2014
 *      Author: andreas
 */

#ifndef CLINKTABLE_H_
#define CLINKTABLE_H_

#include <map>
#include <iostream>
#include <algorithm>

#include "cdptlink.h"
#include "clogging.h"

namespace ipcore {

class eLinkTableBase		: public std::exception {};
class eLinkTableExists		: public eLinkTableBase {};
class eLinkTableNotFound 	: public eLinkTableBase {};

class clinktable {
public:

	/**
	 *
	 */
	clinktable() {};

	/**
	 *
	 */
	~clinktable() {};

	/**
	 *
	 */
	clinktable(
			const clinktable& table) { *this = table; };

	/**
	 *
	 */
	clinktable&
	operator= (
			const clinktable& table) {
		if (this == &table)
			return *this;
		clear();
		for (std::map<uint32_t, clink>::const_iterator
				it = table.links.begin(); it != table.links.end(); ++it) {
			add_link(it->first) = it->second;
		}
		return *this;
	};

	/**
	 *
	 */
	void
	install() {
		for (std::map<uint32_t, clink>::iterator
				it = links.begin(); it != links.end(); ++it) {
			it->second.install();
		}
	};

	/**
	 *
	 */
	void
	uninstall() {
		for (std::map<uint32_t, clink>::iterator
				it = links.begin(); it != links.end(); ++it) {
			it->second.uninstall();
		}
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
		for (std::map<uint32_t, clink>::iterator
				it = links.begin(); it != links.end(); ++it) {
			it->second.set_dptid(dptid);
		}
	};

public:

	/**
	 *
	 */
	void
	clear() { links.clear(); };

	/**
	 *
	 */
	clink&
	add_link(uint32_t ofp_port_no) {
		if (links.find(ofp_port_no) != links.end()) {
			links.erase(ofp_port_no);
		}
		links[ofp_port_no].set_ofp_port_no(ofp_port_no);
		return links[ofp_port_no];
	};

	/**
	 *
	 */
	clink&
	set_link(uint32_t ofp_port_no) {
		if (links.find(ofp_port_no) == links.end()) {
			links[ofp_port_no].set_ofp_port_no(ofp_port_no);
		}
		return links[ofp_port_no];
	};

	/**
	 *
	 */
	const clink&
	get_link(uint32_t ofp_port_no) const {
		if (links.find(ofp_port_no) == links.end()) {
			throw eLinkTableNotFound();
		}
		return links.at(ofp_port_no);
	};

	/**
	 *
	 */
	void
	drop_link(uint32_t ofp_port_no) {
		if (links.find(ofp_port_no) == links.end()) {
			return;
		}
		links.erase(ofp_port_no);
	};

	/**
	 *
	 */
	bool
	has_link(uint32_t ofp_port_no) const {
		return (not (links.find(ofp_port_no) == links.end()));
	};

	/**
	 *
	 */
	const clink&
	get_link_by_ofp_port_no(uint32_t ofp_port_no) const {
		std::map<uint32_t, clink>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(), clink::clink_by_ofp_port_no(ofp_port_no))) == links.end()) {
			throw eLinkTableNotFound();
		}
		return it->second;
	};

	/**
	 *
	 */
	bool
	has_link_by_ofp_port_no(uint32_t ofp_port_no) const {
		std::map<uint32_t, clink>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(), clink::clink_by_ofp_port_no(ofp_port_no))) == links.end()) {
			return false;
		}
		return true;
	};

	/**
	 *
	 */
	const clink&
	get_link_by_ifindex(int ifindex) const {
		std::map<uint32_t, clink>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(), clink::clink_by_ifindex(ifindex))) == links.end()) {
			throw eLinkTableNotFound();
		}
		return it->second;
	};

	/**
	 *
	 */
	bool
	has_link_by_ifindex(int ifindex) const {
		std::map<uint32_t, clink>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(), clink::clink_by_ifindex(ifindex))) == links.end()) {
			return false;
		}
		return true;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const clinktable& table) {
		os << rofcore::indent(0) << "<clinktable >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, clink>::const_iterator
				it = table.links.begin(); it != table.links.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	rofl::cdptid					dptid;
	std::map<uint32_t, clink> 	links;	// key: link ofp port-no
};

}; // end of namespace ipcore

#endif /* CLNKTABLE_H_ */
