/*
 * cgrecore.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CGRECORE_HPP_
#define CGRECORE_HPP_

#include <map>
#include <iostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/protocols/fudpframe.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/thread_helper.h>

#include <roflibs/netlink/clogging.hpp>
#include <roflibs/grecore/cgreterm.hpp>

namespace roflibs {
namespace gre {

class eGreCoreBase : public std::runtime_error {
public:
	eGreCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGreCoreNotFound : public eGreCoreBase {
public:
	eGreCoreNotFound(const std::string& __arg) : eGreCoreBase(__arg) {};
};

class cgrecore {
public:

	/**
	 *
	 */
	static cgrecore&
	add_gre_core(const rofl::cdpid& dpid, uint8_t eth_local_table_id, uint8_t ip_local_table_id, uint8_t gre_local_table_id, uint8_t ip_fwd_table_id) {
		if (cgrecore::grecores.find(dpid) != cgrecore::grecores.end()) {
			delete cgrecore::grecores[dpid];
			cgrecore::grecores.erase(dpid);
		}
		cgrecore::grecores[dpid] = new cgrecore(dpid, eth_local_table_id, ip_local_table_id, gre_local_table_id, ip_fwd_table_id);
		return *(cgrecore::grecores[dpid]);
	};

	/**
	 *
	 */
	static cgrecore&
	set_gre_core(const rofl::cdpid& dpid, uint8_t eth_local_table_id, uint8_t ip_local_table_id, uint8_t gre_local_table_id, uint8_t ip_fwd_table_id) {
		if (cgrecore::grecores.find(dpid) == cgrecore::grecores.end()) {
			cgrecore::grecores[dpid] = new cgrecore(dpid, eth_local_table_id, ip_local_table_id, gre_local_table_id, ip_fwd_table_id);
		}
		return *(cgrecore::grecores[dpid]);
	};


	/**
	 *
	 */
	static cgrecore&
	set_gre_core(const rofl::cdpid& dpid) {
		if (cgrecore::grecores.find(dpid) == cgrecore::grecores.end()) {
			throw eGreCoreNotFound("cgrecore::set_gre_core() dpt not found");
		}
		return *(cgrecore::grecores[dpid]);
	};

	/**
	 *
	 */
	static const cgrecore&
	get_gre_core(const rofl::cdpid& dpid) {
		if (cgrecore::grecores.find(dpid) == cgrecore::grecores.end()) {
			throw eGreCoreNotFound("cgrecore::get_gre_core() dpt not found");
		}
		return *(cgrecore::grecores.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_gre_core(const rofl::cdpid& dpid) {
		if (cgrecore::grecores.find(dpid) == cgrecore::grecores.end()) {
			return;
		}
		delete cgrecore::grecores[dpid];
		cgrecore::grecores.erase(dpid);
	}

	/**
	 *
	 */
	static bool
	has_gre_core(const rofl::cdpid& dpid) {
		return (not (cgrecore::grecores.find(dpid) == cgrecore::grecores.end()));
	};

private:

	/**
	 *
	 */
	cgrecore(const rofl::cdpid& dpid,
			uint8_t eth_local_table_id,
			uint8_t ip_local_table_id,
			uint8_t gre_local_table_id,
			uint8_t ip_fwd_table_id) :
		state(STATE_DETACHED), dpid(dpid),
		eth_local_table_id(eth_local_table_id),
		ip_local_table_id(ip_local_table_id),
		gre_local_table_id(gre_local_table_id),
		ip_fwd_table_id(ip_fwd_table_id) {};

	/**
	 *
	 */
	~cgrecore() {
		while (not terms_in4.empty()) {
			uint32_t term_id = terms_in4.begin()->first;
			drop_gre_term_in4(term_id);
		}
		while (not terms_in6.empty()) {
			uint32_t term_id = terms_in6.begin()->first;
			drop_gre_term_in6(term_id);
		}
	};

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
	std::vector<uint32_t>
	get_gre_terms_in4() const {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_READ);
		std::vector<uint32_t> termids;
		for (std::map<uint32_t, cgreterm_in4*>::const_iterator
				it = terms_in4.begin(); it != terms_in4.end(); ++it) {
			termids.push_back(it->first);
		}
		return termids;
	};

	/**
	 *
	 */
	void
	clear_gre_terms_in4() {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_WRITE);
		for (std::map<uint32_t, cgreterm_in4*>::iterator
				it = terms_in4.begin(); it != terms_in4.end(); ++it) {
			delete it->second;
		}
		terms_in4.clear();
	};

	/**
	 *
	 */
	cgreterm_in4&
	add_gre_term_in4(uint32_t term_id, uint32_t gre_portno,
			const rofl::caddress_in4& laddr, const rofl::caddress_in4& raddr, uint32_t gre_key) {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_WRITE);
		if (terms_in4.find(term_id) != terms_in4.end()) {
			delete terms_in4[term_id];
			terms_in4.erase(term_id);
		}
		terms_in4[term_id] = new cgreterm_in4(dpid, eth_local_table_id, gre_local_table_id, ip_fwd_table_id,
												laddr, raddr, gre_portno, gre_key);
		try {
			if (STATE_ATTACHED == state) {
				terms_in4[term_id]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return *(terms_in4[term_id]);
	};

	/**
	 *
	 */
	cgreterm_in4&
	set_gre_term_in4(uint32_t term_id, uint32_t gre_portno,
			const rofl::caddress_in4& laddr, const rofl::caddress_in4& raddr, uint32_t gre_key) {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_WRITE);
		if (terms_in4.find(term_id) == terms_in4.end()) {
			terms_in4[term_id] = new cgreterm_in4(dpid, eth_local_table_id, gre_local_table_id, ip_fwd_table_id,
													laddr, raddr, gre_portno, gre_key);
		}
		try {
			if (STATE_ATTACHED == state) {
				terms_in4[term_id]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return *(terms_in4[term_id]);
	};

	/**
	 *
	 */
	cgreterm_in4&
	set_gre_term_in4(uint32_t term_id) {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_READ);
		if (terms_in4.find(term_id) == terms_in4.end()) {
			throw eGreTermNotFound("cgrecore::get_gre_term_in4() term_id not found");
		}
		return *(terms_in4[term_id]);
	};

	/**
	 *
	 */
	const cgreterm_in4&
	get_gre_term_in4(uint32_t term_id) const {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_READ);
		if (terms_in4.find(term_id) == terms_in4.end()) {
			throw eGreTermNotFound("cgrecore::get_term_in4() term_id not found");
		}
		return *(terms_in4.at(term_id));
	};

	/**
	 *
	 */
	void
	drop_gre_term_in4(uint32_t term_id) {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_WRITE);
		if (terms_in4.find(term_id) == terms_in4.end()) {
			return;
		}
		delete terms_in4[term_id];
		terms_in4.erase(term_id);
	};

	/**
	 *
	 */
	bool
	has_gre_term_in4(uint32_t term_id) const {
		rofl::RwLock rwlock(rwlock_in4, rofl::RwLock::RWLOCK_READ);
		return (not (terms_in4.find(term_id) == terms_in4.end()));
	};

