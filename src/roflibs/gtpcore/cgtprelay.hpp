/*
 * cgtprelay.hpp
 *
 *  Created on: 21.08.2014
 *      Author: andreas
 */

#ifndef CGTPRELAY_HPP_
#define CGTPRELAY_HPP_

#include <inttypes.h>

#include <map>
#include <iostream>
#include <exception>

#include <rofl/common/csocket.h>
#include <rofl/common/protocols/fgtpuframe.h>
#include <rofl/common/protocols/fipv4frame.h>
#include <rofl/common/protocols/fipv6frame.h>
#include <rofl/common/cdpid.h>

#include <roflibs/netlink/clogging.hpp>
#include <roflibs/gtpcore/caddress_gtp.hpp>
#include <roflibs/gtpcore/ctermdev.hpp>
#include <roflibs/ipcore/cipcore.hpp>

namespace roflibs {
namespace gtp {

class eGtpRelayBase : public std::runtime_error {
public:
	eGtpRelayBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGtpRelayNotFound : public eGtpRelayBase {
public:
	eGtpRelayNotFound(const std::string& __arg) : eGtpRelayBase(__arg) {};
};

class cgtprelay : public rofl::csocket_owner, public rofcore::cnetdev_owner, public rofcore::cnetlink_common_observer {
public:

	/**
	 *
	 */
	static cgtprelay&
	add_gtp_relay(const rofl::cdpid& dpid, uint8_t ofp_table_id) {
		if (cgtprelay::gtprelays.find(dpid) != cgtprelay::gtprelays.end()) {
			delete cgtprelay::gtprelays[dpid];
			cgtprelay::gtprelays.erase(dpid);
		}
		cgtprelay::gtprelays[dpid] = new cgtprelay(dpid, ofp_table_id);
		return *(cgtprelay::gtprelays[dpid]);
	};

	/**
	 *
	 */
	static cgtprelay&
	set_gtp_relay(const rofl::cdpid& dpid, uint8_t ofp_table_id) {
		if (cgtprelay::gtprelays.find(dpid) == cgtprelay::gtprelays.end()) {
			cgtprelay::gtprelays[dpid] = new cgtprelay(dpid, ofp_table_id);
		}
		return *(cgtprelay::gtprelays[dpid]);
	};

	/**
	 *
	 */
	static cgtprelay&
	set_gtp_relay(const rofl::cdpid& dpid) {
		if (cgtprelay::gtprelays.find(dpid) == cgtprelay::gtprelays.end()) {
			throw eGtpRelayNotFound("cgtprelay::set_gtp_relay() dpt not found");
		}
		return *(cgtprelay::gtprelays[dpid]);
	};

