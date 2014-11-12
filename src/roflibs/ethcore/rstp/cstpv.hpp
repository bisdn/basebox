/*
 * cstpv.hpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#ifndef CSTPV_HPP_
#define CSTPV_HPP_

// spanning tree priority vector

#include <inttypes.h>
#include <ostream>
#include <stringstream>
#include <exception>

#include "cbridgeid.hpp"
#include "crpcost.hpp"
#include "cportid.hpp"

namespace roflibs {
namespace eth {
namespace rstp {

class cstpv {
public:

	/**
	 *
	 */
	cstpv()
	{};

	/**
	 *
	 */
	cstpv(
			const cbridgeid& root_bridgeid_D,
			const crpcost& rpcost_D,
			const cbridgeid& designated_bridgeid_D,
			const cportid& portid_D,
			const cportid& portid_B) :
		root_bridgeid_D(root_bridgeid_D),
		rpcost_D(rpcost_D),
		designated_bridgeid_D(designated_bridgeid_D),
		portid_D(portid_D),
		portid_B(portid_B)
	{};

	/**
	 *
	 */
	~cstpv()
	{};

	/**
	 *
	 */
	cstpv(const cstpv& stpv) {
		*this = stpv;
	};

	/**
	 *
	 */
	cstpv&
	operator= (const cstpv& stpv) {
		if (this == &stpv)
			return *this;
		root_bridgeid_D 		= stpv.root_bridgeid_D;
		rpcost_D 				= stpv.rpcost_D;
		designated_bridgeid_D 	= stpv.designated_bridgeid_D;
		portid_D 				= stpv.portid_D;
		portid_B 				= stpv.portid_B;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cstpv& stpv) const {
		return (
				((root_bridgeid_D < stpv.root_bridgeid_D)) ||
				((root_bridgeid_D == stpv.root_bridgeid_D) && (rpcost_D < stpv.rpcost_D)) ||
				((root_bridgeid_D == stpv.root_bridgeid_D) && (rpcost_D == stpv.rpcost_D) &&
						(designated_bridgeid_D < stpv.designated_bridgeid_D)) ||
				((root_bridgeid_D == stpv.root_bridgeid_D) && (rpcost_D == stpv.rpcost_D) &&
						(designated_bridgeid_D == stpv.designated_bridgeid_D) && (portid_D < stpv.portid_D)) ||
				((designated_bridgeid_D == stpv.designated_bridgeid_D) && (portid_D == stpv.portid_D))
				);
	};

	/**
	 *
	 */
	bool
	operator== (const cstpv& stpv) const {
		return (
				(root_bridgeid_D == stpv.root_bridgeid_D) &&
				(rpcost_D == stpv.rpcost_D) &&
				(designated_bridgeid_D == stpv.designated_bridgeid_D) &&
				(portid_D == stpv.portid_D) &&
				(portid_B == stpv.portid_B)
				);
	};

	/**
	 *
	 */
	bool
	operator!= (const cstpv& stpv) const {
		return (
				(root_bridgeid_D != stpv.root_bridgeid_D) ||
				(rpcost_D != stpv.rpcost_D) ||
				(designated_bridgeid_D != stpv.designated_bridgeid_D) ||
				(portid_D != stpv.portid_D) ||
				(portid_B != stpv.portid_B)
				);
	};

public:

	/**
	 *
	 */
	const cbridgeid&
	get_root_bridgeid_D() const { return root_bridgeid_D; };

	/**
	 *
	 */
	const crpcost&
	get_rpcost_D() const { return rpcost_D; };

	/**
	 *
	 */
	crpcost&
	set_rpcost_D() { return rpcost_D; };

	/**
	 *
	 */
	const cbridgeid&
	get_designated_bridgeid_D() const { return designated_bridgeid_D; };

	/**
	 *
	 */
	const cportid&
	get_portid_D() const { return portid_D; };

	/**
	 *
	 */
	const cportid&
	get_portid_B() const { return portid_B; };

public:

	/**
	 *
	 */
	cstpv&
	add_rpc(const crpcost& cost) {
		rpcost_D += cost;
		return *this;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cstpv& stpv) {
		os << "<cstpv "
				<< "root-bridgeid-D:" << stpv.root_bridgeid_D << " "
				<< "root-path-cost-D:" << stpv.rpcost_D << " "
				<< "designated-bridgeid-D:" << stpv.designated_bridgeid_D << " "
				<< "portid-D:" << stpv.portid_D << " "
				<< "portid-B:" << stpv.portid_B << " "
				<< ">";
		return os;
	};

	const std::string
	str() {
		std::stringstream ss; ss << *this; return ss.str();
	};

private:

	cbridgeid	root_bridgeid_D;		// root bridgeID defined in rcvd message
	crpcost		rpcost_D;				// root path costs to root bridgeID in rcvd message
	cbridgeid	designated_bridgeid_D;	// bridgeid of bridge sending the message
	cportid		portid_D;				// portid if port sending this message
	cportid		portid_B;				// portid of port receiving this message
};

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CSTPV_HPP_ */
