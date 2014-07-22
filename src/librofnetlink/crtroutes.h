/*
 * crtroutes.h
 *
 *  Created on: 22.07.2014
 *      Author: andreas
 */

#ifndef CRTROUTES_H_
#define CRTROUTES_H_

#include <iostream>
#include <map>

#include "crtroute.h"
#include "logging.h"

namespace rofcore {

class crtroutes_in4 {
public:

	/**
	 *
	 */
	crtroutes_in4();

	/**
	 *
	 */
	virtual
	~crtroutes_in4();

	/**
	 *
	 */
	crtroutes_in4(const crtroutes_in4& rtroutes) { *this = rtroutes; };

	/**
	 *
	 */
	crtroutes_in4&
	operator= (const crtroutes_in4& rtroutes) {
		if (this == &rtroutes)
			return *this;

		clear();
		for (std::map<unsigned int, crtroute_in4>::const_iterator
				it = rtroutes.rtroutes.begin(); it != rtroutes.rtroutes.end(); ++it) {
			add_route(it->first) = it->second;
		}

		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtroutes.clear(); };

	/**
	 *
	 */
	crtroute_in4&
	add_route(unsigned int nbindex) {
		if (rtroutes.find(nbindex) != rtroutes.end()) {
			rtroutes.erase(nbindex);
		}
		return rtroutes[nbindex];
	};

	/**
	 *
	 */
	crtroute_in4&
	set_route(unsigned int nbindex) {
		if (rtroutes.find(nbindex) == rtroutes.end()) {
			rtroutes[nbindex] = crtroute(nbindex);
		}
		return rtroutes[nbindex];
	};

	/**
	 *
	 */
	const crtroute_in4&
	get_route(unsigned int nbindex) const {
		if (rtroutes.find(nbindex) == rtroutes.end()) {
			throw eRtLinkNotFound();
		}
		return rtroutes.at(nbindex);
	};

	/**
	 *
	 */
	void
	drop_route(unsigned int nbindex) {
		if (rtroutes.find(nbindex) == rtroutes.end()) {
			return;
		}
		rtroutes.erase(nbindex);
	};

	/**
	 *
	 */
	bool
	has_route(unsigned int nbindex) const {
		return (not (rtroutes.find(nbindex) == rtroutes.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtroutes_in4& rtroutes) {
		os << logging::indent(0) << "<crtroutes_in4 #rtroutes: " << rtroutes.rtroutes.size() << " >" << std::endl;
		logging::indent i(2);
		for (std::map<unsigned int, crtroute>::const_iterator
				it = rtroutes.rtroutes.begin(); it != rtroutes.rtroutes.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, crtroute_in4> rtroutes;
};




class crtroutes_in6 {
public:

	/**
	 *
	 */
	crtroutes_in6();

	/**
	 *
	 */
	virtual
	~crtroutes_in6();

	/**
	 *
	 */
	crtroutes_in6(const crtroutes_in6& rtroutes) { *this = rtroutes; };

	/**
	 *
	 */
	crtroutes_in6&
	operator= (const crtroutes_in6& rtroutes) {
		if (this == &rtroutes)
			return *this;

		clear();
		for (std::map<unsigned int, crtroute_in6>::const_iterator
				it = rtroutes.rtroutes.begin(); it != rtroutes.rtroutes.end(); ++it) {
			add_route(it->first) = it->second;
		}

		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtroutes.clear(); };

	/**
	 *
	 */
	crtroute_in6&
	add_route(unsigned int nbindex) {
		if (rtroutes.find(nbindex) != rtroutes.end()) {
			rtroutes.erase(nbindex);
		}
		return rtroutes[nbindex];
	};

	/**
	 *
	 */
	crtroute_in6&
	set_route(unsigned int nbindex) {
		if (rtroutes.find(nbindex) == rtroutes.end()) {
			rtroutes[nbindex] = crtroute(nbindex);
		}
		return rtroutes[nbindex];
	};

	/**
	 *
	 */
	const crtroute_in6&
	get_route(unsigned int nbindex) const {
		if (rtroutes.find(nbindex) == rtroutes.end()) {
			throw eRtLinkNotFound();
		}
		return rtroutes.at(nbindex);
	};

	/**
	 *
	 */
	void
	drop_route(unsigned int nbindex) {
		if (rtroutes.find(nbindex) == rtroutes.end()) {
			return;
		}
		rtroutes.erase(nbindex);
	};

	/**
	 *
	 */
	bool
	has_route(unsigned int nbindex) const {
		return (not (rtroutes.find(nbindex) == rtroutes.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtroutes_in6& rtroutes) {
		os << logging::indent(0) << "<crtroutes_in6 #rtroutes: " << rtroutes.rtroutes.size() << " >" << std::endl;
		logging::indent i(2);
		for (std::map<unsigned int, crtroute>::const_iterator
				it = rtroutes.rtroutes.begin(); it != rtroutes.rtroutes.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, crtroute_in6> rtroutes;
};

#endif /* CRTROUTES_H_ */