	/**
	 *
	 */
	static const cgtprelay&
	get_gtp_relay(const rofl::cdpid& dpid) {
		if (cgtprelay::gtprelays.find(dpid) == cgtprelay::gtprelays.end()) {
			throw eGtpRelayNotFound("cgtprelay::get_gtp_relay() dpt not found");
		}
		return *(cgtprelay::gtprelays.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_gtp_relay(const rofl::cdpid& dpid) {
		if (cgtprelay::gtprelays.find(dpid) == cgtprelay::gtprelays.end()) {
			return;
		}
		delete cgtprelay::gtprelays[dpid];
		cgtprelay::gtprelays.erase(dpid);
	}

	/**
	 *
	 */
	static bool
	has_gtp_relay(const rofl::cdpid& dpid) {
		return (not (cgtprelay::gtprelays.find(dpid) == cgtprelay::gtprelays.end()));
	};

private:

	/**
	 *
	 */
	cgtprelay(const rofl::cdpid& dpid, uint8_t ofp_table_id) :
		state(STATE_DETACHED), dpid(dpid), ofp_table_id(ofp_table_id) {};

	/**
	 *
	 */
	virtual
	~cgtprelay() {};

public:

	/**
	 *
	 */
	void
	add_socket_in4(const caddress_gtp_in4& bindaddr) {
		if (sockets_in4.find(bindaddr) != sockets_in4.end()) {
			drop_socket_in4(bindaddr);
		}
		sockets_in4[bindaddr] = rofl::csocket::csocket_factory(rofl::csocket::SOCKET_TYPE_PLAIN, this);
		rofl::cparams sockparams = rofl::csocket::get_default_params(rofl::csocket::SOCKET_TYPE_PLAIN);
		sockparams.set_param(rofl::csocket::PARAM_KEY_DOMAIN).set_string("inet");
		sockparams.set_param(rofl::csocket::PARAM_KEY_TYPE).set_string("dgram");
		sockparams.set_param(rofl::csocket::PARAM_KEY_PROTOCOL).set_string("udp");
		sockparams.set_param(rofl::csocket::PARAM_KEY_LOCAL_HOSTNAME).set_string(bindaddr.get_addr().str());
		sockparams.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(bindaddr.get_port().str());
		sockets_in4[bindaddr]->listen(sockparams);
	};

	/**
	 *
	 */
	rofl::csocket&
	set_socket_in4(const caddress_gtp_in4& bindaddr) {
		if (sockets_in4.find(bindaddr) == sockets_in4.end()) {
			throw eGtpRelayNotFound("cgtprelay::set_socket() bindaddr_in4 not found");
		}
		return *(sockets_in4[bindaddr]);
	};

	/**
	 *
	 */
	const rofl::csocket&
	get_socket_in4(const caddress_gtp_in4& bindaddr) {
		if (sockets_in4.find(bindaddr) == sockets_in4.end()) {
			throw eGtpRelayNotFound("cgtprelay::get_socket() bindaddr_in4 not found");
		}
		return *(sockets_in4.at(bindaddr));
	};

	/**
	 *
	 */
	void
	drop_socket_in4(const caddress_gtp_in4& bindaddr) {
		if (sockets_in4.find(bindaddr) == sockets_in4.end()) {
			return;
		}
		sockets_in4[bindaddr]->close();
		delete sockets_in4[bindaddr];
		sockets_in4.erase(bindaddr);
	};

	/**
	 *
	 */
	bool
	has_socket_in4(const caddress_gtp_in4& bindaddr) {
		return (not (sockets_in4.find(bindaddr) == sockets_in4.end()));
	};

public:

	/**
	 *
	 */
	void
	add_socket_in6(const caddress_gtp_in6& bindaddr) {
		if (sockets_in6.find(bindaddr) != sockets_in6.end()) {
			drop_socket_in6(bindaddr);
		}
		sockets_in6[bindaddr] = rofl::csocket::csocket_factory(rofl::csocket::SOCKET_TYPE_PLAIN, this);
		rofl::cparams sockparams = rofl::csocket::get_default_params(rofl::csocket::SOCKET_TYPE_PLAIN);
		sockparams.set_param(rofl::csocket::PARAM_KEY_DOMAIN).set_string("inet-any");
		sockparams.set_param(rofl::csocket::PARAM_KEY_TYPE).set_string("dgram");
		sockparams.set_param(rofl::csocket::PARAM_KEY_PROTOCOL).set_string("udp");
		sockparams.set_param(rofl::csocket::PARAM_KEY_LOCAL_HOSTNAME).set_string(bindaddr.get_addr().str());
		sockparams.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(bindaddr.get_port().str());
		sockets_in6[bindaddr]->listen(sockparams);
	};

	/**
	 *
	 */
	rofl::csocket&
	set_socket_in6(const caddress_gtp_in6& bindaddr) {
		if (sockets_in6.find(bindaddr) == sockets_in6.end()) {
			throw eGtpRelayNotFound("cgtprelay::set_socket() bindaddr_in6 not found");
		}
		return *(sockets_in6[bindaddr]);
	};

	/**
	 *
	 */
	const rofl::csocket&
	get_socket_in6(const caddress_gtp_in6& bindaddr) {
		if (sockets_in6.find(bindaddr) == sockets_in6.end()) {
			throw eGtpRelayNotFound("cgtprelay::get_socket() bindaddr_in6 not found");
		}
		return *(sockets_in6.at(bindaddr));
	};

	/**
	 *
	 */
	void
	drop_socket_in6(const caddress_gtp_in6& bindaddr) {
		if (sockets_in6.find(bindaddr) == sockets_in6.end()) {
			return;
		}
		sockets_in6[bindaddr]->close();
		delete sockets_in6[bindaddr];
		sockets_in6.erase(bindaddr);
	};

	/**
	 *
	 */
	bool
	has_socket_in6(const caddress_gtp_in6& bindaddr) {
		return (not (sockets_in6.find(bindaddr) == sockets_in6.end()));
	};

public:

	/**
	 *
	 */
	ctermdev&
	add_termdev(const std::string& devname) {
		if (termdevs.find(devname) != termdevs.end()) {
			delete termdevs[devname];
			termdevs.erase(devname);
		}
		termdevs[devname] = new ctermdev(this, devname, dpid, ofp_table_id);
		try {
			if (STATE_ATTACHED == state) {
				termdevs[devname]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return *(termdevs[devname]);
	};

	/**
	 *
	 */
	ctermdev&
	set_termdev(const std::string& devname) {
		if (termdevs.find(devname) == termdevs.end()) {
			termdevs[devname] = new ctermdev(this, devname, dpid, ofp_table_id);
			try {
				if (STATE_ATTACHED == state) {
					termdevs[devname]->handle_dpt_open(rofl::crofdpt::get_dpt(dpid));
				}
			} catch (rofl::eRofDptNotFound& e) {};
		}
		return *(termdevs[devname]);
	};

	/**
	 *
	 */
	const ctermdev&
	get_termdev(const std::string& devname) const {
		if (termdevs.find(devname) == termdevs.end()) {
			throw eTermDevNotFound("cgtprelay::get_termdev() devname not found");
		}
		return *(termdevs.at(devname));
	};

	/**
	 *
	 */
	void
	drop_termdev(const std::string& devname) {
		if (termdevs.find(devname) == termdevs.end()) {
			return;
		}
		delete termdevs[devname];
		termdevs.erase(devname);
	};

	/**
	 *
	 */
	bool
	has_termdev(const std::string& devname) {
		return (not (termdevs.find(devname) == termdevs.end()));
	};

public:

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt) {
		state = STATE_ATTACHED;
		for (std::map<std::string, ctermdev*>::iterator
				it = termdevs.begin(); it != termdevs.end(); ++it) {
			it->second->handle_dpt_open(dpt);
		}
	};

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt) {
		state = STATE_DETACHED;
		for (std::map<std::string, ctermdev*>::iterator
				it = termdevs.begin(); it != termdevs.end(); ++it) {
			it->second->handle_dpt_close(dpt);
		}
	};

private:

	friend class rofl::csocket;

	virtual void
	handle_listen(
			rofl::csocket& socket, int newsd) {};

	/**
	 *
	 */
	virtual void
	handle_accepted(
			rofl::csocket& socket) {};

	/**
	 *
	 */
	virtual void
	handle_accept_refused(rofl::csocket& socket) {};

	/**
	 *
	 */
	virtual void
	handle_connected(rofl::csocket& socket) {};

	/**
	 *
	 */
	virtual void
	handle_connect_refused(rofl::csocket& socket) {};

	/**
	 *
	 */
	virtual void
	handle_connect_failed(rofl::csocket& socket) {};

	/**
	 *
	 */
	virtual void
	handle_read(rofl::csocket& socket);

	/**
	 *
	 */
	virtual void
	handle_write(rofl::csocket& socket) {};

	/**
	 *
	 */
	virtual void
	handle_closed(rofl::csocket& socket) {};

private:

	friend class rofcore::cnetdev;

	/**
	 *
	 */
	virtual void
	enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt);

	/**
	 *
	 */
	virtual void
	enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts);

