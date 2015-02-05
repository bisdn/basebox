/*
 * cgtpcoredb.hpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#ifndef CGTPDB_HPP_
#define CGTPDB_HPP_

#include <inttypes.h>
#include <exception>
#include <ostream>
#include <string>
#include <set>
#include <map>
#include <rofl/common/cdpid.h>
#include <rofl/common/caddress.h>
#include <roflibs/netlink/clogging.hpp>

namespace roflibs {
namespace gtp {


class eGtpDBBase : public std::runtime_error {
public:
	eGtpDBBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGtpDBNotFound : public eGtpDBBase {
public:
	eGtpDBNotFound(const std::string& __arg) : eGtpDBBase(__arg) {};
};



class cgtplabelentry {
public:

	/**
	 *
	 */
	~cgtplabelentry()
	{};

	/**
	 *
	 */
	cgtplabelentry() :
		vers(0),
		sport(0),
		dport(0),
		teid(0)
	{};

	/**
	 *
	 */
	cgtplabelentry(
			int version,
			const std::string& saddr, uint16_t sport,
			const std::string& daddr, uint16_t dport,
			uint32_t teid) :
				vers(version),
				saddr(saddr),
				sport(sport),
				daddr(daddr),
				dport(dport),
				teid(teid)
	{};

	/**
	 *
	 */
	cgtplabelentry(
			const cgtplabelentry& port)
	{ *this = port; };

	/**
	 *
	 */
	cgtplabelentry&
	operator= (
			const cgtplabelentry& entry) {
		if (this == &entry)
			return *this;
		vers    = entry.vers;
		saddr   = entry.saddr;
		sport   = entry.sport;
		daddr   = entry.daddr;
		dport   = entry.dport;
		teid    = entry.teid;
		info_s  = entry.info_s;
		return *this;
	};

public:

	void
	set_version(
			int vers)
	{ this->vers = vers; };

	int&
	set_version()
	{ return vers; };

	void
	set_src_addr(
			const std::string& addr)
	{ this->saddr = addr; };

	void
	set_src_port(
			uint16_t port)
	{ this->sport = port; };

	void
	set_dst_addr(
			const std::string& addr)
	{ this->daddr = addr; };

	void
	set_dst_port(
			uint16_t port)
	{ this->dport = port; };

	void
	set_teid(
			uint32_t teid)
	{ this->teid = teid; };


public:

	int
	get_version() const
	{ return vers; };

	const std::string&
	get_src_addr() const
	{ return saddr; };

	uint16_t
	get_src_port() const
	{ return sport; };

	const std::string&
	get_dst_addr() const
	{ return daddr; };

	uint16_t
	get_dst_port() const
	{ return dport; };

	uint32_t
	get_teid() const
	{ return teid; };


public:

	friend std::ostream&
	operator<< (
			std::ostream& os, const cgtplabelentry& entry) {
		os << rofcore::indent(0) << "<cgtplabelentry ";
		os << "GTP label: [IPv" << entry.get_version() << " ";
		os << "teid: " << (unsigned int)entry.get_teid() << " ";
		os << "src: " << entry.get_src_addr() << ":" << (unsigned int)entry.get_src_port() << " ";
		os << "dst: " << entry.get_dst_addr() << ":" << (unsigned int)entry.get_dst_port() << " ";
		os << "] >" << std::endl;
		return os;
	};

	const std::string&
	str() const {
		std::stringstream ss;
		ss << "IPv" << get_version() << ", ";
		ss << "teid: " << (unsigned int)get_teid() << ", ";
		ss << "src: " << get_src_addr() << ":" << (unsigned int)get_src_port() << ", ";
		ss << "dst: " << get_dst_addr() << ":" << (unsigned int)get_dst_port();
		info_s = ss.str();
		return info_s;
	};

private:

	// GTP label
	int					vers;	// IP version (4,6,...)
	std::string			saddr; 	// src addr
	uint16_t			sport; 	// src port
	std::string			daddr; 	// dst addr
	uint16_t			dport; 	// dst port
	uint32_t			teid; 	// TEID

	mutable std::string	info_s;
};



