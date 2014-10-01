/*
 * cgreterm.hpp
 *
 *  Created on: 18.08.2014
 *      Author: andreas
 */

#ifndef CGRETERM_HPP_
#define CGRETERM_HPP_

#include <bitset>
#include <vector>
#include <string>
#include <ostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/openflow/cofmatch.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/protocols/fudpframe.h>
#include <rofl/common/openflow/experimental/actions/gre_actions.h>

#include <roflibs/netlink/clogging.hpp>


namespace roflibs {
namespace gre {

class eGreTermBase : public std::runtime_error {
public:
	eGreTermBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGreTermNotFound : public eGreTermBase {
public:
	eGreTermNotFound(const std::string& __arg) : eGreTermBase(__arg) {};
};


/*
 * -ingress- means: send traffic into the tunnel
 * -egress-  means: strip the tunnel and send traffic to the external world
 */
class cgreterm {
public:

	/**
	 *
	 */
	cgreterm() :
		state(STATE_DETACHED), eth_ofp_table_id(0), gre_ofp_table_id(0), ip_fwd_ofp_table_id(0),
		idle_timeout(DEFAULT_IDLE_TIMEOUT),
		gre_portno(0), gre_key(0) {};

	/**
	 *
	 */
	~cgreterm() {};

	/**
	 *
	 */
	cgreterm(const rofl::cdpid& dpid, uint8_t eth_ofp_table_id,
			uint8_t gre_ofp_table_id, uint8_t ip_fwd_ofp_table_id,
			uint32_t gre_portno, uint32_t gre_key) :
		state(STATE_DETACHED), dpid(dpid), eth_ofp_table_id(eth_ofp_table_id),
		gre_ofp_table_id(gre_ofp_table_id), ip_fwd_ofp_table_id(ip_fwd_ofp_table_id),
		idle_timeout(DEFAULT_IDLE_TIMEOUT),
		gre_portno(gre_portno), gre_key(gre_key) {};

	/**
	 *
	 */
	cgreterm(const cgreterm& greterm) { *this = greterm; };

	/**
	 *
	 */
	cgreterm&
	operator= (const cgreterm& greterm) {
		if (this == &greterm)
			return *this;
		state 				= greterm.state;
		dpid 				= greterm.dpid;
		eth_ofp_table_id 	= greterm.eth_ofp_table_id;
		gre_ofp_table_id 	= greterm.gre_ofp_table_id;
		ip_fwd_ofp_table_id	= greterm.ip_fwd_ofp_table_id;
		idle_timeout 		= greterm.idle_timeout;
		gre_portno 			= greterm.gre_portno;
		gre_key 			= greterm.gre_key;
		return *this;
	};

public:

	/**
	 *
	 */
	const uint32_t&
	get_gre_key() const { return gre_key; };

	/**
	 *
	 */
	const uint32_t&
	get_gre_portno() const { return gre_portno; };

protected:

	/**
	 *
	 */
	void
	execute(
			std::string const& executable,
			std::vector<std::string> argv,
			std::vector<std::string> envp);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgreterm& greterm) {
		os << rofcore::indent(0) << "<cgreterm key:0x"
				<< std::hex << (unsigned int)greterm.get_gre_key() << std::dec << " >" << std::endl;
		return os;
	};

protected:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};
	enum ofp_state_t 		state;

	enum cgreterm_flags_t {
		FLAG_INGRESS_FM_INSTALLED 	= (1 << 0),
		FLAG_EGRESS_FM_INSTALLED 	= (1 << 1),
	};
	std::bitset<32>			flags;

	rofl::cdpid 			dpid;
	uint8_t					eth_ofp_table_id;				// OFP tableid for capturing plain ethernet frames (table 0)
	uint8_t 				gre_ofp_table_id;				// OFP tableid for installing GRE-pop entries
	uint8_t					ip_fwd_ofp_table_id;			// OFP tableid for forwarding IP datagrams
	static const int 		DEFAULT_IDLE_TIMEOUT = 15; 		// seconds
	int 					idle_timeout;

	uint32_t				gre_portno; 					// portno of ethernet port
	uint32_t				gre_key;							// GRE key according to IETF RFC 2890
	static const uint8_t	GRE_IP_PROTO = 47;
	static const uint16_t 	GRE_PROT_TYPE_TRANSPARENT_ETHERNET_BRIDGING = 0x6558;

	static std::string		script_path_init;
	static std::string		script_path_term;
};



class cgreterm_in4 : public cgreterm {
public:

	/**
	 *
	 */
	cgreterm_in4() {};

	/**
	 *
	 */
	~cgreterm_in4() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		hook_term();
	};

