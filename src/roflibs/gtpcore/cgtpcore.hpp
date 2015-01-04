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
#include <rofl/common/protocols/fudpframe.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/cauxid.h>
#include <rofl/common/openflow/messages/cofmsg_packet_in.h>
#include <rofl/common/openflow/messages/cofmsg_flow_removed.h>
#include <rofl/common/openflow/messages/cofmsg_error.h>

#include "roflibs/netlink/clogging.hpp"
#include "roflibs/gtpcore/crelay.hpp"
#include "roflibs/gtpcore/cterm.hpp"
#include "roflibs/netlink/ccookiebox.hpp"

namespace roflibs {
namespace gtp {

class eGtpCoreBase : public std::runtime_error {
public:
	eGtpCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGtpCoreNotFound : public eGtpCoreBase {
public:
	eGtpCoreNotFound(const std::string& __arg) : eGtpCoreBase(__arg) {};
};

class cgtpcore : public roflibs::common::openflow::ccookie_owner {
public:

	/**
	 *
	 */
	static cgtpcore&
	add_gtp_core(const rofl::cdpid& dpid, uint8_t ip_local_table_id, uint8_t gtp_table_id) {
		if (cgtpcore::gtpcores.find(dpid) != cgtpcore::gtpcores.end()) {
			delete cgtpcore::gtpcores[dpid];
			cgtpcore::gtpcores.erase(dpid);
		}
		cgtpcore::gtpcores[dpid] = new cgtpcore(dpid, ip_local_table_id, gtp_table_id);
		return *(cgtpcore::gtpcores[dpid]);
	};

	/**
	 *
	 */
	static cgtpcore&
	set_gtp_core(const rofl::cdpid& dpid, uint8_t ip_local_table_id, uint8_t gtp_table_id) {
		if (cgtpcore::gtpcores.find(dpid) == cgtpcore::gtpcores.end()) {
			cgtpcore::gtpcores[dpid] = new cgtpcore(dpid, ip_local_table_id, gtp_table_id);
		}
		return *(cgtpcore::gtpcores[dpid]);
	};


	/**
	 *
	 */
	static cgtpcore&
	set_gtp_core(const rofl::cdpid& dpid) {
		if (cgtpcore::gtpcores.find(dpid) == cgtpcore::gtpcores.end()) {
			throw eGtpCoreNotFound("cgtpcore::set_gtp_core() dpt not found");
		}
		return *(cgtpcore::gtpcores[dpid]);
	};

