/*
 * crtaddrs.h
 *
 *  Created on: 22.07.2014
 *      Author: andreas
 */

#ifndef CRTADDRS_H_
#define CRTADDRS_H_


#include <iostream>
#include <map>

#include "crtaddr.h"
#include "logging.h"

namespace rofcore {

class crtaddrs_in4 {
public:

	/**
	 *
	 */
	crtaddrs_in4();

	/**
	 *
	 */
	virtual
	~crtaddrs_in4();

	/**
	 *
	 */
	crtaddrs_in4(const crtaddrs_in4& rtaddrs) { *this = rtaddrs; };

	/**
	 *
	 */
	crtaddrs_in4&
	operator= (const crtaddrs_in4& rtaddrs) {
		if (this == &rtaddrs)
			return *this;

		clear();
		for (std::map<unsigned int, crtaddr_in4>::const_iterator
				it = rtaddrs.rtaddrs.begin(); it != rtaddrs.rtaddrs.end(); ++it) {
			add_addr(it->first) = it->second;
		}

		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtaddrs.clear(); };

	/**
	 *
	 */
	crtaddr_in4&
	add_addr(unsigned int adindex) {
		if (rtaddrs.find(adindex) != rtaddrs.end()) {
			rtaddrs.erase(adindex);
		}
		return rtaddrs[adindex];
	};

	/**
	 *
	 */
	crtaddr_in4&
	set_addr(unsigned int adindex) {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			rtaddrs[adindex] = crtaddr(adindex);
		}
		return rtaddrs[adindex];
	};

	/**
	 *
	 */
	const crtaddr_in4&
	get_addr(unsigned int adindex) const {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			throw eRtLinkNotFound();
		}
		return rtaddrs.at(adindex);
	};

	/**
	 *
	 */
	void
	drop_addr(unsigned int adindex) {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			return;
		}
		rtaddrs.erase(adindex);
	};

	/**
	 *
	 */
	bool
	has_addr(unsigned int adindex) const {
		return (not (rtaddrs.find(adindex) == rtaddrs.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtaddrs_in4& rtaddrs) {
		os << logging::indent(0) << "<crtaddrs_in4 #rtaddrs: " << rtaddrs.rtaddrs.size() << " >" << std::endl;
		logging::indent i(2);
		for (std::map<unsigned int, crtaddr>::const_iterator
				it = rtaddrs.rtaddrs.begin(); it != rtaddrs.rtaddrs.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, crtaddr_in4> rtaddrs;
};

class crtaddrs_in6 {
public:

	/**
	 *
	 */
	crtaddrs_in6();

	/**
	 *
	 */
	virtual
	~crtaddrs_in6();

	/**
	 *
	 */
	crtaddrs_in6(const crtaddrs_in6& rtaddrs) { *this = rtaddrs; };

	/**
	 *
	 */
	crtaddrs_in6&
	operator= (const crtaddrs_in6& rtaddrs) {
		if (this == &rtaddrs)
			return *this;

		clear();
		for (std::map<unsigned int, crtaddr_in6>::const_iterator
				it = rtaddrs.rtaddrs.begin(); it != rtaddrs.rtaddrs.end(); ++it) {
			add_addr(it->first) = it->second;
		}

		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtaddrs.clear(); };

	/**
	 *
	 */
	crtaddr_in6&
	add_addr(unsigned int adindex) {
		if (rtaddrs.find(adindex) != rtaddrs.end()) {
			rtaddrs.erase(adindex);
		}
		return rtaddrs[adindex];
	};

	/**
	 *
	 */
	crtaddr_in6&
	set_addr(unsigned int adindex) {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			rtaddrs[adindex] = crtaddr(adindex);
		}
		return rtaddrs[adindex];
	};

	/**
	 *
	 */
	const crtaddr_in6&
	get_addr(unsigned int adindex) const {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			throw eRtLinkNotFound();
		}
		return rtaddrs.at(adindex);
	};

	/**
	 *
	 */
	void
	drop_addr(unsigned int adindex) {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			return;
		}
		rtaddrs.erase(adindex);
	};

	/**
	 *
	 */
	bool
	has_addr(unsigned int adindex) const {
		return (not (rtaddrs.find(adindex) == rtaddrs.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtaddrs_in6& rtaddrs) {
		os << logging::indent(0) << "<crtaddrs_in6 #rtaddrs: " << rtaddrs.rtaddrs.size() << " >" << std::endl;
		logging::indent i(2);
		for (std::map<unsigned int, crtaddr>::const_iterator
				it = rtaddrs.rtaddrs.begin(); it != rtaddrs.rtaddrs.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, crtaddr_in6> rtaddrs;
};


}; // end of namespace



#endif /* CRTADDRS_H_ */