class cgtpinjectentry {
public:

	/**
	 *
	 */
	~cgtpinjectentry()
	{};

	/**
	 *
	 */
	cgtpinjectentry() :
		version(0)
	{};

	/**
	 *
	 */
	cgtpinjectentry(
			int version,
			const std::string& saddr, const std::string& smask,
			const std::string& daddr, const std::string& dmask) :
				version(version),
				saddr(saddr),
				smask(smask),
				daddr(daddr),
				dmask(dmask)
	{};

	/**
	 *
	 */
	cgtpinjectentry(
			const cgtpinjectentry& entry)
	{ *this = entry; };

	/**
	 *
	 */
	cgtpinjectentry&
	operator=
			(const cgtpinjectentry& entry) {
		if (this == &entry)
			return *this;
		version	= entry.version;
		saddr	= entry.saddr;
		smask	= entry.smask;
		daddr	= entry.daddr;
		dmask	= entry.dmask;
		info_s	= entry.info_s;
		return *this;
	};

public:

	void
	set_version(
			int version)
	{ this->version = version; };

	void
	set_src_addr(
			const std::string& saddr)
	{ this->saddr = saddr; };

	void
	set_src_mask(
			const std::string& smask)
	{ this->smask = smask; };

	void
	set_dst_addr(
			const std::string& daddr)
	{ this->daddr = daddr; };

	void
	set_dst_mask(
			const std::string& dmask)
	{ this->dmask = dmask; };

public:

	int
	get_version() const
	{ return version; };

	const std::string&
	get_src_addr() const
	{ return saddr; };

	const std::string&
	get_src_mask() const
	{ return smask; };

	const std::string&
	get_dst_addr() const
	{ return daddr; };

	const std::string&
	get_dst_mask() const
	{ return dmask; };

public:

	friend std::ostream&
	operator<< (
			std::ostream& os, const cgtpinjectentry& entry) {
		os << rofcore::indent(0) << "<cgtpinjectentry IPv" << entry.get_version() << ", ";
		os << "saddr: " << entry.get_src_addr() << "/" << entry.get_src_mask() << ", ";
		os << "daddr: " << entry.get_dst_addr() << "/" << entry.get_dst_mask() << " ";
		os << " >" << std::endl;
		return os;
	};

	const std::string&
	str() const {
		std::stringstream ss;
		ss << "IPv" << get_version() << ", ";
		ss << "saddr: " << get_src_addr() << "/" << get_src_mask() << ", ";
		ss << "daddr: " << get_dst_addr() << "/" << get_dst_mask();
		info_s = ss.str();
		return info_s;
	};

private:

	int 		version;	// IP version (4,6,...)
	std::string saddr;		// src addr
	std::string smask;		// src mask
	std::string daddr;		// dst addr
	std::string dmask;		// dst mask
	mutable std::string info_s;
};




class cgtprelayentry {
public:

	/**
	 *
	 */
	~cgtprelayentry()
	{};

	/**
	 *
	 */
	cgtprelayentry() :
		rid(0)
	{};

	/**
	 *
	 */
	cgtprelayentry(
			unsigned int rid,
			const cgtplabelentry& inc_label,
			const cgtplabelentry& out_label) :
				rid(rid),
				inc_label(inc_label),
				out_label(out_label)
	{};

	/**
	 *
	 */
	cgtprelayentry(
			const cgtprelayentry& port)
	{ *this = port; };

	/**
	 *
	 */
	cgtprelayentry&
	operator= (
			const cgtprelayentry& entry) {
		if (this == &entry)
			return *this;
		rid = entry.rid;
		inc_label = entry.inc_label;
		out_label = entry.out_label;
		return *this;
	};

public:

	void
	set_relay_id(
			unsigned int rid)
	{ this->rid = rid; };