	/**
	 *
	 */
	cgreterm_in4(const rofl::cdpid& dpid, uint8_t eth_ofp_table_id, uint8_t gre_ofp_table_id, uint8_t ip_fwd_ofp_table_id,
			const rofl::caddress_in4& laddr, const rofl::caddress_in4& raddr, uint32_t gre_portno, uint32_t gre_key) :
		cgreterm(dpid, eth_ofp_table_id, gre_ofp_table_id, ip_fwd_ofp_table_id, gre_portno, gre_key),
		laddr(laddr), raddr(raddr) {
		hook_init();
	};

	/**
	 *
	 */
	cgreterm_in4(const cgreterm_in4& greterm) { *this = greterm; };

	/**
	 *
	 */
	cgreterm_in4&
	operator= (const cgreterm_in4& greterm) {
		if (this == &greterm)
			return *this;
		cgreterm::operator= (greterm);
		laddr = greterm.laddr;
		raddr = greterm.raddr;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cgreterm_in4& greterm) const {
		return (raddr < greterm.raddr);
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_local_addr() const { return laddr; };

	/**
	 *
	 */
	const rofl::caddress_in4&
	get_remote_addr() const { return raddr; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt) {
		handle_dpt_open_egress(dpt);
		handle_dpt_open_ingress(dpt);
	};

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt) {
		handle_dpt_close_egress(dpt);
		handle_dpt_close_ingress(dpt);
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgreterm_in4& greterm) {
		os << dynamic_cast<const cgreterm&>( greterm );
		os << rofcore::indent(2) << "<cgreterm_in4 laddr:" << greterm.get_local_addr().str()
				<< " <==> raddr:" << greterm.get_remote_addr().str() << " >" << std::endl;
		return os;
	};

private:

	/**
	 *
	 */
	void
	handle_dpt_open_egress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close_egress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_open_ingress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close_ingress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	hook_init();

	/**
	 *
	 */
	void
	hook_term();

private:

	rofl::caddress_in4 laddr;
	rofl::caddress_in4 raddr;
};



class cgreterm_in6 : public cgreterm {
public:

	/**
	 *
	 */
	cgreterm_in6() {};

	/**
	 *
	 */
	~cgreterm_in6() {
		try {
			if (STATE_ATTACHED == state) {
				handle_dpt_close(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		hook_term();
	};

	/**
	 *
	 */
	cgreterm_in6(const rofl::cdpid& dpid, uint8_t eth_ofp_table_id, uint8_t gre_ofp_table_id, uint8_t ip_fwd_ofp_table_id,
			const rofl::caddress_in6& laddr, const rofl::caddress_in6& raddr, uint32_t gre_portno, uint32_t gre_key) :
		cgreterm(dpid, eth_ofp_table_id, gre_ofp_table_id, ip_fwd_ofp_table_id, gre_portno, gre_key),
		laddr(laddr), raddr(raddr) {
		hook_init();
	};


	/**
	 *
	 */
	cgreterm_in6(const cgreterm_in6& greterm) { *this = greterm; };

	/**
	 *
	 */
	cgreterm_in6&
	operator= (const cgreterm_in6& greterm) {
		if (this == &greterm)
			return *this;
		cgreterm::operator= (greterm);
		laddr = greterm.laddr;
		raddr = greterm.raddr;
		return *this;
	};

	/**
	 *
	 */
	bool
	operator< (const cgreterm_in6& greterm) const {
		return (raddr < greterm.raddr);
	};

public:

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_local_addr() const { return laddr; };

	/**
	 *
	 */
	const rofl::caddress_in6&
	get_remote_addr() const { return raddr; };

public:

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt) {
		handle_dpt_open_egress(dpt);
		handle_dpt_open_ingress(dpt);
	};

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt) {
		handle_dpt_close_egress(dpt);
		handle_dpt_close_ingress(dpt);
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgreterm_in6& greterm) {
		os << dynamic_cast<const cgreterm&>( greterm );
		os << rofcore::indent(2) << "<cgreterm_in6 laddr:" << greterm.get_local_addr().str()
				<< " <==> raddr:" << greterm.get_remote_addr().str() << " >" << std::endl;
		return os;
	};

private:

	/**
	 *
	 */
	void
	handle_dpt_open_egress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close_egress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_open_ingress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	handle_dpt_close_ingress(rofl::crofdpt& dpt);

	/**
	 *
	 */
	void
	hook_init();

	/**
	 *
	 */
	void
	hook_term();

private:

	rofl::caddress_in6 laddr;
	rofl::caddress_in6 raddr;
};

}; // end of namespace gre
}; // end of namespace roflibs

#endif /* CGRETERM_HPP_ */
