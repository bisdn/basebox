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
#include "logging.h"

namespace ipcore {

class eLinkTableBase		: public std::exception {};
class eLinkTableExists		: public eLinkTableBase {};
class eLinkTableNotFound 	: public eLinkTableBase {};

class clinktable {
public:

	/**
	 *
	 */
	clinktable();

	/**
	 *
	 */
	~clinktable();

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
	clear() { links.clear(); };

	/**
	 *
	 */
	cdptlink&
	add_link(int index) {
		if (links.find(index) != links.end()) {
			links.erase(index);
		}
		links[index] = cdptlink(index);
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
		std::map<unsigned int, cdptlink>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(), cdptlink::cdptlink_by_ofp_port_no(ofp_port_no))) == links.end()) {
			throw eLinkTableNotFound();
		}
		return it->second;
	};

	/**
	 *
	 */
	bool
	has_link_by_ofp_port_no(uint32_t ofp_port_no) const {
		std::map<unsigned int, cdptlink>::const_iterator it;
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
		std::map<unsigned int, cdptlink>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(), cdptlink::cdptlink_by_ifindex(ifindex))) == links.end()) {
			throw eLinkTableNotFound();
		}
		return it->second;
	};

	/**
	 *
	 */
	bool
	has_link_by_ifindex(int ifindex) const {
		std::map<unsigned int, cdptlink>::const_iterator it;
		if ((it = find_if(links.begin(), links.end(), cdptlink::cdptlink_by_ifindex(ifindex))) == links.end()) {
			return false;
		}
		return true;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const clinktable& table) {
		os << rofcore::indent(0) << "<clinktable >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, cdptlink>::const_iterator
				it = table.links.begin(); it != table.links.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, cdptlink> links;	// key: internal link index, neither ofp_port_no, nor ifindex assigned from kernel
};

}; // end of namespace ipcore

#endif /* CLNKTABLE_H_ */