	/**
	 *
	 */
	static const cgtpcore&
	get_gtp_core(const rofl::cdpid& dpid) {
		if (cgtpcore::gtpcores.find(dpid) == cgtpcore::gtpcores.end()) {
			throw eGtpCoreNotFound("cgtpcore::get_gtp_core() dpt not found");
		}
		return *(cgtpcore::gtpcores.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_gtp_core(const rofl::cdpid& dpid) {
		if (cgtpcore::gtpcores.find(dpid) == cgtpcore::gtpcores.end()) {
			return;
		}
		delete cgtpcore::gtpcores[dpid];
		cgtpcore::gtpcores.erase(dpid);
	}

	/**
	 *
	 */
	static bool
	has_gtp_core(const rofl::cdpid& dpid) {
		return (not (cgtpcore::gtpcores.find(dpid) == cgtpcore::gtpcores.end()));
	};

private:

	/**
	 *
	 */
	cgtpcore(const rofl::cdpid& dpid, uint8_t ip_local_table_id, uint8_t gtp_table_id = 0) :
		state(STATE_DETACHED), dpid(dpid),
		cookie_miss_entry_ipv4(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
		cookie_miss_entry_ipv6(roflibs::common::openflow::ccookie_owner::acquire_cookie()),
		ip_local_table_id(ip_local_table_id), gtp_table_id(gtp_table_id) {};

	/**
	 *
	 */
	virtual
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
		for (std::map<clabel_in4, crelay_in4*>::iterator
				it = relays_in4.begin(); it != relays_in4.end(); ++it) {
			delete it->second;
		}
		relays_in4.clear();
	};

	/**
	 *
	 */
	crelay_in4&
	add_relay_in4(const clabel_in4& label_in, const clabel_in4& label_out) {
		if (relays_in4.find(label_in) != relays_in4.end()) {
			delete relays_in4[label_in];
			relays_in4.erase(label_in);
		}
		relays_in4[label_in] = new crelay_in4(dpid, gtp_table_id, label_in, label_out);
#if 0
		try {
			if (STATE_ATTACHED == state) {
				relays_in4[label_in]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(relays_in4[label_in]);
	};

	/**
	 *
	 */
	crelay_in4&
	set_relay_in4(const clabel_in4& label_in, const clabel_in4& label_out) {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			relays_in4[label_in] = new crelay_in4(dpid, gtp_table_id, label_in, label_out);
		}
#if 0
		try {
			if (STATE_ATTACHED == state) {
				relays_in4[label_in]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(relays_in4[label_in]);
	};

	/**
	 *
	 */
	crelay_in4&
	set_relay_in4(const clabel_in4& label_in) {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in4() label not found");
		}
		return *(relays_in4[label_in]);
	};

	/**
	 *
	 */
	const crelay_in4&
	get_relay_in4(const clabel_in4& label_in) const {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in4() label not found");
		}
		return *(relays_in4.at(label_in));
	};

	/**
	 *
	 */
	void
	drop_relay_in4(const clabel_in4& label_in) {
		if (relays_in4.find(label_in) == relays_in4.end()) {
			return;
		}
		delete relays_in4[label_in];
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
		for (std::map<clabel_in6, crelay_in6*>::iterator
				it = relays_in6.begin(); it != relays_in6.end(); ++it) {
			delete it->second;
		}
		relays_in6.clear();
	};

	/**
	 *
	 */
	crelay_in6&
	add_relay_in6(const clabel_in6& label_in, const clabel_in6& label_out) {
		if (relays_in6.find(label_in) != relays_in6.end()) {
			delete relays_in6[label_in];
			relays_in6.erase(label_in);
		}
		relays_in6[label_in] = new crelay_in6(dpid, gtp_table_id, label_in, label_out);
#if 0
		try {
			if (STATE_ATTACHED == state) {
				relays_in6[label_in]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(relays_in6[label_in]);
	};

	/**
	 *
	 */
	crelay_in6&
	set_relay_in6(const clabel_in6& label_in, const clabel_in6& label_out) {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			relays_in6[label_in] = new crelay_in6(dpid, gtp_table_id, label_in, label_out);
		}
#if 0
		try {
			if (STATE_ATTACHED == state) {
				relays_in6[label_in]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(relays_in6[label_in]);
	};

	/**
	 *
	 */
	crelay_in6&
	set_relay_in6(const clabel_in6& label_in) {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in6() label not found");
		}
		return *(relays_in6[label_in]);
	};

	/**
	 *
	 */
	const crelay_in6&
	get_relay_in6(const clabel_in6& label_in) const {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			throw eRelayNotFound("cgtpcore::get_relay_in6() label not found");
		}
		return *(relays_in6.at(label_in));
	};

	/**
	 *
	 */
	void
	drop_relay_in6(const clabel_in6& label_in) {
		if (relays_in6.find(label_in) == relays_in6.end()) {
			return;
		}
		delete relays_in6[label_in];
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
	clear_terms_in4() {
		for (std::map<clabel_in4, cterm_in4*>::iterator
				it = terms_in4.begin(); it != terms_in4.end(); ++it) {
			delete it->second;
		}
		terms_in4.clear();
	};

	/**
	 *
	 */
	cterm_in4&
	add_term_in4(const clabel_in4& label_egress, const clabel_in4& label_ingress, const rofl::openflow::cofmatch& tft_match) {
		if (terms_in4.find(label_egress) != terms_in4.end()) {
			delete terms_in4[label_egress];
			terms_in4.erase(label_egress);
		}
		terms_in4[label_egress] = new cterm_in4(dpid, gtp_table_id, label_egress, label_ingress, tft_match);
#if 0
		try {
			if (STATE_ATTACHED == state) {
				terms_in4[label_ingress]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(terms_in4[label_egress]);
	};

	/**
	 *
	 */
	cterm_in4&
	set_term_in4(const clabel_in4& label_egress, const clabel_in4& label_ingress, const rofl::openflow::cofmatch& tft_match) {
		if (terms_in4.find(label_egress) == terms_in4.end()) {
			terms_in4[label_egress] = new cterm_in4(dpid, gtp_table_id, label_egress, label_ingress, tft_match);
		}
#if 0
		try {
			if (STATE_ATTACHED == state) {
				terms_in4[label_egress]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(terms_in4[label_egress]);
	};

	/**
	 *
	 */
	cterm_in4&
	set_term_in4(const clabel_in4& label_egress) {
		if (terms_in4.find(label_egress) == terms_in4.end()) {
			throw eRelayNotFound("cgtpcore::get_term_in4() label not found");
		}
		return *(terms_in4[label_egress]);
	};

	/**
	 *
	 */
	cterm_in4&
	set_term_in4(const rofl::openflow::cofmatch& tft_match) {
		std::map<clabel_in4, cterm_in4*>::iterator it;
		if ((it = find_if(terms_in4.begin(), terms_in4.end(),
				cterm_in4::cterm_in4_find_by_tft_match(tft_match))) == terms_in4.end()) {
			throw eRelayNotFound("cgtpcore::set_term_in4() match not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const cterm_in4&
	get_term_in4(const clabel_in4& label_egress) const {
		if (terms_in4.find(label_egress) == terms_in4.end()) {
			throw eRelayNotFound("cgtpcore::get_term_in4() label not found");
		}
		return *(terms_in4.at(label_egress));
	};

	/**
	 *
	 */
	const cterm_in4&
	get_term_in4(const rofl::openflow::cofmatch& tft_match) const {
		std::map<clabel_in4, cterm_in4*>::const_iterator it;
		if ((it = find_if(terms_in4.begin(), terms_in4.end(),
				cterm_in4::cterm_in4_find_by_tft_match(tft_match))) == terms_in4.end()) {
			throw eRelayNotFound("cgtpcore::get_term_in4() match not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	void
	drop_term_in4(const clabel_in4& label_egress) {
		if (terms_in4.find(label_egress) == terms_in4.end()) {
			return;
		}
		delete terms_in4[label_egress];
		terms_in4.erase(label_egress);
	};

	/**
	 *
	 */
	bool
	has_term_in4(const clabel_in4& label_egress) const {
		return (not (terms_in4.find(label_egress) == terms_in4.end()));
	};

	/**
	 *
	 */
	bool
	has_term_in4(const rofl::openflow::cofmatch& tft_match) const {
		std::map<clabel_in4, cterm_in4*>::const_iterator it;
		 return (not (find_if(terms_in4.begin(), terms_in4.end(),
				cterm_in4::cterm_in4_find_by_tft_match(tft_match)) == terms_in4.end()));
	};

public:

	/**
	 *
	 */
	void
	clear_terms_in6() {
		for (std::map<clabel_in6, cterm_in6*>::iterator
				it = terms_in6.begin(); it != terms_in6.end(); ++it) {
			delete it->second;
		}
		terms_in6.clear();
	};

	/**
	 *
	 */
	cterm_in6&
	add_term_in6(const clabel_in6& label_egress, const clabel_in6& label_ingress, const rofl::openflow::cofmatch& tft_match) {
		if (terms_in6.find(label_egress) != terms_in6.end()) {
			delete terms_in6[label_egress];
			terms_in6.erase(label_egress);
		}
		terms_in6[label_egress] = new cterm_in6(dpid, gtp_table_id, label_egress, label_ingress, tft_match);
#if 0
		try {
			if (STATE_ATTACHED == state) {
				terms_in6[label_ingress]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(terms_in6[label_egress]);
	};

	/**
	 *
	 */
	cterm_in6&
	set_term_in6(const clabel_in6& label_egress, const clabel_in6& label_ingress, const rofl::openflow::cofmatch& tft_match) {
		if (terms_in6.find(label_egress) == terms_in6.end()) {
			terms_in6[label_egress] = new cterm_in6(dpid, gtp_table_id, label_egress, label_ingress, tft_match);
		}
#if 0
		try {
			if (STATE_ATTACHED == state) {
				terms_in6[label_egress]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
#endif
		return *(terms_in6[label_egress]);
	};

	/**
	 *
	 */
	cterm_in6&
	set_term_in6(const clabel_in6& label_egress) {
		if (terms_in6.find(label_egress) == terms_in6.end()) {
			throw eRelayNotFound("cgtpcore::get_term_in6() label not found");
		}
		return *(terms_in6[label_egress]);
	};

	/**
	 *
	 */
	cterm_in6&
	set_term_in6(const rofl::openflow::cofmatch& tft_match) {
		std::map<clabel_in6, cterm_in6*>::iterator it;
		if ((it = find_if(terms_in6.begin(), terms_in6.end(),
				cterm_in6::cterm_in6_find_by_tft_match(tft_match))) == terms_in6.end()) {
			throw eRelayNotFound("cgtpcore::set_term_in6() match not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	const cterm_in6&
	get_term_in6(const clabel_in6& label_egress) const {
		if (terms_in6.find(label_egress) == terms_in6.end()) {
			throw eRelayNotFound("cgtpcore::get_term_in6() label not found");
		}
		return *(terms_in6.at(label_egress));
	};

	/**
	 *
	 */
	const cterm_in6&
	get_term_in6(const rofl::openflow::cofmatch& tft_match) const {
		std::map<clabel_in6, cterm_in6*>::const_iterator it;
		if ((it = find_if(terms_in6.begin(), terms_in6.end(),
				cterm_in6::cterm_in6_find_by_tft_match(tft_match))) == terms_in6.end()) {
			throw eRelayNotFound("cgtpcore::get_term_in6() match not found");
		}
		return *(it->second);
	};

	/**
	 *
	 */
	void
	drop_term_in6(const clabel_in6& label_egress) {
		if (terms_in6.find(label_egress) == terms_in6.end()) {
			return;
		}
		delete terms_in6[label_egress];
		terms_in6.erase(label_egress);
	};

	/**
	 *
	 */
	bool
	has_term_in6(const clabel_in6& label_egress) const {
		return (not (terms_in6.find(label_egress) == terms_in6.end()));
	};

	/**
	 *
	 */
	bool
	has_term_in6(const rofl::openflow::cofmatch& tft_match) const {
		std::map<clabel_in6, cterm_in6*>::const_iterator it;
		 return (not (find_if(terms_in6.begin(), terms_in6.end(),
				cterm_in6::cterm_in6_find_by_tft_match(tft_match)) == terms_in6.end()));
	};

public:

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg);

	/**
	 *
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_flow_removed& msg) {};

	/**
	 *
	 */
	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_error& msg) {};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgtpcore& gtpcore) {
		os << rofcore::indent(0) << "<cgtpcore "
				<< "#in4-relay(s): " << gtpcore.relays_in4.size() << " "
				<< "#in6-relay(s): " << gtpcore.relays_in6.size() << " "
				<< "#in4-term(s): " << gtpcore.terms_in4.size() << " "
				<< "#in6-term(s): " << gtpcore.terms_in6.size() << " >" << std::endl;
		for (std::map<clabel_in4, crelay_in4*>::const_iterator
				it = gtpcore.relays_in4.begin(); it != gtpcore.relays_in4.end(); ++it) {
			rofcore::indent i(2); os << *(it->second);
		}
		for (std::map<clabel_in6, crelay_in6*>::const_iterator
				it = gtpcore.relays_in6.begin(); it != gtpcore.relays_in6.end(); ++it) {
			rofcore::indent i(2); os << *(it->second);
		}
		for (std::map<clabel_in4, cterm_in4*>::const_iterator
				it = gtpcore.terms_in4.begin(); it != gtpcore.terms_in4.end(); ++it) {
			rofcore::indent i(2); os << *(it->second);
		}
		for (std::map<clabel_in6, cterm_in6*>::const_iterator
				it = gtpcore.terms_in6.begin(); it != gtpcore.terms_in6.end(); ++it) {
			rofcore::indent i(2); os << *(it->second);
		}
		return os;
	};

	static const uint16_t DEFAULT_GTPU_PORT = 2152;

private:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t							state;
	rofl::cdpid									dpid;
	uint64_t									cookie_miss_entry_ipv4;
	uint64_t									cookie_miss_entry_ipv6;
	uint8_t										ip_local_table_id;
	uint8_t										gtp_table_id;
	std::map<clabel_in4, crelay_in4*>			relays_in4;
	std::map<clabel_in6, crelay_in6*>			relays_in6;
	std::map<clabel_in4, cterm_in4*>			terms_in4;
	std::map<clabel_in6, cterm_in6*>			terms_in6;
	static std::map<rofl::cdpid, cgtpcore*>		gtpcores;
};

}; // end of namespace gtp
}; // end of namespace roflibs

#endif /* CGTPCORE_HPP_ */
