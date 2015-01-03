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
#include <sstream>
#include <exception>

namespace roflibs {
namespace eth {
namespace rstp {

class crpcost {
public:

	// recommended values according to IEEE 802.1D-2004
	enum port_path_cost_t {
		PORT_PATH_COST_100K = 200000000,
		PORT_PATH_COST_1M 	= 20000000,
		PORT_PATH_COST_10M 	= 2000000,
		PORT_PATH_COST_100M = 200000,
		PORT_PATH_COST_1G	= 20000,
		PORT_PATH_COST_10G	= 2000,
		PORT_PATH_COST_100G	= 200,
		PORT_PATH_COST_1T	= 20,
		PORT_PATH_COST_10T	= 2,
	};

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

	/**
	 *
	 */
	crpcost
	operator+ (const crpcost& rpcost) {
		crpcost cost(this->rpc);
		cost += rpcost.rpc;
		return cost;
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
};

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CRPCOST_HPP_ */
