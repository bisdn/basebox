/*
 * cbearer.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CBEARER_HPP_
#define CBEARER_HPP_

#include <ostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fudpframe.h>

#include "clabel.hpp"
#include "clogging.h"

namespace rofgtp {

class eRelayBase : public std::runtime_error {
public:
	eRelayBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eRelayNotFound : public eRelayBase {
public:
	eRelayNotFound(const std::string& __arg) : eRelayBase(__arg) {};
};

class crelay {
public:

	/**
	 *
	 */
	crelay() :
		state(STATE_DETACHED), ofp_table_id(0) {};

	/**
	 *
	 */
	~crelay() {};

	/**
	 *
	 */
	crelay(const rofl::cdpid& dpid, uint8_t ofp_table_id) :
		state(STATE_DETACHED), dpid(dpid), ofp_table_id(ofp_table_id) {};

	/**
	 *
	 */
	crelay(const crelay& relay) { *this = relay; };

	/**
	 *
	 */
	crelay&
	operator= (const crelay& relay) {
		if (this == &relay)
			return *this;
		state = relay.state;
		dpid = relay.dpid;
		ofp_table_id = relay.ofp_table_id;
		return *this;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crelay& bearer) {
		os << rofcore::indent(0) << "<crelay >" << std::endl;
		return os;
	};

protected:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t state;
	rofl::cdpid dpid;
	uint8_t ofp_table_id;
};



class crelay_in4 : public crelay {
public:

	/**
	 *
	 */
	crelay_in4() {};

	/**
	 *
	 */
	~crelay_in4() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	crelay_in4(const rofl::cdpid& dpid, uint8_t ofp_table_id,
			const clabel_in4& label_in, const clabel_in4& label_out) :
		crelay(dpid, ofp_table_id), label_in(label_in), label_out(label_out) {};

	/**
	 *
	 */
	crelay_in4(const crelay_in4& relay) { *this = relay; };

	/**
	 *
	 */
	crelay_in4&
	operator= (const crelay_in4& relay) {
		if (this == &relay)
			return *this;
		crelay::operator= (relay);
		label_in = relay.label_in;
		label_out = relay.label_out;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const crelay_in4& relay) const {
		return ((label_in < relay.label_in) && (label_out < relay.label_out));
	};

public:
	
	/**
	 * 
	 */
	const clabel_in4&
	get_label_in() const { return label_in; };
	
	/**
	 *
	 */
	const clabel_in4&
	get_label_out() const { return label_out; };

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

	friend std::ostream&
	operator<< (std::ostream& os, const crelay_in4& relay) {
		os << rofcore::indent(0) << "<crelay_in4 >" << std::endl;
		os << rofcore::indent(2) << "<label-in >" << std::endl;
		{ rofcore::indent i(4); os << relay.get_label_in(); };
		os << rofcore::indent(2) << "<label-out >" << std::endl;
		{ rofcore::indent i(4); os << relay.get_label_out(); };
		return os;
	};

private:

	clabel_in4 label_in;
	clabel_in4 label_out;
};



class crelay_in6 : public crelay {
public:

	/**
	 *
	 */
	crelay_in6() {};

	/**
	 *
	 */
	~crelay_in6() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	crelay_in6(const rofl::cdpid& dpid, uint8_t ofp_table_id,
			const clabel_in6& label_in, const clabel_in6& label_out) :
		crelay(dpid, ofp_table_id), label_in(label_in), label_out(label_out) {};

	/**
	 *
	 */
	crelay_in6(const crelay_in6& relay) { *this = relay; };

	/**
	 *
	 */
	crelay_in6&
	operator= (const crelay_in6& relay) {
		if (this == &relay)
			return *this;
		label_in = relay.label_in;
		label_out = relay.label_out;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const crelay_in6& relay) const {
		return ((label_in < relay.label_in) && (label_out < relay.label_out));
	};

public:

	/**
	 *
	 */
	const clabel_in6&
	get_label_in() const { return label_in; };

	/**
	 *
	 */
	const clabel_in6&
	get_label_out() const { return label_out; };

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

	friend std::ostream&
	operator<< (std::ostream& os, const crelay_in6& relay) {
		os << rofcore::indent(0) << "<crelay_in6 >" << std::endl;
		os << rofcore::indent(2) << "<label-in >" << std::endl;
		{ rofcore::indent i(4); os << relay.get_label_in(); };
		os << rofcore::indent(2) << "<label-out >" << std::endl;
		{ rofcore::indent i(4); os << relay.get_label_out(); };
		return os;
	};

private:

	clabel_in6 label_in;
	clabel_in6 label_out;
};


}; // end of namespace rofgtp

#endif /* CBEARER_HPP_ */
