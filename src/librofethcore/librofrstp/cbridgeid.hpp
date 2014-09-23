/*
 * cbridgeid.hpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#ifndef CBRIDGEID_HPP_
#define CBRIDGEID_HPP_

#include <inttypes.h>
#include <ostream>
#include <sstream>
#include <exception>

namespace rofeth {
namespace rstp {

class cbridgeid {
public:

	/**
	 *
	 */
	cbridgeid() :
		brid(0)
	{};

	/**
	 *
	 */
	cbridgeid(uint64_t brid) :
		brid(brid)
	{};

	/**
	 *
	 */
	~cbridgeid()
	{};

	/**
	 *
	 */
	cbridgeid(const cbridgeid& bridgeid) {
		*this = bridgeid;
	};

	/**
	 *
	 */
	cbridgeid&
	operator= (const cbridgeid& bridgeid) {
		if (this == &bridgeid)
			return *this;
		brid = bridgeid.brid;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cbridgeid& bridgeid) const {
		return (brid < bridgeid.brid);
	};

	/**
	 *
	 */
	bool
	operator== (const cbridgeid& bridgeid) const {
		return (brid == bridgeid.brid);
	};

	/**
	 *
	 */
	bool
	operator!= (const cbridgeid& bridgeid) const {
		return (brid != bridgeid.brid);
	};

public:

	const uint64_t&
	get_bridge_id() const { return brid; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbridgeid& bridgeid) {
		os << "<cbridgeid rid:" << (unsigned long long)bridgeid.brid << " >";
		return os;
	};

	const std::string
	str() {
		std::stringstream ss; ss << *this; return ss.str();
	};

private:

	uint64_t	brid;	// bridge identifier
};

}; // end of namespace rstp
}; // end of namespace rofeth

#endif /* CBRIDGEID_HPP_ */
