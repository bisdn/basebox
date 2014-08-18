/*
 * cgtpcore.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CGTPCORE_HPP_
#define CGTPCORE_HPP_

#include <map>
#include <iostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>

#include "clogging.h"
#include "cbearer.hpp"
#include "cterm.hpp"

namespace rofgtp {

class eGtpCoreBase : public std::runtime_error {
public:
	eGtpCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGtpCoreNotFound : public eGtpCoreBase {
public:
	eGtpCoreNotFound(const std::string& __arg) : eGtpCoreBase(__arg) {};
};

class cgtpcore {
public:

	/**
	 *
	 */
	static cgtpcore&
	add_gtp_core(const rofl::cdptid& dptid) {
		if (cgtpcore::gtpcores.find(dptid) != cgtpcore::gtpcores.end()) {
			delete cgtpcore::gtpcores[dptid];
			cgtpcore::gtpcores.erase(dptid);
		}
		cgtpcore::gtpcores[dptid] = new cgtpcore(dptid);
		return *(cgtpcore::gtpcores[dptid]);
	};

	/**
	 *
	 */
	static cgtpcore&
	set_gtp_core(const rofl::cdptid& dptid) {
		if (cgtpcore::gtpcores.find(dptid) == cgtpcore::gtpcores.end()) {
			cgtpcore::gtpcores[dptid] = new cgtpcore(dptid);
		}
		return *(cgtpcore::gtpcores[dptid]);
	};

	/**
	 *
	 */
	static const cgtpcore&
	get_gtp_core(const rofl::cdptid& dptid) {
		if (cgtpcore::gtpcores.find(dptid) == cgtpcore::gtpcores.end()) {
			throw eGtpCoreNotFound("cgtpcore::get_gtp_core() dptid not found");
		}
		return *(cgtpcore::gtpcores.at(dptid));
	};

	/**
	 *
	 */
	static void
	drop_gtp_core(const rofl::cdptid& dptid) {
		if (cgtpcore::gtpcores.find(dptid) == cgtpcore::gtpcores.end()) {
			return;
		}
		delete cgtpcore::gtpcores[dptid];
		cgtpcore::gtpcores.erase(dptid);
	}

	/**
	 *
	 */
	static bool
	has_gtp_core(const rofl::cdptid& dptid) {
		return (not (cgtpcore::gtpcores.find(dptid) == cgtpcore::gtpcores.end()));
	};

public:

	/**
	 *
	 */
	cgtpcore(const rofl::cdptid& dptid) :
		dptid(dptid) {};

	/**
	 *
	 */
	~cgtpcore() {};

public:

	/**
	 *
	 */
	void
	clear_bearers();

	/**
	 *
	 */
	cbearer&
	add_bearer() {

	};

	/**
	 *
	 */
	cbearer&
	set_bearer() {

	};

	/**
	 *
	 */
	const cbearer&
	get_bearer() const {

	};

	/**
	 *
	 */
	void
	drop_bearer() {

	};

	/**
	 *
	 */
	bool
	has_bearer() const {

	};

public:

	/**
	 *
	 */
	void
	clear_terms();

	/**
	 *
	 */
	cterm&
	add_term() {

	};

	/**
	 *
	 */
	cterm&
	set_term() {

	};

	/**
	 *
	 */
	const cterm&
	get_term() const {

	};

	/**
	 *
	 */
	void
	drop_term() {

	};

	/**
	 *
	 */
	bool
	has_term() const {

	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgtpcore& gtpcore) {

		return os;
	};

private:

	rofl::cdptid		dptid;

	static std::map<rofl::cdptid, cgtpcore*>	gtpcores;
};

}; // end of namespace rofgtp

#endif /* CGTPCORE_HPP_ */