public:

	/**
	 *
	 */
	std::vector<uint32_t>
	get_gre_terms_in6() const {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_READ);
		std::vector<uint32_t> termids;
		for (std::map<uint32_t, cgreterm_in6*>::const_iterator
				it = terms_in6.begin(); it != terms_in6.end(); ++it) {
			termids.push_back(it->first);
		}
		return termids;
	};

	/**
	 *
	 */
	void
	clear_gre_terms_in6() {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_WRITE);
		for (std::map<uint32_t, cgreterm_in6*>::iterator
				it = terms_in6.begin(); it != terms_in6.end(); ++it) {
			delete it->second;
		}
		terms_in6.clear();
	};

	/**
	 *
	 */
	cgreterm_in6&
	add_gre_term_in6(uint32_t term_id, uint32_t gre_portno,
			const rofl::caddress_in6& laddr, const rofl::caddress_in6& raddr, uint32_t gre_key) {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_WRITE);
		if (terms_in6.find(term_id) != terms_in6.end()) {
			delete terms_in6[term_id];
			terms_in6.erase(term_id);
		}
		terms_in6[term_id] = new cgreterm_in6(dpid, eth_local_table_id, gre_local_table_id, ip_fwd_table_id,
												laddr, raddr, gre_portno, gre_key);
		try {
			if (STATE_ATTACHED == state) {
				terms_in6[term_id]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return *(terms_in6[term_id]);
	};

	/**
	 *
	 */
	cgreterm_in6&
	set_gre_term_in6(uint32_t term_id, uint32_t gre_portno,
			const rofl::caddress_in6& laddr, const rofl::caddress_in6& raddr, uint32_t gre_key) {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_WRITE);
		if (terms_in6.find(term_id) == terms_in6.end()) {
			terms_in6[term_id] = new cgreterm_in6(dpid, eth_local_table_id, gre_local_table_id, ip_fwd_table_id,
													laddr, raddr, gre_portno, gre_key);
		}
		try {
			if (STATE_ATTACHED == state) {
				terms_in6[term_id]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return *(terms_in6[term_id]);
	};

	/**
	 *
	 */
	cgreterm_in6&
	set_gre_term_in6(uint32_t term_id) {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_READ);
		if (terms_in6.find(term_id) == terms_in6.end()) {
			throw eGreTermNotFound("cgrecore::get_gre_term_in6() term_id not found");
		}
		return *(terms_in6[term_id]);
	};

	/**
	 *
	 */
	const cgreterm_in6&
	get_gre_term_in6(uint32_t term_id) const {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_READ);
		if (terms_in6.find(term_id) == terms_in6.end()) {
			throw eGreTermNotFound("cgrecore::get_term_in6() term_id not found");
		}
		return *(terms_in6.at(term_id));
	};

	/**
	 *
	 */
	void
	drop_gre_term_in6(uint32_t term_id) {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_WRITE);
		if (terms_in6.find(term_id) == terms_in6.end()) {
			return;
		}
		delete terms_in6[term_id];
		terms_in6.erase(term_id);
	};

	/**
	 *
	 */
	bool
	has_gre_term_in6(uint32_t term_id) const {
		rofl::RwLock rwlock(rwlock_in6, rofl::RwLock::RWLOCK_READ);
		return (not (terms_in6.find(term_id) == terms_in6.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgrecore& grecore) {
		os << rofcore::indent(0) << "<cgrecore dpid:" << (unsigned long long)grecore.dpid.get_uint64_t() << " "
				<< " 0x" << std::hex << &grecore << std::dec << " "
				<< "#in4-term(s): " << grecore.terms_in4.size() << " "
				<< "#in6-term(s): " << grecore.terms_in6.size() << " >" << std::endl;
		for (std::map<uint32_t, cgreterm_in4*>::const_iterator
				it = grecore.terms_in4.begin(); it != grecore.terms_in4.end(); ++it) {
			rofcore::indent i(2); os << *(it->second);
		}
		for (std::map<uint32_t, cgreterm_in6*>::const_iterator
				it = grecore.terms_in6.begin(); it != grecore.terms_in6.end(); ++it) {
			rofcore::indent i(2); os << *(it->second);
		}
		return os;
	};


private:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t							state;
	rofl::cdpid									dpid;
	uint8_t										eth_local_table_id;
	uint8_t										ip_local_table_id;
	uint8_t										gre_local_table_id;
	uint8_t										ip_fwd_table_id;
	std::map<uint32_t, cgreterm_in4*>			terms_in4;
	mutable rofl::PthreadRwLock					rwlock_in4;
	std::map<uint32_t, cgreterm_in6*>			terms_in6;
	mutable rofl::PthreadRwLock					rwlock_in6;
	static std::map<rofl::cdpid, cgrecore*>		grecores;

	static const uint8_t						GRE_IP_PROTO = 47;
	static const uint16_t 						GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING = 0x6558;
};

}; // end of namespace gre
}; // end of namespace roflibs

#endif /* CGRECORE_HPP_ */
