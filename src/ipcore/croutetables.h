/*
 * croutetables.h
 *
 *  Created on: 01.07.2014
 *      Author: andreas
 */

#ifndef CROUTETABLES_H_
#define CROUTETABLES_H_

#include <map>
#include <iostream>

#include "croutetable.h"
#include "logging.h"

namespace ipcore {

class eRtTablesBase : public std::exception {};
class eRtTablesExists : public eRtTableBase {};
class eRtTablesNotFound : public eRtTablesBase {};

class croutetables {
public:

	/**
	 *
	 */
	croutetables() {};

	/**
	 *
	 */
	~croutetables() {};

	/**
	 *
	 */
	croutetables(
			const croutetables& rttables) { *this = rttables; };

	/**
	 *
	 */
	croutetables&
	operator= (
			const croutetables& rttables) {
		if (this == &rttables)
			return *this;
		rtables.clear();
		for (std::map<unsigned int, croutetable>::const_iterator
				it = rttables.rtables.begin(); it != rttables.rtables.end(); ++it) {
			set_table(it->first) = rttables.get_table(it->first);
		}
		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtables.clear(); };

	/**
	 *
	 */
	croutetable&
	add_table(unsigned int table_id) {
		if (rtables.find(table_id) != rtables.end()) {
			rtables.erase(table_id);
		}
		return rtables[table_id];
	};

	/**
	 *
	 */
	croutetable&
	set_table(unsigned int table_id) {
		if (rtables.find(table_id) == rtables.end()) {
			(void)rtables[table_id];
		}
		return rtables[table_id];
	};

	/**
	 *
	 */
	const croutetable&
	get_table(unsigned int table_id) const {
		if (rtables.find(table_id) == rtables.end()) {
			throw eRtTablesNotFound();
		}
		return rtables.at(table_id);
	};

	/**
	 *
	 */
	void
	drop_table(unsigned int table_id) {
		if (rtables.find(table_id) == rtables.end()) {
			return;
		}
		rtables.erase(table_id);
	};

	/**
	 *
	 */
	bool
	has_table(unsigned int table_id) const {
		return (not (rtables.find(table_id) == rtables.end()));
	};


public:

	friend std::ostream&
	operator<< (std::ostream& os, const croutetables& rttables) {
		os << rofcore::indent(0) << "<croutetables >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, croutetable>::const_iterator
				it = rttables.rtables.begin(); it != rttables.rtables.end(); ++it) {
			os << rofcore::indent(0) << "<table-id: " << (unsigned int)it->first << " >" << std::endl;
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, croutetable>	rtables;
};

}; // end of namespace ipcore

#endif /* CROUTETABLES_H_ */
