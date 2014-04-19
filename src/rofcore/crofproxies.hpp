/*
 * crofproxies.hpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#ifndef CROFPROXIES_HPP_
#define CROFPROXIES_HPP_

#include <inttypes.h>

#include <map>
#include <string>
#include <iostream>

#include <rofl/common/crofbase.h>

#include "crofproxy.hpp"
#include "croflog.hpp"
#include "erofcorexcp.hpp"

namespace rofcore {

class crofproxies : public rofl::crofbase {

	static std::set<crofproxies*>			rofproxies;
	enum crofproxy::rofproxy_type_t			proxy_type;
	std::map<uint64_t, crofproxy*>			proxies;

public:

	/**
	 *
	 * @param signum
	 */
	static void
	crofproxies_sa_handler(
			int signum);

public:

	/**
	 *
	 */
	crofproxies(
			enum crofproxy::rofproxy_type_t proxy_type,
			rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap);

	/**
	 *
	 */
	virtual
	~crofproxies();

	/**
	 *
	 */
	crofproxies(
			crofproxies const& rofproxies);

	/**
	 *
	 */
	crofproxies&
	operator= (
			crofproxies const& rofproxies);

public:

	/**
	 *
	 */
	virtual void
	signal_handler(
			int signum);

	/**
	 *
	 */
	void
	clear();

	/**
	 *
	 */
	crofproxy&
	add_proxy(
			uint64_t dpid);

	/**
	 *
	 */
	crofproxy&
	set_proxy(
			uint64_t dpid);

	/**
	 *
	 */
	crofproxy const&
	get_proxy(
			uint64_t dpid) const;

	/**
	 *
	 */
	void
	drop_proxy(
			uint64_t dpid);

	/**
	 *
	 */
	bool
	has_proxy(
			uint64_t dpid) const;

public:

	friend std::ostream&
	operator<< (std::ostream& os, crofproxies const& rofproxies) {
		// TODO
		return os;
	};
};

}; // end of namespace rofcore

#endif /* ROFPROXIES_HPP_ */