	/**
	 *
	 */
	void
	enqueue_in4(rofcore::cnetdev *netdev, rofl::cpacket* pkt);

	/**
	 *
	 */
	void
	enqueue_in6(rofcore::cnetdev *netdev, rofl::cpacket* pkt);

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgtprelay& relay) {
		os << rofcore::indent(0) << "<cgtprelay "
				<< " #in4: " << (int)relay.sockets_in4.size() << " "
				<< " #in6: " << (int)relay.sockets_in6.size() << " >" << std::endl;
		for (std::map<caddress_gtp_in4, rofl::csocket*>::const_iterator
				it = relay.sockets_in4.begin(); it != relay.sockets_in4.end(); ++it) {
			rofcore::indent i(2); os << rofl::indent(0) << *(it->second);
		}
		for (std::map<caddress_gtp_in6, rofl::csocket*>::const_iterator
				it = relay.sockets_in6.begin(); it != relay.sockets_in6.end(); ++it) {
			rofcore::indent i(2); os << rofl::indent(0) << *(it->second);
		}
		return os;
	};

private:

	static const int IP_VERSION_4 = 4;
	static const int IP_VERSION_6 = 6;

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t							state;
	rofl::cdpid 								dpid;
	uint8_t										ofp_table_id;
	std::map<caddress_gtp_in4, rofl::csocket*>	sockets_in4;
	std::map<caddress_gtp_in6, rofl::csocket*>	sockets_in6;
	std::map<std::string, ctermdev*>			termdevs;
	static std::map<rofl::cdpid, cgtprelay*>	gtprelays;

private:

	friend class cnetlink;

	virtual void
	addr_in4_created(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in4_updated(unsigned int ifindex, uint16_t adindex) {};

	virtual void
	addr_in4_deleted(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in6_created(unsigned int ifindex, uint16_t adindex);

	virtual void
	addr_in6_updated(unsigned int ifindex, uint16_t adindex) {};

	virtual void
	addr_in6_deleted(unsigned int ifindex, uint16_t adindex);

};

}; // end of namespace gtp
}; // end of namespace roflibs

#endif /* CGTPRELAY_HPP_ */
