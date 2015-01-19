/*
 * cbearer.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CRELAY_HPP_
#define CRELAY_HPP_

#include <ostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fudpframe.h>
#include <rofl/common/crofbase.h>

#include <roflibs/gtpcore/clabel.hpp>
#include <roflibs/netlink/clogging.hpp>

namespace roflibs {
namespace gtp {

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
		state(STATE_DETACHED), ofp_table_id(0), idle_timeout(DEFAULT_IDLE_TIMEOUT) {};

	/**
	 *
	 */
	~crelay() {};

	/**
	 *
	 */
	crelay(const rofl::cdptid& dptid, uint8_t ofp_table_id) :
		state(STATE_DETACHED), dptid(dptid), ofp_table_id(ofp_table_id), idle_timeout(DEFAULT_IDLE_TIMEOUT) {};

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
		dptid = relay.dptid;
		ofp_table_id = relay.ofp_table_id;
		idle_timeout = relay.idle_timeout;
		return *this;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const crelay& relay) {
		os << rofcore::indent(0) << "<crelay >" << std::endl;
		return os;
	};

protected:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t state;
	rofl::cdptid dptid;
	uint8_t ofp_table_id;

	static const int DEFAULT_IDLE_TIMEOUT = 15; // seconds

	int idle_timeout;
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
				handle_dpt_close();
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	crelay_in4(const rofl::cdptid& dptid, uint8_t ofp_table_id,
			const clabel_in4& label_in, const clabel_in4& label_out) :
		crelay(dptid, ofp_table_id), label_in(label_in), label_out(label_out) {};

	/**
	 *
	 */
	crelay_in4(const crelay_in4& relay)
	{ *this = relay; };

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
	get_label_in() const
	{ return label_in; };
	
	/**
	 *
	 */
	const clabel_in4&
	get_label_out() const
	{ return label_out; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open();

	/**
	 *
	 */
	void
	handle_dpt_close();

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
				handle_dpt_close();
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	crelay_in6(const rofl::cdptid& dptid, uint8_t ofp_table_id,
			const clabel_in6& label_in, const clabel_in6& label_out) :
		crelay(dptid, ofp_table_id), label_in(label_in), label_out(label_out) {};

	/**
	 *
	 */
	crelay_in6(const crelay_in6& relay)
	{ *this = relay; };

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
	get_label_in() const
	{ return label_in; };

	/**
	 *
	 */
	const clabel_in6&
	get_label_out() const
	{ return label_out; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open();

	/**
	 *
	 */
	void
	handle_dpt_close();

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

}; // end of namespace gtp
}; // end of namespace roflibs

#endif /* CRELAY_HPP_ */
