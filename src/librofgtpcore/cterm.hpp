/*
 * cterm.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CTERM_HPP_
#define CTERM_HPP_

#include <ostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/openflow/cofmatch.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fudpframe.h>

#include "clabel.hpp"
#include "clogging.hpp"


namespace rofgtp {

class eTermBase : public std::runtime_error {
public:
	eTermBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eTermNotFound : public eTermBase {
public:
	eTermNotFound(const std::string& __arg) : eTermBase(__arg) {};
};

class cterm {
public:

	/**
	 *
	 */
	cterm() :
		state(STATE_DETACHED), ofp_table_id(0), idle_timeout(DEFAULT_IDLE_TIMEOUT) {};

	/**
	 *
	 */
	~cterm() {};

	/**
	 *
	 */
	cterm(const rofl::cdpid& dpid, uint8_t ofp_table_id) :
		state(STATE_DETACHED), dpid(dpid), ofp_table_id(ofp_table_id), idle_timeout(DEFAULT_IDLE_TIMEOUT) {};

	/**
	 *
	 */
	cterm(const cterm& term) { *this = term; };

	/**
	 *
	 */
	cterm&
	operator= (const cterm& term) {
		if (this == &term)
			return *this;
		state = term.state;
		dpid = term.dpid;
		ofp_table_id = term.ofp_table_id;
		idle_timeout = term.idle_timeout;
		return *this;
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cterm& term) {
		os << rofcore::indent(0) << "<cterm >" << std::endl;
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

	static const int DEFAULT_IDLE_TIMEOUT = 15; // seconds

	int idle_timeout;
};



class cterm_in4 : public cterm {
public:

	/**
	 *
	 */
	cterm_in4() {};

	/**
	 *
	 */
	~cterm_in4() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	cterm_in4(const rofl::cdpid& dpid, uint8_t ofp_table_id,
			const clabel_in4& label_egress, const clabel_in4& label_ingress, const rofl::openflow::cofmatch& tft_match) :
		cterm(dpid, ofp_table_id), label_egress(label_egress), label_ingress(label_ingress), tft_match(tft_match) {};

	/**
	 *
	 */
	cterm_in4(const cterm_in4& term) { *this = term; };

	/**
	 *
	 */
	cterm_in4&
	operator= (const cterm_in4& term) {
		if (this == &term)
			return *this;
		cterm::operator= (term);
		label_egress = term.label_egress;
		label_ingress = term.label_ingress;
		tft_match = term.tft_match;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cterm_in4& term) const {
		return (label_ingress < term.label_ingress);
	};

public:

	/**
	 *
	 */
	const clabel_in4&
	get_label_egress() const { return label_egress; };

	/**
	 *
	 */
	const clabel_in4&
	get_label_ingress() const { return label_ingress; };

	/**
	 *
	 */
	const rofl::openflow::cofmatch&
	get_tft_match() const { return tft_match; };

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
	operator<< (std::ostream& os, const cterm_in4& term) {
		os << rofcore::indent(0) << "<cterm_in4 >" << std::endl;
		os << rofcore::indent(2) << "<gtp-label-ingress >" << std::endl;
		{ rofcore::indent i(4); os << term.get_label_ingress(); };
		os << rofcore::indent(2) << "<gtp-label-egress >" << std::endl;
		{ rofcore::indent i(4); os << term.get_label_egress(); };
		os << rofcore::indent(2) << "<tft-match >" << std::endl;
		{ rofcore::indent i(4); os << term.get_tft_match(); };
		return os;
	};

	class cterm_in4_find_by_tft_match {
		rofl::openflow::cofmatch tft_match;
	public:
		cterm_in4_find_by_tft_match(const rofl::openflow::cofmatch& tft_match) :
			tft_match(tft_match) {};
		bool operator() (const std::pair<clabel_in4, cterm_in4*>& p) {
			return (p.second->tft_match == tft_match);
		};
	};

private:

	clabel_in4 label_egress;
	clabel_in4 label_ingress;
	rofl::openflow::cofmatch tft_match;
};



class cterm_in6 : public cterm {
public:

	/**
	 *
	 */
	cterm_in6() {};

	/**
	 *
	 */
	~cterm_in6() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	cterm_in6(const rofl::cdpid& dpid, uint8_t ofp_table_id,
			const clabel_in6& label_egress, const clabel_in6& label_ingress, const rofl::openflow::cofmatch& tft_match) :
		cterm(dpid, ofp_table_id), label_egress(label_egress), label_ingress(label_ingress), tft_match(tft_match) {};

	/**
	 *
	 */
	cterm_in6(const cterm_in6& term) { *this = term; };

	/**
	 *
	 */
	cterm_in6&
	operator= (const cterm_in6& term) {
		if (this == &term)
			return *this;
		cterm::operator= (term);
		label_egress = term.label_egress;
		label_ingress = term.label_ingress;
		tft_match = term.tft_match;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cterm_in6& term) const {
		return (label_ingress < term.label_ingress);
	};

public:

	/**
	 *
	 */
	const clabel_in6&
	get_label_egress() const { return label_egress; };

	/**
	 *
	 */
	const clabel_in6&
	get_label_ingress() const { return label_ingress; };

	/**
	 *
	 */
	const rofl::openflow::cofmatch&
	get_tft_match() const { return tft_match; };

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
	operator<< (std::ostream& os, const cterm_in6& term) {
		os << rofcore::indent(0) << "<cterm_in6 >" << std::endl;
		os << rofcore::indent(2) << "<gtp-label-ingress >" << std::endl;
		{ rofcore::indent i(4); os << term.get_label_ingress(); };
		os << rofcore::indent(2) << "<gtp-label-egress >" << std::endl;
		{ rofcore::indent i(4); os << term.get_label_egress(); };
		os << rofcore::indent(2) << "<tft-match >" << std::endl;
		{ rofcore::indent i(4); os << term.get_tft_match(); };
		return os;
	};

	class cterm_in6_find_by_tft_match {
		rofl::openflow::cofmatch tft_match;
	public:
		cterm_in6_find_by_tft_match(const rofl::openflow::cofmatch& tft_match) :
			tft_match(tft_match) {};
		bool operator() (const std::pair<clabel_in6, cterm_in6*>& p) {
			return (p.second->tft_match == tft_match);
		};
	};

private:

	clabel_in6 label_egress;
	clabel_in6 label_ingress;
	rofl::openflow::cofmatch tft_match;
};

}; // end of namespace rofgtp

#endif /* CTERM_HPP_ */
