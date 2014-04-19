/*
 * crofproxy.hpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#ifndef CROFPROXY_HPP_
#define CROFPROXY_HPP_

#include <string>

#include "erofcorexcp.hpp"

namespace rofcore {

class eRofProxyBase			: public eRofCoreXcp {};
class eRofProxyNotFound		: public eRofProxyBase {};

class crofproxy {
public:

	/**
	 * @brief	Existing proxy types
	 */
	enum rofproxy_type_t {
		PROXY_TYPE_ETHCORE		= 1,
		PROXY_TYPE_IPCORE		= 2,
		// add more proxy types here ...
		PROXY_TYPE_MAX,
	};

	/**
	 *
	 */
	static crofproxy*
	crofproxy_factory(
			enum rofproxy_type_t proxy_type);

public:

	/**
	 *
	 */
	crofproxy();

	/**
	 *
	 */
	virtual
	~crofproxy();

	/**
	 *
	 */
	crofproxy(
			crofproxy const& rofproxy);

	/**
	 *
	 */
	crofproxy&
	operator= (
			crofproxy const& rofproxy);

public:

	// TODO

public:

	friend std::ostream&
	operator<< (std::ostream& os, crofproxy const& rofproxy) {
		// TODO
		return os;
	};
};

}; // end of namespace rofcore

#endif /* CROFPROXY_HPP_ */