	void
	set_incoming_label(
			const cgtplabelentry& inc_label)
	{ this->inc_label = inc_label; };

	cgtplabelentry&
	set_incoming_label()
	{ return inc_label; };

	void
	set_outgoing_label(
			const cgtplabelentry& out_label)
	{ this->out_label = out_label; };

	cgtplabelentry&
	set_outgoing_label()
	{ return out_label; };

public:

	unsigned int
	get_relay_id() const
	{ return rid; };

	const cgtplabelentry&
	get_incoming_label() const
	{ return inc_label; };

	const cgtplabelentry&
	get_outgoing_label() const
	{ return out_label; };

public:

	friend std::ostream&
	operator<< (
			std::ostream& os, const cgtprelayentry& entry) {
		os << rofcore::indent(0) << "<cgtprelayentry relay-id: " << entry.get_relay_id() << " ";
		os << "incoming GTP label: " << entry.get_incoming_label().str() << " ";
		os << "outgoing GTP label: " << entry.get_outgoing_label().str() << " >" << std::endl;
		return os;
	};

	const std::string&
	str() const {
		std::stringstream ss;
		ss << "relay-id: " << get_relay_id() << ", ";
		ss << "incoming GTP label: " << get_incoming_label().str() << ", ";
		ss << "outgoing GTP label: " << get_outgoing_label().str();
		info_s = ss.str();
		return info_s;
	};

private:

	unsigned int		rid;		// relay identifier
	cgtplabelentry		inc_label;	// incoming GTP label
	cgtplabelentry		out_label;	// outgoing GTP label
	mutable std::string	info_s;
};




class cgtptermentry {
public:

	/**
	 *
	 */
	~cgtptermentry()
	{};

	/**
	 *
	 */
	cgtptermentry() :
		tid(0)
	{};

	/**
	 *
	 */
	cgtptermentry(
			unsigned int tid,
			const cgtplabelentry& ingress_label,
			const cgtplabelentry& egress_label,
			const cgtpinjectentry& inject_filter) :
				tid(tid),
				ingress_label(ingress_label),
				egress_label(egress_label),
				inject_filter(inject_filter)
	{};

	/**
	 *
	 */
	cgtptermentry(
			const cgtptermentry& port)
	{ *this = port; };

	/**
	 *
	 */
	cgtptermentry&
	operator= (
			const cgtptermentry& entry) {
		if (this == &entry)
			return *this;
		tid 			= entry.tid;
		ingress_label 	= entry.ingress_label;
		egress_label 	= entry.egress_label;
		inject_filter 	= entry.inject_filter;
		return *this;
	};

public:

	void
	set_term_id(
			unsigned int tid)
	{ this->tid = tid; };

	void
	set_ingress_label(
			const cgtplabelentry& ingress_label)
	{ this->ingress_label = ingress_label; };

	cgtplabelentry&
	set_ingress_label()
	{ return ingress_label; };

	void
	set_egress_label(
			const cgtplabelentry& egress_label)
	{ this->egress_label = egress_label; };

	cgtplabelentry&
	set_egress_label()
	{ return egress_label; };

	void
	set_inject_filter(
			const cgtpinjectentry& inject_filter)
	{ this->inject_filter = inject_filter; };

	cgtpinjectentry&
	set_inject_filter()
	{ return inject_filter; };

public:

	unsigned int
	get_term_id() const
	{ return tid; };

	const cgtplabelentry&
	get_ingress_label() const
	{ return ingress_label; };

	const cgtplabelentry&
	get_egress_label() const
	{ return egress_label; };

	const cgtpinjectentry&
	get_inject_filter() const
	{ return inject_filter; };

public:

	friend std::ostream&
	operator<< (
			std::ostream& os, const cgtptermentry& entry) {
		os << rofcore::indent(0) << "<cgtptermentry relay-id: " << entry.get_term_id() << " ";
		os << "ingress GTP label: " << entry.get_ingress_label().str() << " ";
		os << "egress GTP label: " << entry.get_egress_label().str() << " ";
		os << "inject filter: " << entry.get_inject_filter().str() << " >" << std::endl;
		return os;
	};

