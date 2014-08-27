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

#include "crtaddr.hpp"
#include "clogging.hpp"

namespace rofcore {

class crtaddrs_in4 {
public:

	/**
	 *
	 */
	crtaddrs_in4() {};

	/**
	 *
	 */
	virtual
	~crtaddrs_in4() {};

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
	unsigned int
	add_addr(const crtaddr_in4& rtaddr) {
		std::map<unsigned int, crtaddr_in4>::iterator it;
		if ((it = find_if(rtaddrs.begin(), rtaddrs.end(),
				crtaddr_in4_find(rtaddr))) != rtaddrs.end()) {
			rtaddrs.erase(it->first);
		}
		unsigned int adindex = 0;
		while (rtaddrs.find(adindex) != rtaddrs.end()) {
			adindex++;
		}
		rtaddrs[adindex] = rtaddr;
		return adindex;
	};


	/**
	 *
	 */
	unsigned int
	set_addr(const crtaddr_in4& rtaddr) {
		std::map<unsigned int, crtaddr_in4>::iterator it;
		if ((it = find_if(rtaddrs.begin(), rtaddrs.end(),
				crtaddr_in4_find(rtaddr))) == rtaddrs.end()) {
			return add_addr(rtaddr);
		}
		rtaddrs[it->first] = rtaddr;
		return it->first;
	};


	/**
	 *
	 */
	unsigned int
	get_addr(const crtaddr_in4& rtaddr) const {
		std::map<unsigned int, crtaddr_in4>::const_iterator it;
		if ((it = find_if(rtaddrs.begin(), rtaddrs.end(),
				crtaddr_in4_find(rtaddr))) == rtaddrs.end()) {
			throw crtaddr::eRtAddrNotFound("crtaddrs_in4::get_addr() / error: rtaddr not found");
		}
		return it->first;
	};




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
			rtaddrs[adindex];
		}
		return rtaddrs[adindex];
	};

	/**
	 *
	 */
	const crtaddr_in4&
	get_addr(unsigned int adindex) const {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			throw crtaddr::eRtAddrNotFound("crtaddrs_in4::get_addr() / error: adindex not found");
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
		os << rofcore::indent(0) << "<crtaddrs_in4 #rtaddrs: " << rtaddrs.rtaddrs.size() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, crtaddr_in4>::const_iterator
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
	crtaddrs_in6() {};

	/**
	 *
	 */
	virtual
	~crtaddrs_in6() {};

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
	unsigned int
	add_addr(const crtaddr_in6& rtaddr) {
		std::map<unsigned int, crtaddr_in6>::iterator it;
		if ((it = find_if(rtaddrs.begin(), rtaddrs.end(),
				crtaddr_in6_find(rtaddr))) != rtaddrs.end()) {
			rtaddrs.erase(it->first);
		}
		unsigned int adindex = 0;
		while (rtaddrs.find(adindex) != rtaddrs.end()) {
			adindex++;
		}
		rtaddrs[adindex] = rtaddr;
		return adindex;
	};


	/**
	 *
	 */
	unsigned int
	set_addr(const crtaddr_in6& rtaddr) {
		std::map<unsigned int, crtaddr_in6>::iterator it;
		if ((it = find_if(rtaddrs.begin(), rtaddrs.end(),
				crtaddr_in6_find(rtaddr))) == rtaddrs.end()) {
			return add_addr(rtaddr);
		}
		rtaddrs[it->first] = rtaddr;
		return it->first;
	};


	/**
	 *
	 */
	unsigned int
	get_addr(const crtaddr_in6& rtaddr) const {
		std::map<unsigned int, crtaddr_in6>::const_iterator it;
		if ((it = find_if(rtaddrs.begin(), rtaddrs.end(),
				crtaddr_in6_find(rtaddr))) == rtaddrs.end()) {
			throw crtaddr::eRtAddrNotFound("crtaddrs_in6::get_addr() / error: rtaddr not found");
		}
		return it->first;
	};






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
			rtaddrs[adindex];
		}
		return rtaddrs[adindex];
	};

	/**
	 *
	 */
	const crtaddr_in6&
	get_addr(unsigned int adindex) const {
		if (rtaddrs.find(adindex) == rtaddrs.end()) {
			throw crtaddr::eRtAddrNotFound("crtaddrs_in6::get_addr() / error: adindex not found");
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
		os << rofcore::indent(0) << "<crtaddrs_in6 #rtaddrs: " << rtaddrs.rtaddrs.size() << " >" << std::endl;
		rofcore::indent i(2);
		for (std::map<unsigned int, crtaddr_in6>::const_iterator
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
