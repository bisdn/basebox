/*
 * crpcost.hpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#ifndef CRPCOST_HPP_
#define CRPCOST_HPP_

#include <inttypes.h>
#include <ostream>
#include <stringstream>
#include <exception>

namespace roflibs {
namespace ethernet {
namespace rstp {

class crpcost {
public:

	/**
	 *
	 */
	crpcost() :
		rpc(0)
	{};

	/**
	 *
	 */
	crpcost(uint32_t rpc) :
		rpc(rpc)
	{};

	/**
	 *
	 */
	~crpcost()
	{};

	/**
	 *
	 */
	crpcost(const crpcost& rpcost) {
		*this = rpcost;
	};

	/**
	 *
	 */
	crpcost&
	operator= (const crpcost& rpcost) {
		if (this == &rpcost)
			return *this;
		rpc = rpcost.rpc;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const crpcost& rpcost) const {
		return (rpc < rpcost.rpc);
	};

	/**
	 *
	 */
	bool
	operator== (const crpcost& rpcost) const {
		return (rpc == rpcost.rpc);
	};

	/**
	 *
	 */
	bool
	operator!= (const crpcost& rpcost) const {
		return (rpc != rpcost.rpc);
	};

	/**
	 *
	 */
	crpcost&
	operator+= (const crpcost& rpcost) {
		rpc += rpcost.rpc;
		return *this;
	};

public:

	const uint32_t&
	get_rpcost() const { return rpc; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crpcost& rpcost) {
		os << "<crpcost root-path-cost:" << (unsigned long long)rpcost.rpc << " >";
		return os;
	};

	const std::string
	str() {
		std::stringstream ss; ss << *this; return ss.str();
	};

private:

	uint32_t	rpc;	// root path cost
}

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CRPCOST_HPP_ */