	const std::string&
	str() const {
		std::stringstream ss;
		ss << "term-id: " << get_term_id() << ", ";
		ss << "ingress GTP label: " << get_ingress_label().str() << ", ";
		ss << "egress GTP label: " << get_egress_label().str() << ", ";
		ss << "inject filter: " << get_inject_filter().str();
		info_s = ss.str();
		return info_s;
	};

private:

	unsigned int		tid;			// termination point identifier
	cgtplabelentry		ingress_label;	// ingress GTP label
	cgtplabelentry		egress_label;	// egress GTP label
	cgtpinjectentry		inject_filter;	// inject traffic matching this filter into GTP tunnel
	mutable std::string	info_s;
};





class cgtpcoredb {
public:

	/**
	 *
	 */
	static cgtpcoredb&
	get_gtpcoredb(const std::string& name);

	/**
	 *
	 */
	virtual
	~cgtpcoredb() {};

public:

	/**
	 *
	 */
	const std::set<unsigned int>&
	get_gtp_relay_ids(const rofl::cdpid& dpid) const {
		return relayids[dpid];
	};

	/**
	 *
	 */
	cgtprelayentry&
	add_gtp_relay(
			const rofl::cdpid& dpid,
			const cgtprelayentry& entry) {
		if (relayentries[dpid].find(entry.get_relay_id()) != relayentries[dpid].end()) {
			relayentries[dpid].erase(entry.get_relay_id());
		}
		relayids[dpid].insert(entry.get_relay_id());
		return (relayentries[dpid][entry.get_relay_id()] = entry);
	};

	/**
	 *
	 */
	cgtprelayentry&
	set_gtp_relay(
			const rofl::cdpid& dpid,
			const cgtprelayentry& entry) {
		if (relayentries[dpid].find(entry.get_relay_id()) == relayentries[dpid].end()) {
			relayids[dpid].insert(entry.get_relay_id());
			relayentries[dpid][entry.get_relay_id()] = entry;
		}
		return relayentries[dpid][entry.get_relay_id()];
	};

	/**
	 *
	 */
	cgtprelayentry&
	set_gtp_relay(
			const rofl::cdpid& dpid,
			unsigned int relay_id) {
		if (relayentries[dpid].find(relay_id) == relayentries[dpid].end()) {
			throw eGtpDBNotFound("cgtpcoredb::set_gtp_relay() relay_id not found");
		}
		return relayentries[dpid][relay_id];
	};

	/**
	 *
	 */
	const cgtprelayentry&
	get_gtp_relay(
			const rofl::cdpid& dpid,
			unsigned int relay_id) const {
		if (relayentries.at(dpid).find(relay_id) == relayentries.at(dpid).end()) {
			throw eGtpDBNotFound("cgtpcoredb::get_gtp_relay() relay_id not found");
		}
		return relayentries.at(dpid).at(relay_id);
	};

	/**
	 *
	 */
	void
	drop_gtp_relay(
			const rofl::cdpid& dpid,
			unsigned int relay_id) {
		if (relayentries.at(dpid).find(relay_id) == relayentries.at(dpid).end()) {
			return;
		}
		relayentries[dpid].erase(relay_id);
		relayids[dpid].erase(relay_id);
	};

	/**
	 *
	 */
	bool
	has_gtp_relay(
			const rofl::cdpid& dpid,
			unsigned int relay_id) const {
		if (relayentries.find(dpid) == relayentries.end()) {
			return false;
		}
		return (not (relayentries.at(dpid).find(relay_id) == relayentries.at(dpid).end()));
	};

public:

	/**
	 *
	 */
	const std::set<unsigned int>&
	get_gtp_term_ids(const rofl::cdpid& dpid) const {
		return termids[dpid];
	};

