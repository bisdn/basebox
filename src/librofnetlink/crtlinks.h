/*
 * crtlinks.h
 *
 *  Created on: 22.07.2014
 *      Author: andreas
 */

#ifndef CRTLINKS_H_
#define CRTLINKS_H_

#include <iostream>
#include <map>

#include "crtlink.h"
#include "logging.h"

namespace rofcore {

class crtlinks {
public:

	/**
	 *
	 */
	crtlinks();

	/**
	 *
	 */
	virtual
	~crtlinks();

	/**
	 *
	 */
	crtlinks(const crtlinks& rtlinks) { *this = rtlinks; };

	/**
	 *
	 */
	crtlinks&
	operator= (const crtlinks& rtlinks) {
		if (this == &rtlinks)
			return *this;

		clear();
		for (std::map<unsigned int, crtlink>::const_iterator
				it = rtlinks.rtlinks.begin(); it != rtlinks.rtlinks.end(); ++it) {
			add_link(it->first) = it->second;
		}

		return *this;
	};

public:

	/**
	 *
	 */
	void
	clear() { rtlinks.clear(); };

	/**
	 *
	 */
	crtlink&
	add_link(unsigned int ifindex) {
		if (rtlinks.find(ifindex) != rtlinks.end()) {
			rtlinks.erase(ifindex);
		}
		return rtlinks[ifindex];
	};

	/**
	 *
	 */
	crtlink&
	set_link(unsigned int ifindex) {
		if (rtlinks.find(ifindex) == rtlinks.end()) {
			rtlinks[ifindex] = crtlink(ifindex);
		}
		return rtlinks[ifindex];
	};

	/**
	 *
	 */
	const crtlink&
	get_link(unsigned int ifindex) const {
		if (rtlinks.find(ifindex) == rtlinks.end()) {
			throw eRtLinkNotFound();
		}
		return rtlinks.at(ifindex);
	};

	/**
	 *
	 */
	void
	drop_link(unsigned int ifindex) {
		if (rtlinks.find(ifindex) == rtlinks.end()) {
			return;
		}
		rtlinks.erase(ifindex);
	};

	/**
	 *
	 */
	bool
	has_link(unsigned int ifindex) const {
		return (not (rtlinks.find(ifindex) == rtlinks.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtlinks& rtlinks) {
		os << logging::indent(0) << "<crtlinks #rtlinks: " << rtlinks.rtlinks.size() << " >" << std::endl;
		logging::indent i(2);
		for (std::map<unsigned int, crtlink>::const_iterator
				it = rtlinks.rtlinks.begin(); it != rtlinks.rtlinks.end(); ++it) {
			os << it->second;
		}
		return os;
	};

private:

	std::map<unsigned int, crtlink> rtlinks;

};

}; // end of namespace



#endif /* CRTLINKS_H_ */
