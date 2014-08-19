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
#include "crelay.hpp"
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
	cgtpcore(const rofl::cdptid& dptid, uint8_t gtp_table_id = 0) :
		state(STATE_DETACHED), dptid(dptid), gtp_table_id(gtp_table_id) {};

	/**
	 *
	 */
	~cgtpcore() {};

public:

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt);

public:

	/**
	 *
	 */
	void
	clear_relays_in4() {
		relays_in4.clear();
	};

	/**
	 *
	 */
	crelay_in4&
	add_relay_in4(const clabel_in4& label_in, const clabel_in4& label_out) {
		if (relays_in4.find(label_in) != relays_in4.end()) {
			relays_in4.erase(label_in);
		}
		relays_in4[label_in] = crelay_in4(dptid, gtp_table_id, label_in, label_out);
		try {
			if (STATE_ATTACHED == state) {
				relays_in4[label_in].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return relays_in4[label_in];
	};

	/**
	 *
	 */
	crelay_in4&
	set_relay_in4(const clabel_in4& label_in, const clabel_in4& label_out) {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			relays_in4[label_in] = crelay_in4(dptid, gtp_table_id, label_in, label_out);
		}
		try {
			if (STATE_ATTACHED == state) {
				relays_in4[label_in].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return relays_in4[label_in];
	};

	/**
	 *
	 */
	crelay_in4&
	set_relay_in4(const clabel_in4& label_in) {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in4() label not found");
		}
		return relays_in4[label_in];
	};

	/**
	 *
	 */
	const crelay_in4&
	get_relay_in4(const clabel_in4& label_in) const {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in4() label not found");
		}
		return relays_in4.at(label_in);
	};

	/**
	 *
	 */
	void
	drop_relay_in4(const clabel_in4& label_in) {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			return;
		}
		relays_in4.erase(label_in);
	};

	/**
	 *
	 */
	bool
	has_relay_in4(const clabel_in4& label_in) const {
		return (not (relays_in4.find(label_in) == relays_in4.end()));
	};




	/**
	 *
	 */
	void
	clear_relays_in6() {
		relays_in6.clear();
	};

	/**
	 *
	 */
	crelay_in6&
	add_relay_in6(const clabel_in6& label_in, const clabel_in6& label_out) {
		if (relays_in6.find(label_in) != relays_in6.end()) {
			relays_in6.erase(label_in);
		}
		relays_in6[label_in] = crelay_in6(dptid, gtp_table_id, label_in, label_out);
		try {
			if (STATE_ATTACHED == state) {
				relays_in6[label_in].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return relays_in6[label_in];
	};

	/**
	 *
	 */
	crelay_in6&
	set_relay_in6(const clabel_in6& label_in, const clabel_in6& label_out) {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			relays_in6[label_in] = crelay_in6(dptid, gtp_table_id, label_in, label_out);
		}
		try {
			if (STATE_ATTACHED == state) {
				relays_in6[label_in].handle_dpt_open(rofl::crofdpt::get_dpt(dptid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return relays_in6[label_in];
	};

	/**
	 *
	 */
	crelay_in6&
	set_relay_in6(const clabel_in6& label_in) {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in6() label not found");
		}
		return relays_in6[label_in];
	};

	/**
	 *
	 */
	const crelay_in6&
	get_relay_in6(const clabel_in6& label_in) const {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in6() label not found");
		}
		return relays_in6.at(label_in);
	};

	/**
	 *
	 */
	void
	drop_relay_in6(const clabel_in6& label_in) {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			return;
		}
		relays_in6.erase(label_in);
	};

	/**
	 *
	 */
	bool
	has_relay_in6(const clabel_in6& label_in) const {
		return (not (relays_in6.find(label_in) == relays_in6.end()));
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

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t							state;
	rofl::cdptid								dptid;
	uint8_t										gtp_table_id;
	std::map<clabel_in4, crelay_in4>			relays_in4;
	std::map<clabel_in6, crelay_in6>			relays_in6;
	static std::map<rofl::cdptid, cgtpcore*>	gtpcores;
};

}; // end of namespace rofgtp

#endif /* CGTPCORE_HPP_ */