	/**
	 *
	 */
	cgtptermentry&
	add_gtp_term(
			const rofl::cdpid& dpid,
			const cgtptermentry& entry) {
		if (termentries[dpid].find(entry.get_term_id()) != termentries[dpid].end()) {
			termentries[dpid].erase(entry.get_term_id());
		}
		termids[dpid].insert(entry.get_term_id());
		return (termentries[dpid][entry.get_term_id()] = entry);
	};

	/**
	 *
	 */
	cgtptermentry&
	set_gtp_term(
			const rofl::cdpid& dpid,
			const cgtptermentry& entry) {
		if (termentries[dpid].find(entry.get_term_id()) == termentries[dpid].end()) {
			termids[dpid].insert(entry.get_term_id());
			termentries[dpid][entry.get_term_id()] = entry;
		}
		return termentries[dpid][entry.get_term_id()];
	};

	/**
	 *
	 */
	cgtptermentry&
	set_gtp_term(
			const rofl::cdpid& dpid,
			unsigned int term_id) {
		if (termentries[dpid].find(term_id) == termentries[dpid].end()) {
			throw eGtpDBNotFound("cgtpcoredb::set_gtp_term() term_id not found");
		}
		return termentries[dpid][term_id];
	};

	/**
	 *
	 */
	const cgtptermentry&
	get_gtp_term(
			const rofl::cdpid& dpid,
			unsigned int term_id) const {
		if (termentries.at(dpid).find(term_id) == termentries.at(dpid).end()) {
			throw eGtpDBNotFound("cgtpcoredb::get_gtp_term() term_id not found");
		}
		return termentries.at(dpid).at(term_id);
	};

	/**
	 *
	 */
	void
	drop_gtp_term(
			const rofl::cdpid& dpid,
			unsigned int term_id) {
		if (termentries.at(dpid).find(term_id) == termentries.at(dpid).end()) {
			return;
		}
		termentries[dpid].erase(term_id);
		termids[dpid].erase(term_id);
	};

	/**
	 *
	 */
	bool
	has_gtp_term(
			const rofl::cdpid& dpid,
			unsigned int term_id) const {
		if (termentries.find(dpid) == termentries.end()) {
			return false;
		}
		return (not (termentries.at(dpid).find(term_id) == termentries.at(dpid).end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgtpcoredb& db) {
		os << rofcore::indent(0) << "<cgtpcoredb >" << std::endl;
		rofcore::indent i(2);
		for (std::map<rofl::cdpid, std::map<unsigned int, cgtprelayentry> >::const_iterator
				it = db.relayentries.begin(); it != db.relayentries.end(); ++it) {
			os << rofcore::indent(0) << "<cdpid: " << it->first.str() << " >" << std::endl;
			rofcore::indent j(2);
			for (std::map<unsigned int, cgtprelayentry>::const_iterator
					jt = it->second.begin(); jt != it->second.end(); ++jt) {
				os << jt->second;
			}
		}
		for (std::map<rofl::cdpid, std::map<unsigned int, cgtptermentry> >::const_iterator
				it = db.termentries.begin(); it != db.termentries.end(); ++it) {
			os << rofcore::indent(0) << "<cdpid: " << it->first.str() << " >" << std::endl;
			rofcore::indent j(2);
			for (std::map<unsigned int, cgtptermentry>::const_iterator
					jt = it->second.begin(); jt != it->second.end(); ++jt) {
				os << jt->second;
			}
		}
		return os;
	};

private:


	std::map<rofl::cdpid, std::map<unsigned int, cgtprelayentry> >	relayentries;
	mutable std::map<rofl::cdpid, std::set<unsigned int> >			relayids;
	std::map<rofl::cdpid, std::map<unsigned int, cgtptermentry> >	termentries;
	mutable std::map<rofl::cdpid, std::set<unsigned int> >			termids;
	static std::map<std::string, cgtpcoredb*> 						gtpcoredbs;
};

}; // end of namespace gtp
}; // end of namespace roflibs

#endif /* CPORTCONF_HPP_ */
