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
	unsigned int
	add_nexthop(const crtnexthop_in4& rtnexthop) {
		std::map<unsigned int, crtnexthop_in4>::iterator it;
		if ((it = find_if(rtnexthops.begin(), rtnexthops.end(),
				crtnexthop_in4_find(rtnexthop))) != rtnexthops.end()) {
			rtnexthops.erase(it->first);
		}
		unsigned int adindex = 0;
		while (rtnexthops.find(adindex) != rtnexthops.end()) {
			adindex++;
		}
		rtnexthops[adindex] = rtnexthop;
		return adindex;
	};


	/**
	 *
	 */
	unsigned int
	set_nexthop(const crtnexthop_in4& rtnexthop) {
		std::map<unsigned int, crtnexthop_in4>::iterator it;
		if ((it = find_if(rtnexthops.begin(), rtnexthops.end(),
				crtnexthop_in4_find(rtnexthop))) == rtnexthops.end()) {
			return add_nexthop(rtnexthop);
		}
		rtnexthops[it->first] = rtnexthop;
		return it->first;
	};


	/**
	 *
	 */
	unsigned int
	get_nexthop(const crtnexthop_in4& rtnexthop) const {
		std::map<unsigned int, crtnexthop_in4>::const_iterator it;
		if ((it = find_if(rtnexthops.begin(), rtnexthops.end(),
				crtnexthop_in4_find(rtnexthop))) == rtnexthops.end()) {
			throw crtnexthop::eRtNextHopNotFound("crtnexthops_in4::get_nexthop() / error: rtnexthop not found");
		}
		return it->first;
	};

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
			rtnexthops[nbindex];
		}
		return rtnexthops[nbindex];
	};

	/**
	 *
	 */
	const crtnexthop_in4&
	get_nexthop(unsigned int nbindex) const {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			throw crtnexthop::eRtNextHopNotFound("crtnexthops::get_link() / error: nbindex not found");
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
		os << rofcore::indent(0) << "<crtnexthops_in4 #rtnexthops: " << rtnexthops.rtnexthops.size() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, crtnexthop_in4>::const_iterator
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
	unsigned int
	add_nexthop(const crtnexthop_in6& rtnexthop) {
		std::map<unsigned int, crtnexthop_in6>::iterator it;
		if ((it = find_if(rtnexthops.begin(), rtnexthops.end(),
				crtnexthop_in6_find(rtnexthop))) != rtnexthops.end()) {
			rtnexthops.erase(it->first);
		}
		unsigned int adindex = 0;
		while (rtnexthops.find(adindex) != rtnexthops.end()) {
			adindex++;
		}
		rtnexthops[adindex] = rtnexthop;
		return adindex;
	};


	/**
	 *
	 */
	unsigned int
	set_nexthop(const crtnexthop_in6& rtnexthop) {
		std::map<unsigned int, crtnexthop_in6>::iterator it;
		if ((it = find_if(rtnexthops.begin(), rtnexthops.end(),
				crtnexthop_in6_find(rtnexthop))) == rtnexthops.end()) {
			return add_nexthop(rtnexthop);
		}
		rtnexthops[it->first] = rtnexthop;
		return it->first;
	};


	/**
	 *
	 */
	unsigned int
	get_nexthop(const crtnexthop_in6& rtnexthop) const {
		std::map<unsigned int, crtnexthop_in6>::const_iterator it;
		if ((it = find_if(rtnexthops.begin(), rtnexthops.end(),
				crtnexthop_in6_find(rtnexthop))) == rtnexthops.end()) {
			throw crtnexthop::eRtNextHopNotFound("crtnexthops_in4::get_nexthop() / error: rtnexthop not found");
		}
		return it->first;
	};


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
			rtnexthops[nbindex];
		}
		return rtnexthops[nbindex];
	};

	/**
	 *
	 */
	const crtnexthop_in6&
	get_nexthop(unsigned int nbindex) const {
		if (rtnexthops.find(nbindex) == rtnexthops.end()) {
			throw crtnexthop::eRtNextHopNotFound("crtnexthops::get_nexthop() / error: nbindex not found");
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
		os << rofcore::indent(0) << "<crtnexthops_in6 #rtnexthops: " << rtnexthops.rtnexthops.size() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, crtnexthop_in6>::const_iterator
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
