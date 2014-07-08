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

	static croutetables&
	get_route_tables(const rofl::cdptid& dptid) {
		if (croutetables::routetables.find(dptid) == croutetables::routetables.end()) {
			new croutetables(dptid);
		}
		return croutetables::routetables[dptid];
	};

	static bool
	has_route_tables(const rofl::cdptid& dptid) const {
		return (not (croutetables::routetables.find(dptid) == croutetables::routetables.end()));
	};

public:

	/**
	 *
	 */
	croutetables(const rofl::cdptid& dptid) {
		if (croutetables::routetables.find(dptid) != croutetables::routetables.end()) {
			throw eRtTablesExists();
		}
		croutetables::routetables[dptid] = this;
	};

	/**
	 *
	 */
	~croutetables() {
		croutetables::routetables.erase(dptid);
	};

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
		tables.clear();
		for (std::map<unsigned int, croutetable>::const_iterator
				it = rttables.tables.begin(); it != rttables.tables.end(); ++it) {
			set_table(it->first) = rttables.get_table(it->first);
		}
		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { tables.clear(); };

	/**
	 *
	 */
	croutetable&
	add_table(unsigned int table_id) {
		if (tables.find(table_id) != tables.end()) {
			tables.erase(table_id);
		}
		return tables[table_id];
	};

	/**
	 *
	 */
	croutetable&
	set_table(unsigned int table_id) {
		if (tables.find(table_id) == tables.end()) {
			(void)tables[table_id];
		}
		return tables[table_id];
	};

	/**
	 *
	 */
	const croutetable&
	get_table(unsigned int table_id) const {
		if (tables.find(table_id) == tables.end()) {
			throw eRtTablesNotFound();
		}
		return tables.at(table_id);
	};

	/**
	 *
	 */
	void
	drop_table(unsigned int table_id) {
		if (tables.find(table_id) == tables.end()) {
			return;
		}
		tables.erase(table_id);
	};

	/**
	 *
	 */
	bool
	has_table(unsigned int table_id) const {
		return (not (tables.find(table_id) == tables.end()));
	};


public:

	friend std::ostream&
	operator<< (std::ostream& os, const croutetables& rttables) {
		os << rofcore::indent(0) << "<croutetables dptid: " << rttables.dptid << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, croutetable>::const_iterator
				it = rttables.tables.begin(); it != rttables.tables.end(); ++it) {
			os << rofcore::indent(0) << "<table-id: " << (unsigned int)it->first << " >" << std::endl;
			os << it->second;
		}
		return os;
	};

private:

	std::map<rofl::cdptid, croutetables*> routetables;
	rofl::cdptid dptid;
	std::map<unsigned int, croutetable>	tables;

};

}; // end of namespace ipcore

#endif /* CROUTETABLES_H_ */
