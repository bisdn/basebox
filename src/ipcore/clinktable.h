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

#include "cdptlink.h"
#include "logging.h"

namespace ipcore {

class eLinkTableBase		: public std::exception {};
class eLinkTableExists		: public eLinkTableBase {};
class eLinkTableNotFound 	: public eLinkTableBase {};

class clinktable {
public:

	static clinktable&
	get_link_table(const rofl::cdptid& dptid) {
		if (clinktable::linktables.find(dptid) == clinktable::linktables.end()) {
			new clinktable(dptid);
		}
		return clinktable::linktables[dptid];
	};

	static bool
	has_link_table(const rofl::cdptid& dptid) const {
		return (not (clinktable::linktables.find(dptid) == clinktable::linktables.end()));
	};

public:

	/**
	 *
	 */
	clinktable(rofl::cdptid& dptid) : dptid(dptid) {
		if (clinktable::linktables.find(dptid) != clinktable::linktables.end()) {
			throw eLinkTableExists();
		}
		clinktable::linktables[dptid] = this;
	};

	/**
	 *
	 */
	~clinktable() {
		clinktable::linktables.erase(dptid);
	};

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
		dptid = table.dptid;
		clear();
		for (std::map<unsigned int, cdptlink>::const_iterator
				it = table.links.begin(); it != table.links.end(); ++it) {
			add_link(it->first) = it->second;
		}
		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { return links.clear(); };

	/**
	 *
	 */
	const rofl::cdptid&
	get_dptid() const { return dptid; };

	/**
	 *
	 */
	void
	clear() { links.clear(); };

	/**
	 *
	 */
	cdptlink&
	add_link(int index) {
		if (links.find(index) != links.end()) {
			links.erase(index);
		}
		return links[index];
	};

	/**
	 *
	 */
	cdptlink&
	set_link(int index) {
		if (links.find(index) == links.end()) {
			(void)links[index];
		}
		return links[index];
	};

	/**
	 *
	 */
	const cdptlink&
	get_link(int index) const {
		if (links.find(index) == links.end()) {
			throw eLinkTableNotFound();
		}
		return links.at(index);
	};

	/**
	 *
	 */
	void
	drop_link(int index) {
		if (links.find(index) == links.end()) {
			return;
		}
		links.erase(index);
	};

	/**
	 *
	 */
	bool
	has_link(int index) const {
		return (not (links.find(index) == links.end()));
	};

	/**
	 *
	 */
	const cdptlink&
	get_link_by_ofp_port_no(uint32_t ofp_port_no) const {
		std::map<unsigned int, cdptlink>::iterator it;
		if ((it = find_if(links.begin(), links.end(), cdptlink::cdptlink_by_ofp_port_no(ofp_port_no))) == links.end()) {
			throw eLinkTableNotFound();
		}
		return it->second;
	};

	/**
	 *
	 */
	const cdptlink&
	has_link_by_ofp_port_no(uint32_t ofp_port_no) const {
		std::map<unsigned int, cdptlink>::iterator it;
		if ((it = find_if(links.begin(), links.end(), cdptlink::cdptlink_by_ofp_port_no(ofp_port_no))) == links.end()) {
			return false;
		}
		return true;
	};

	/**
	 *
	 */
	const cdptlink&
	get_link_by_ifindex(int ifindex) const {
		std::map<unsigned int, cdptlink>::iterator it;
		if ((it = find_if(links.begin(), links.end(), cdptlink::cdptlink_by_ifindex(ifindex))) == links.end()) {
			throw eLinkTableNotFound();
		}
		return it->second;
	};

	/**
	 *
	 */
	const cdptlink&
	has_link_by_ifindex(int ifindex) const {
		std::map<unsigned int, cdptlink>::iterator it;
		if ((it = find_if(links.begin(), links.end(), cdptlink::cdptlink_by_ifindex(ifindex))) == links.end()) {
			return false;
		}
		return true;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const clinktable& table) {
		os << rofcore::indent(0) << "<clinktable dptid:" << table.dptid << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, cdptlink>::const_iterator
				it = table.links.begin(); it != table.links.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	static std::map<rofl::cdptid, clinktable*>		linktables;
	rofl::cdptid 									dptid;
	std::map<unsigned int, cdptlink> 				links;	// key: internal link index, neither ofp_port_no, nor ifindex assigned from kernel
};

}; // end of namespace ipcore

#endif /* CLNKTABLE_H_ */
