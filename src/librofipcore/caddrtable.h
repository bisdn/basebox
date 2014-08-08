/*
 * caddrtable.h
 *
 *  Created on: 24.07.2014
 *      Author: andreas
 */

#ifndef CADDRTABLE_H_
#define CADDRTABLE_H_

#include <map>
#include <iostream>
#include <algorithm>

#include "cdptaddr.h"
#include "clogging.h"

namespace ipcore {

class eAddrTableBase		: public std::exception {};
class eAddrTableExists		: public eAddrTableBase {};
class eAddrTableNotFound 	: public eAddrTableBase {};

class caddrtable {
public:

	/**
	 *
	 */
	caddrtable() {};

	/**
	 *
	 */
	~caddrtable() {};

	/**
	 *
	 */
	caddrtable(
			const caddrtable& table) { *this = table; };

	/**
	 *
	 */
	caddrtable&
	operator= (
			const caddrtable& table) {
		if (this == &table)
			return *this;
		clear();
		for (std::map<uint32_t, cdptaddr_in4>::const_iterator
				it = table.addrs_in4.begin(); it != table.addrs_in4.end(); ++it) {
			add_addr_in4(it->first) = it->second;
		}
		for (std::map<uint32_t, cdptaddr_in6>::const_iterator
				it = table.addrs_in6.begin(); it != table.addrs_in6.end(); ++it) {
			add_addr_in6(it->first) = it->second;
		}
		return *this;
	};

	/**
	 *
	 */
	void
	install() {
		for (std::map<unsigned int, cdptaddr_in4>::iterator
				it = addrs_in4.begin(); it != addrs_in4.end(); ++it) {
			it->second.install();
		}
		for (std::map<unsigned int, cdptaddr_in6>::iterator
				it = addrs_in6.begin(); it != addrs_in6.end(); ++it) {
			it->second.install();
		}
	};

	/**
	 *
	 */
	void
	uninstall() {
		for (std::map<unsigned int, cdptaddr_in4>::iterator
				it = addrs_in4.begin(); it != addrs_in4.end(); ++it) {
			it->second.uninstall();
		}
		for (std::map<unsigned int, cdptaddr_in6>::iterator
				it = addrs_in6.begin(); it != addrs_in6.end(); ++it) {
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
		for (std::map<unsigned int, cdptaddr_in4>::iterator
				it = addrs_in4.begin(); it != addrs_in4.end(); ++it) {
			it->second.set_dptid(dptid);
		}
		for (std::map<unsigned int, cdptaddr_in6>::iterator
				it = addrs_in6.begin(); it != addrs_in6.end(); ++it) {
			it->second.set_dptid(dptid);
		}
	};

public:

	/**
	 *
	 */
	void
	clear() { addrs_in4.clear(); addrs_in6.clear(); };

	/**
	 *
	 */
	cdptaddr_in4&
	add_addr_in4(unsigned int adindex) {
		if (addrs_in4.find(adindex) != addrs_in4.end()) {
			addrs_in4.erase(adindex);
		}
		addrs_in4[adindex].set_dptid(dptid);
		return addrs_in4[adindex];
	};

	/**
	 *
	 */
	cdptaddr_in4&
	set_addr_in4(unsigned int adindex) {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			addrs_in4[adindex].set_dptid(dptid);
		}
		return addrs_in4[adindex];
	};

	/**
	 *
	 */
	const cdptaddr_in4&
	get_addr_in4(unsigned int adindex) const {
		if (addrs_in4.find(adindex) == addrs_in4.end()) {
			throw eAddrTableNotFound();
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
	cdptaddr_in6&
	add_addr_in6(unsigned int adindex) {
		if (addrs_in6.find(adindex) != addrs_in6.end()) {
			addrs_in6.erase(adindex);
		}
		addrs_in6[adindex].set_dptid(dptid);
		return addrs_in6[adindex];
	};

	/**
	 *
	 */
	cdptaddr_in6&
	set_addr_in6(unsigned int adindex) {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			addrs_in6[adindex].set_dptid(dptid);
		}
		return addrs_in6[adindex];
	};

	/**
	 *
	 */
	const cdptaddr_in6&
	get_addr_in6(unsigned int adindex) const {
		if (addrs_in6.find(adindex) == addrs_in6.end()) {
			throw eAddrTableNotFound();
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

	friend std::ostream&
	operator<< (std::ostream& os, const caddrtable& table) {
		os << rofcore::indent(0) << "<caddrtable >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, cdptaddr_in4>::const_iterator
				it = table.addrs_in4.begin(); it != table.addrs_in4.end(); ++it) {
			os << it->second;
		}
		for (std::map<unsigned int, cdptaddr_in6>::const_iterator
				it = table.addrs_in6.begin(); it != table.addrs_in6.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	rofl::cdptid							dptid;
	std::map<unsigned int, cdptaddr_in4> 	addrs_in4;	// key: addr ofp port-no
	std::map<unsigned int, cdptaddr_in6> 	addrs_in6;	// key: addr ofp port-no
};

}; // end of namespace ipcore


#endif /* CADDRTABLE_H_ */
