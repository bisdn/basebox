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

#include <roflibs/netlink/crtlink.hpp>
#include <roflibs/netlink/clogging.hpp>

namespace rofcore {

class crtlinks {
public:

	/**
	 *
	 */
	crtlinks() {};

	/**
	 *
	 */
	virtual
	~crtlinks() {};

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

	size_t
	size() const { return rtlinks.size(); }

	/**
	 *
	 */
	crtlink&
	add_link(const crtlink& rtlink) {
		return (rtlinks[rtlink.get_ifindex()] = rtlink);
	};

	/**
	 *
	 */
	crtlink&
	set_link(const crtlink& rtlink) {
		return (rtlinks[rtlink.get_ifindex()] = rtlink);
	};





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
			rtlinks[ifindex];
		}
		return rtlinks[ifindex];
	};

	/**
	 *
	 */
	const crtlink&
	get_link(unsigned int ifindex) const {
		if (rtlinks.find(ifindex) == rtlinks.end()) {
			throw crtlink::eRtLinkNotFound("crtlinks::get_link() / error: ifindex not found");
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




	/**
	 *
	 */
	crtlink&
	set_link(const std::string& devname) {
		std::map<unsigned int, crtlink>::const_iterator it;
		if ((it = find_if(rtlinks.begin(), rtlinks.end(),
				crtlink::crtlink_find_by_devname(devname))) == rtlinks.end()) {
			throw crtlink::eRtLinkNotFound("crtlinks::set_link() / error: devname not found");
		}
		return rtlinks[it->first];
	};

	/**
	 *
	 */
	const crtlink&
	get_link(const std::string& devname) const {
		std::map<unsigned int, crtlink>::const_iterator it;
		if ((it = find_if(rtlinks.begin(), rtlinks.end(),
				crtlink::crtlink_find_by_devname(devname))) == rtlinks.end()) {
			throw crtlink::eRtLinkNotFound("crtlinks::get_link() / error: devname not found");
		}
		return rtlinks.at(it->first);
	};

	/**
	 *
	 */
	void
	drop_link(const std::string& devname) {
		std::map<unsigned int, crtlink>::const_iterator it;
		if ((it = find_if(rtlinks.begin(), rtlinks.end(),
				crtlink::crtlink_find_by_devname(devname))) == rtlinks.end()) {
			return;
		}
		rtlinks.erase(it->first);
	};

	/**
	 *
	 */
	bool
	has_link(const std::string& devname) const {
		std::map<unsigned int, crtlink>::const_iterator it;
		if ((it = find_if(rtlinks.begin(), rtlinks.end(),
				crtlink::crtlink_find_by_devname(devname))) == rtlinks.end()) {
			return false;
		}
		return true;
	};



public:

	friend std::ostream&
	operator<< (std::ostream& os, const crtlinks& rtlinks) {
		os << rofcore::indent(0) << "<crtlinks #rtlinks: " << rtlinks.rtlinks.size() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, crtlink>::const_iterator
				it = rtlinks.rtlinks.begin(); it != rtlinks.rtlinks.end(); ++it) {
			os << it->second;
		}
		return os;
	};

	std::string
	str() const {
		std::stringstream ss;
		for (std::map<unsigned int, crtlink>::const_iterator
				it = rtlinks.begin(); it != rtlinks.end(); ++it) {
			ss << it->second.str() << std::endl;
		}
		return ss.str();
	};

	const std::map<unsigned int, crtlink>&
	get_all_links() {
		return rtlinks;
	}
private:

	std::map<unsigned int, crtlink> rtlinks;

};

}; // end of namespace



#endif /* CRTLINKS_H_ */
