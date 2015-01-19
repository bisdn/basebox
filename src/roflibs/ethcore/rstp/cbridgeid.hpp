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

#include <rofl/common/caddress.h>

namespace roflibs {
namespace eth {
namespace rstp {

class cbridgeid {
public:

	/**
	 *
	 */
	cbridgeid() :
		priority(0), system_id(0), braddr(rofl::caddress_ll("00:00:00:00:00:00"))
	{};

	/**
	 *
	 */
	cbridgeid(uint64_t brid) :
		priority(0), system_id(0)
	{ /* TODO */ };

	/**
	 *
	 */
	cbridgeid(uint8_t priority, uint16_t system_id, const rofl::caddress_ll& braddr) :
		priority(priority), system_id(system_id), braddr(braddr)
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
		priority 	= bridgeid.priority;
		system_id 	= bridgeid.system_id;
		braddr 		= bridgeid.braddr;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cbridgeid& bridgeid) const {
		return ((priority 	< bridgeid.priority) ||
				(system_id 	< bridgeid.system_id) ||
				(braddr 	< bridgeid.braddr));
	};

	/**
	 *
	 */
	bool
	operator== (const cbridgeid& bridgeid) const {
		return ((priority == bridgeid.priority) &&
				(system_id == bridgeid.system_id) &&
				(braddr == bridgeid.braddr));
	};

	/**
	 *
	 */
	bool
	operator!= (const cbridgeid& bridgeid) const {
		return not operator== (bridgeid);
	};

public:

	uint64_t
	get_bridge_id() const {
		uint64_t brid(0);
		brid = (brid & ~((0x4ULL) << 60)) | ((uint64_t)(priority & 0xf0) << 60);
		brid = (brid & ~((0x0fffULL) << 48)) | ((uint64_t)(system_id & 0x0fff) << 48);
		((uint8_t*)&brid)[2] = braddr[0];
		((uint8_t*)&brid)[3] = braddr[1];
		((uint8_t*)&brid)[4] = braddr[2];
		((uint8_t*)&brid)[5] = braddr[3];
		((uint8_t*)&brid)[6] = braddr[4];
		((uint8_t*)&brid)[7] = braddr[5];
		return brid;
	};

	void
	set_bridge_id(uint64_t brid) {
		priority 	= ((brid & ((0x4ULL) << 60)) >> 60);
		system_id 	= ((brid & ((0x0fffULL) << 48)) >> 48);
		braddr[0] 	= ((uint8_t*)&brid)[2];
		braddr[1] 	= ((uint8_t*)&brid)[3];
		braddr[2] 	= ((uint8_t*)&brid)[4];
		braddr[3] 	= ((uint8_t*)&brid)[5];
		braddr[4] 	= ((uint8_t*)&brid)[6];
		braddr[5] 	= ((uint8_t*)&brid)[7];
	};

	uint8_t
	get_priority() const
	{ return priority; };

	void
	set_priority(uint8_t priority)
	{ this->priority = (priority & 0x000f); };

	uint16_t
	get_system_id() const
	{ return system_id; };

	void
	set_system_id(uint16_t system_id)
	{ this->system_id = (system_id & 0x0fff); };

	rofl::caddress_ll
	get_bridge_address() const
	{ return braddr; };

	void
	set_bridge_address(const rofl::caddress_ll& braddr)
	{ this->braddr = braddr; };

	// 7.12.5 and 9.2.5

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbridgeid& bridgeid) {
		os << "<cbridgeid rid:" << (unsigned long long)bridgeid.get_bridge_id() << " >";
		return os;
	};

	const std::string
	str() const {
		std::stringstream ss; ss << std::hex << "0x" << get_bridge_id() << std::dec; return ss.str();
		//std::stringstream ss; ss << *this; return ss.str();
	};

private:

	uint8_t				priority;
	uint16_t			system_id;
	rofl::caddress_ll	braddr;
};

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CBRIDGEID_HPP_ */
