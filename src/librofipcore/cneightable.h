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
#include "clogging.h"

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
	add_neigh_in4(unsigned int nbindex) {
		if (neighs_in4.find(nbindex) != neighs_in4.end()) {
			neighs_in4.erase(nbindex);
		}
		neighs_in4[nbindex].set_dptid(dptid);
		return neighs_in4[nbindex];
	};

	/**
	 *
	 */
	cdptneigh_in4&
	set_neigh_in4(unsigned int nbindex) {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			neighs_in4[nbindex].set_dptid(dptid);
		}
		return neighs_in4[nbindex];
	};

	/**
	 *
	 */
	const cdptneigh_in4&
	get_neigh_in4(unsigned int nbindex) const {
		if (neighs_in4.find(nbindex) == neighs_in4.end()) {
			throw eNeighTableNotFound();
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
	const cdptneigh_in4&
	get_neigh_in4(const rofl::caddress_in4& dst) const {
		std::map<unsigned int, cdptneigh_in4>::const_iterator it;
		if ((it = find_if(neighs_in4.begin(), neighs_in4.end(),
				cdptneigh_in4_find_by_dst(dst))) == neighs_in4.end()) {
			throw eNeighTableNotFound();
		}
		return it->second;
	};

	/**
	 *
	 */
	cdptneigh_in6&
	add_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) != neighs_in6.end()) {
			neighs_in6.erase(nbindex);
		}
		neighs_in6[nbindex].set_dptid(dptid);
		return neighs_in6[nbindex];
	};

	/**
	 *
	 */
	cdptneigh_in6&
	set_neigh_in6(unsigned int nbindex) {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			neighs_in6[nbindex].set_dptid(dptid);
		}
		return neighs_in6[nbindex];
	};

	/**
	 *
	 */
	const cdptneigh_in6&
	get_neigh_in6(unsigned int nbindex) const {
		if (neighs_in6.find(nbindex) == neighs_in6.end()) {
			throw eNeighTableNotFound();
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
	const cdptneigh_in6&
	get_neigh_in6(const rofl::caddress_in6& dst) const {
		std::map<unsigned int, cdptneigh_in6>::const_iterator it;
		if ((it = find_if(neighs_in6.begin(), neighs_in6.end(),
				cdptneigh_in6_find_by_dst(dst))) == neighs_in6.end()) {
			throw eNeighTableNotFound();
		}
		return it->second;
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
