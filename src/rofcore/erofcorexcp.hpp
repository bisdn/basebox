/*
 * erofcorexcp.hpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#ifndef EROFCOREXCP_HPP_
#define EROFCOREXCP_HPP_

#include <string>
#include <iostream>
#include <exception>

#include "croflog.hpp"

namespace rofcore {

class eRofCoreXcp 	: public std::exception {
	std::string s_error;
public:
	/**
	 *
	 */
	eRofCoreXcp(
			std::string const& s_error = std::string("")) :
				s_error(s_error) {
		// intentionally left empty
	};
	/**
	 *
	 */
	virtual
	~eRofCoreXcp() throw() {
		// intentionally left empty
	};

	/**
	 *
	 */
	eRofCoreXcp(
			eRofCoreXcp const& e) {
		*this = e;
	};

	/**
	 *
	 */
	eRofCoreXcp&
	operator= (
			eRofCoreXcp const& e) {
		if (this == &e)
			return *this;
		std::exception::operator= (e);
		s_error = e.s_error;
		return *this;
	};
public:
	friend std::ostream&
	operator<< (std::ostream& os, eRofCoreXcp const& e) {
		os << rofcore::indent(0) << "<rofcore exception: " << e.s_error << " >" << std::endl;
		return os;
	};
};

}; // end of namespace rofcore

#endif /* EROFCOREXCP_HPP_ */
