/*
 * cterm.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CTERM_HPP_
#define CTERM_HPP_

#include <bitset>
#include <ostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/openflow/cofmatch.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/protocols/fudpframe.h>
#include <rofl/common/crofbase.h>

#include <roflibs/gtpcore/clabel.hpp>
#include <roflibs/netlink/clogging.hpp>


namespace roflibs {
namespace gtp {

class eTermBase : public std::runtime_error {
public:
	eTermBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eTermNotFound : public eTermBase {
public:
	eTermNotFound(const std::string& __arg) : eTermBase(__arg) {};
};


/*
 * -ingress- means: send traffic into the tunnel
 * -egress-  means: strip the tunnel and send traffic to the external world
 */
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
	cterm(const rofl::cdptid& dptid, uint8_t ofp_table_id, const std::string& devname) :
		state(STATE_DETACHED), dptid(dptid),
		ofp_table_id(ofp_table_id), devname(devname), idle_timeout(DEFAULT_IDLE_TIMEOUT)
	{};

	/**
	 *
	 */
	cterm(const cterm& term)
	{ *this = term; };

	/**
	 *
	 */
	cterm&
	operator= (const cterm& term) {
		if (this == &term)
			return *this;
		state = term.state;
		dptid = term.dptid;
		ofp_table_id = term.ofp_table_id;
		devname = term.devname;
		idle_timeout = term.idle_timeout;
		return *this;
	};

public:

	/**
	 *
	 */
	const std::string&
	get_devname() const
	{ return devname; };

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
	enum ofp_state_t 	state;
	enum cterm_flags_t {
		FLAG_INGRESS_FM_INSTALLED 	= (1 << 0),
		FLAG_EGRESS_FM_INSTALLED 	= (1 << 1),
	};
	std::bitset<32>		flags;
	rofl::cdptid 		dptid;
	uint8_t 			ofp_table_id;
	std::string			devname;

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
				handle_dpt_close();
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	cterm_in4(
			const rofl::cdptid& dptid,
			uint8_t ofp_table_id,
			const std::string& devname,
			const clabel_in4& label_egress,
			const clabel_in4& label_ingress,
			const rofl::openflow::cofmatch& tft_match) :
				cterm(dptid, ofp_table_id, devname),
				label_egress(label_egress),
				label_ingress(label_ingress),
				tft_match(tft_match)
	{};

	/**
	 *
	 */
	cterm_in4(const cterm_in4& term)
	{ *this = term; };

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
	get_label_egress() const
	{ return label_egress; };

	/**
	 *
	 */
	const clabel_in4&
	get_label_ingress() const
	{ return label_ingress; };

	/**
	 *
	 */
	const rofl::openflow::cofmatch&
	get_tft_match() const
	{ return tft_match; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open_egress();

	/**
	 *
	 */
	void
	handle_dpt_close_egress();

	/**
	 *
	 */
	void
	handle_dpt_open_ingress();

	/**
	 *
	 */
	void
	handle_dpt_close_ingress();

	/**
	 *
	 */
	void
	handle_dpt_close() {
		handle_dpt_close_egress();
		handle_dpt_close_ingress();
	};

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

	class cterm_in4_find_by_devname {
		std::string devname;
	public:
		cterm_in4_find_by_devname(const std::string& devname) :
			devname(devname) {};
		bool operator() (const std::pair<unsigned int, cterm_in4*>& p) {
			return (p.second->devname == devname);
		};
	};

	class cterm_in4_find_by_tft_match {
		rofl::openflow::cofmatch tft_match;
	public:
		cterm_in4_find_by_tft_match(const rofl::openflow::cofmatch& tft_match) :
			tft_match(tft_match) {};
		bool operator() (const std::pair<unsigned int, cterm_in4*>& p) {
			return (p.second->tft_match == tft_match);
		};
	};

	class cterm_in4_find_by_label_in {
		roflibs::gtp::clabel_in4 label_in;
	public:
		cterm_in4_find_by_label_in(const roflibs::gtp::clabel_in4& label_in) :
			label_in(label_in) {};
		bool operator() (const std::pair<unsigned int, cterm_in4*>& p) {
			return (p.second->label_ingress == label_in);
		};
	};

	class cterm_in4_find_by_label_out {
		roflibs::gtp::clabel_in4 label_out;
	public:
		cterm_in4_find_by_label_out(const roflibs::gtp::clabel_in4& label_out) :
			label_out(label_out) {};
		bool operator() (const std::pair<unsigned int, cterm_in4*>& p) {
			return (p.second->label_egress == label_out);
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
				handle_dpt_close();
			}
		} catch (rofl::eRofDptNotFound& e) {};
	};

	/**
	 *
	 */
	cterm_in6(
			const rofl::cdptid& dptid,
			uint8_t ofp_table_id,
			const std::string& devname,
			const clabel_in6& label_egress,
			const clabel_in6& label_ingress,
			const rofl::openflow::cofmatch& tft_match) :
				cterm(dptid, ofp_table_id, devname),
				label_egress(label_egress),
				label_ingress(label_ingress),
				tft_match(tft_match)
	{};

	/**
	 *
	 */
	cterm_in6(const cterm_in6& term)
	{ *this = term; };

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
	get_label_egress() const
	{ return label_egress; };

	/**
	 *
	 */
	const clabel_in6&
	get_label_ingress() const
	{ return label_ingress; };

	/**
	 *
	 */
	const rofl::openflow::cofmatch&
	get_tft_match() const
	{ return tft_match; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open_egress();

	/**
	 *
	 */
	void
	handle_dpt_close_egress();

	/**
	 *
	 */
	void
	handle_dpt_open_ingress();

	/**
	 *
	 */
	void
	handle_dpt_close_ingress();

	/**
	 *
	 */
	void
	handle_dpt_close() {
		handle_dpt_close_egress();
		handle_dpt_close_ingress();
	};

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

	class cterm_in6_find_by_devname {
		std::string devname;
	public:
		cterm_in6_find_by_devname(const std::string& devname) :
			devname(devname) {};
		bool operator() (const std::pair<unsigned int, cterm_in6*>& p) {
			return (p.second->devname == devname);
		};
	};

	class cterm_in6_find_by_tft_match {
		rofl::openflow::cofmatch tft_match;
	public:
		cterm_in6_find_by_tft_match(const rofl::openflow::cofmatch& tft_match) :
			tft_match(tft_match) {};
		bool operator() (const std::pair<unsigned int, cterm_in6*>& p) {
			return (p.second->tft_match == tft_match);
		};
	};

	class cterm_in6_find_by_label_in {
		roflibs::gtp::clabel_in6 label_in;
	public:
		cterm_in6_find_by_label_in(const roflibs::gtp::clabel_in6& label_in) :
			label_in(label_in) {};
		bool operator() (const std::pair<unsigned int, cterm_in6*>& p) {
			return (p.second->label_ingress == label_in);
		};
	};

	class cterm_in6_find_by_label_out {
		roflibs::gtp::clabel_in6 label_out;
	public:
		cterm_in6_find_by_label_out(const roflibs::gtp::clabel_in6& label_out) :
			label_out(label_out) {};
		bool operator() (const std::pair<unsigned int, cterm_in6*>& p) {
			return (p.second->label_egress == label_out);
		};
	};

private:

	clabel_in6 label_egress;
	clabel_in6 label_ingress;
	rofl::openflow::cofmatch tft_match;
};

}; // end of namespace gtp
}; // end of namespace roflibs

#endif /* CTERM_HPP_ */
