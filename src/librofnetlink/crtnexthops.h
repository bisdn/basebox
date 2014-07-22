/*
 * crtnexthops.h
 *
 *  Created on: 22.07.2014
 *      Author: andreas
 */

#ifndef CRTNEXTHOPS_H_
#define CRTNEXTHOPS_H_

#include <iostream>
#include <map>

#include "crtnexthop.h"
#include "logging.h"

namespace rofcore {

class crtnexthops_in4 {
public:

	/**
	 *
	 */
	crtnexthops_in4();

	/**
	 *
	 */
	virtual
	~crtnexthops_in4();

	/**
	 *
	 */
	crtnexthops_in4(const crtnexthops_in4& rtnexthops) { *this = rtnexthops; };

	/**
	 *
	 */
	crtnexthops_in4&
	operator= (const crtnexthops_in4& rtnexthops) {
		if (this == &rtnexthops)
			return *this;

		clear();
		for (std::map<unsigned int, crtnexthop_in4>::const_iterator
				it = rtnexthops.rtnexthops.begin(); it != rtnexthops.rtnexthops.end(); ++it) {
			add_nexthop(it->first) = it->second;
		}

		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtnexthops.clear(); };

	/**
	 *
	 */
	crtnexthop_in4&
	add_nexthop(unsigned int nbindex) {
		if (rtnexthops.find(nbindex) != rtnexthops.end()) {
			rtnexthops.erase(nbindex);
		}
		return rtnexthops[nbindex];
	};

	/**
	 *
	 */
	crtnexthop_in4&
	set_nexthop(unsigned int nbindex) {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			rtnexthops[nbindex] = crtnexthop(nbindex);
		}
		return rtnexthops[nbindex];
	};

	/**
	 *
	 */
	const crtnexthop_in4&
	get_nexthop(unsigned int nbindex) const {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			throw eRtLinkNotFound();
		}
		return rtnexthops.at(nbindex);
	};

	/**
	 *
	 */
	void
	drop_nexthop(unsigned int nbindex) {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			return;
		}
		rtnexthops.erase(nbindex);
	};

	/**
	 *
	 */
	bool
	has_nexthop(unsigned int nbindex) const {
		return (not (rtnexthops.find(nbindex) == rtnexthops.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtnexthops_in4& rtnexthops) {
		os << logging::indent(0) << "<crtnexthops_in4 #rtnexthops: " << rtnexthops.rtnexthops.size() << " >" << std::endl;
		logging::indent i(2);
		for (std::map<unsigned int, crtnexthop>::const_iterator
				it = rtnexthops.rtnexthops.begin(); it != rtnexthops.rtnexthops.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, crtnexthop_in4> rtnexthops;
};




class crtnexthops_in6 {
public:

	/**
	 *
	 */
	crtnexthops_in6();

	/**
	 *
	 */
	virtual
	~crtnexthops_in6();

	/**
	 *
	 */
	crtnexthops_in6(const crtnexthops_in6& rtnexthops) { *this = rtnexthops; };

	/**
	 *
	 */
	crtnexthops_in6&
	operator= (const crtnexthops_in6& rtnexthops) {
		if (this == &rtnexthops)
			return *this;

		clear();
		for (std::map<unsigned int, crtnexthop_in6>::const_iterator
				it = rtnexthops.rtnexthops.begin(); it != rtnexthops.rtnexthops.end(); ++it) {
			add_nexthop(it->first) = it->second;
		}

		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtnexthops.clear(); };

	/**
	 *
	 */
	crtnexthop_in6&
	add_nexthop(unsigned int nbindex) {
		if (rtnexthops.find(nbindex) != rtnexthops.end()) {
			rtnexthops.erase(nbindex);
		}
		return rtnexthops[nbindex];
	};

	/**
	 *
	 */
	crtnexthop_in6&
	set_nexthop(unsigned int nbindex) {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			rtnexthops[nbindex] = crtnexthop(nbindex);
		}
		return rtnexthops[nbindex];
	};

	/**
	 *
	 */
	const crtnexthop_in6&
	get_nexthop(unsigned int nbindex) const {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			throw eRtLinkNotFound();
		}
		return rtnexthops.at(nbindex);
	};

	/**
	 *
	 */
	void
	drop_nexthop(unsigned int nbindex) {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			return;
		}
		rtnexthops.erase(nbindex);
	};

	/**
	 *
	 */
	bool
	has_nexthop(unsigned int nbindex) const {
		return (not (rtnexthops.find(nbindex) == rtnexthops.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtnexthops_in6& rtnexthops) {
		os << logging::indent(0) << "<crtnexthops_in6 #rtnexthops: " << rtnexthops.rtnexthops.size() << " >" << std::endl;
		logging::indent i(2);
		for (std::map<unsigned int, crtnexthop>::const_iterator
				it = rtnexthops.rtnexthops.begin(); it != rtnexthops.rtnexthops.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, crtnexthop_in6> rtnexthops;
};


}; // end of namespace


#endif /* CRTNEXTHOPS_H_ */
