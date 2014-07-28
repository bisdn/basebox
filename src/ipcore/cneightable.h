/*
 * cneightable.h
 *
 *  Created on: 24.07.2014
 *      Author: andreas
 */

#ifndef CNEIGHTABLE_H_
#define CNEIGHTABLE_H_


#include <map>
#include <iostream>
#include <algorithm>

#include "cdptneigh.h"
#include "logging.h"

namespace ipcore {

class eNeighTableBase		: public std::exception {};
class eNeighTableExists		: public eNeighTableBase {};
class eNeighTableNotFound 	: public eNeighTableBase {};

class cneightable {
public:

	/**
	 *
	 */
	cneightable() {};

	/**
	 *
	 */
	~cneightable() {};

	/**
	 *
	 */
	cneightable(
			const cneightable& table) { *this = table; };

	/**
	 *
	 */
	cneightable&
	operator= (
			const cneightable& table) {
		if (this == &table)
			return *this;
		clear();
		for (std::map<uint32_t, cdptneigh_in4>::const_iterator
				it = table.neighs_in4.begin(); it != table.neighs_in4.end(); ++it) {
			add_neigh_in4(it->first) = it->second;
		}
		for (std::map<uint32_t, cdptneigh_in6>::const_iterator
				it = table.neighs_in6.begin(); it != table.neighs_in6.end(); ++it) {
			add_neigh_in6(it->first) = it->second;
		}
		return *this;
	};

	/**
	 *
	 */
	void
	install() {
		for (std::map<unsigned int, cdptneigh_in4>::iterator
				it = neighs_in4.begin(); it != neighs_in4.end(); ++it) {
			it->second.install();
		}
		for (std::map<unsigned int, cdptneigh_in6>::iterator
				it = neighs_in6.begin(); it != neighs_in6.end(); ++it) {
			it->second.install();
		}
	};

	/**
	 *
	 */
	void
	uninstall() {
		for (std::map<unsigned int, cdptneigh_in4>::iterator
				it = neighs_in4.begin(); it != neighs_in4.end(); ++it) {
			it->second.uninstall();
		}
		for (std::map<unsigned int, cdptneigh_in6>::iterator
				it = neighs_in6.begin(); it != neighs_in6.end(); ++it) {
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
		for (std::map<unsigned int, cdptneigh_in4>::iterator
				it = neighs_in4.begin(); it != neighs_in4.end(); ++it) {
			it->second.set_dptid(dptid);
		}
		for (std::map<unsigned int, cdptneigh_in6>::iterator
				it = neighs_in6.begin(); it != neighs_in6.end(); ++it) {
			it->second.set_dptid(dptid);
		}
	};

public:

	/**
	 *
	 */
	void
	clear() { neighs_in4.clear(); neighs_in6.clear(); };

	/**
	 *
	 */
	cdptneigh_in4&
	add_neigh_in4(unsigned int adindex) {
		if (neighs_in4.find(adindex) != neighs_in4.end()) {
			neighs_in4.erase(adindex);
		}
		neighs_in4[adindex].set_dptid(dptid);
		return neighs_in4[adindex];
	};

	/**
	 *
	 */
	cdptneigh_in4&
	set_neigh_in4(unsigned int adindex) {
		if (neighs_in4.find(adindex) == neighs_in4.end()) {
			neighs_in4[adindex].set_dptid(dptid);
		}
		return neighs_in4[adindex];
	};

	/**
	 *
	 */
	const cdptneigh_in4&
	get_neigh_in4(unsigned int adindex) const {
		if (neighs_in4.find(adindex) == neighs_in4.end()) {
			throw eNeighTableNotFound();
		}
		return neighs_in4.at(adindex);
	};

	/**
	 *
	 */
	void
	drop_neigh_in4(unsigned int adindex) {
		if (neighs_in4.find(adindex) == neighs_in4.end()) {
			return;
		}
		neighs_in4.erase(adindex);
	};

	/**
	 *
	 */
	bool
	has_neigh_in4(unsigned int adindex) const {
		return (not (neighs_in4.find(adindex) == neighs_in4.end()));
	};


	/**
	 *
	 */
	cdptneigh_in6&
	add_neigh_in6(unsigned int adindex) {
		if (neighs_in6.find(adindex) != neighs_in6.end()) {
			neighs_in6.erase(adindex);
		}
		neighs_in6[adindex].set_dptid(dptid);
		return neighs_in6[adindex];
	};

	/**
	 *
	 */
	cdptneigh_in6&
	set_neigh_in6(unsigned int adindex) {
		if (neighs_in6.find(adindex) == neighs_in6.end()) {
			neighs_in6[adindex].set_dptid(dptid);
		}
		return neighs_in6[adindex];
	};

	/**
	 *
	 */
	const cdptneigh_in6&
	get_neigh_in6(unsigned int adindex) const {
		if (neighs_in6.find(adindex) == neighs_in6.end()) {
			throw eNeighTableNotFound();
		}
		return neighs_in6.at(adindex);
	};

	/**
	 *
	 */
	void
	drop_neigh_in6(unsigned int adindex) {
		if (neighs_in6.find(adindex) == neighs_in6.end()) {
			return;
		}
		neighs_in6.erase(adindex);
	};

	/**
	 *
	 */
	bool
	has_neigh_in6(unsigned int adindex) const {
		return (not (neighs_in6.find(adindex) == neighs_in6.end()));
	};


public:

	friend std::ostream&
	operator<< (std::ostream& os, const cneightable& table) {
		os << rofcore::indent(0) << "<cneightable >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, cdptneigh_in4>::const_iterator
				it = table.neighs_in4.begin(); it != table.neighs_in4.end(); ++it) {
			os << it->second;
		}
		for (std::map<unsigned int, cdptneigh_in6>::const_iterator
				it = table.neighs_in6.begin(); it != table.neighs_in6.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	rofl::cdptid							dptid;
	std::map<unsigned int, cdptneigh_in4> 	neighs_in4;	// key: neigh ofp port-no
	std::map<unsigned int, cdptneigh_in6> 	neighs_in6;	// key: neigh ofp port-no
};

}; // end of namespace ipcore

#endif /* CNEIGHTABLE_H_ */
