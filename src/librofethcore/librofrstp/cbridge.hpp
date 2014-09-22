/*
 * cbridge.hpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#ifndef CBRIDGE_HPP_
#define CBRIDGE_HPP_

#include <inttypes.h>
#include <map>
#include <ostream>
#include <exception>

#include "cbridgeid.hpp"

namespace rofeth {
namespace rstp {

class eBridgeBase : public std::runtime_error {
public:
	eBridgeBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eBridgeNotFound : public eBridgeBase {
public:
	eBridgeNotFound(const std::string& __arg) : std::runtime_error(__arg) {};
};

class cbridge {
	static std::map<uint64_t, cbridge*>	 	bridges;


	/**
	 *
	 */
	cbridge()
	{};

	/**
	 *
	 */
	cbridge(uint64_t bridgeid) :
		brid(bridgeid)
	{};

	/**
	 *
	 */
	~cbridge()
	{};

	/**
	 *
	 */
	cbridge(const cbridge& bridge);
		// private, not implemented

	/**
	 *
	 */
	cbridge&
	operator= (const cbridge& bridge);
		// private, not implemented

public:

	/**
	 *
	 */
	static cbridge&
	add_bridge(uint64_t bridgeid) {
		if (cbridge::bridges.find(bridgeid) != cbridge::bridges.end()) {
			delete cbridge::bridges[bridgeid];
		}
		cbridge::bridges[bridgeid] = new cbridge(bridgeid);
		return *(cbridge::bridges[bridgeid]);
	};

	/**
	 *
	 */
	static cbridge&
	set_bridge(uint64_t bridgeid) {
		if (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()) {
			cbridge::bridges[bridgeid] = new cbridge(bridgeid);
		}
		return *(cbridge::bridges[bridgeid]);
	};

	/**
	 *
	 */
	static const cbridge&
	get_bridge(uint64_t bridgeid) const {
		if (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()) {
			throw eBridgeNotFound("cbridge::get_bridge()");
		}
		return *(cbridge::bridges.at(bridgeid));
	};

	/**
	 *
	 */
	static void
	drop_bridge(uint64_t bridgeid) {
		if (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()) {
			return;
		}
		delete cbridge::bridges[bridgeid];
		cbridge::bridges.erase(bridgeid);
	};

	/**
	 *
	 */
	static bool
	has_bridge(uint64_t bridgeid) const {
		return (not (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()));
	};

public:

private:

	cbridgeid	brid;

};

}; // end of namespace rstp
}; // end of namespace rofeth

#endif /* CBRIDGE_HPP_ */
