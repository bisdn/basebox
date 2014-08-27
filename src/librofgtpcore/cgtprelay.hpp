/*
 * cgtprelay.hpp
 *
 *  Created on: 21.08.2014
 *      Author: andreas
 */

#ifndef CGTPRELAY_HPP_
#define CGTPRELAY_HPP_

#include <map>
#include <iostream>
#include <exception>

#include <rofl/common/csocket.h>
#include <rofl/common/protocols/fgtpuframe.h>
#include <rofl/common/cdpid.h>

#include "clogging.hpp"
#include "caddress_gtp.hpp"
#include "ctundev.hpp"

namespace rofgtp {

class eGtpRelayBase : public std::runtime_error {
public:
	eGtpRelayBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGtpRelayNotFound : public eGtpRelayBase {
public:
	eGtpRelayNotFound(const std::string& __arg) : eGtpRelayBase(__arg) {};
};

class cgtprelay : public rofl::csocket_owner, public rofcore::cnetdev_owner {
public:

	/**
	 *
	 */
	static cgtprelay&
	add_gtp_relay(const rofl::cdpid& dpid) {
		if (cgtprelay::gtprelays.find(dpid) != cgtprelay::gtprelays.end()) {
			delete cgtprelay::gtprelays[dpid];
			cgtprelay::gtprelays.erase(dpid);
		}
		cgtprelay::gtprelays[dpid] = new cgtprelay(dpid);
		return *(cgtprelay::gtprelays[dpid]);
	};

	/**
	 *
	 */
	static cgtprelay&
	set_gtp_relay(const rofl::cdpid& dpid) {
		if (cgtprelay::gtprelays.find(dpid) == cgtprelay::gtprelays.end()) {
			cgtprelay::gtprelays[dpid] = new cgtprelay(dpid);
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
	cgtprelay(const rofl::cdpid& dpid) : dpid(dpid) {};

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
	rofcore::ctundev&
	add_tundev(const std::string& devname) {
		if (tundevs.find(devname) != tundevs.end()) {
			delete tundevs[devname];
			tundevs.erase(devname);
		}
		tundevs[devname] = new rofcore::ctundev(this, devname);
		return *(tundevs[devname]);
	};

	/**
	 *
	 */
	rofcore::ctundev&
	set_tundev(const std::string& devname) {
		if (tundevs.find(devname) == tundevs.end()) {
			tundevs[devname] = new rofcore::ctundev(this, devname);
		}
		return *(tundevs[devname]);
	};

	/**
	 *
	 */
	const rofcore::ctundev&
	get_tundev(const std::string& devname) const {
		if (tundevs.find(devname) == tundevs.end()) {
			throw rofcore::eTunDevNotFound();
		}
		return *(tundevs.at(devname));
	};

	/**
	 *
	 */
	void
	drop_tundev(const std::string& devname) {
		if (tundevs.find(devname) == tundevs.end()) {
			return;
		}
		delete tundevs[devname];
		tundevs.erase(devname);
	};

	/**
	 *
	 */
	bool
	has_tundev(const std::string& devname) {
		return (not (tundevs.find(devname) == tundevs.end()));
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
	enqueue(rofcore::cnetdev *netdev, rofl::cpacket* pkt) { /* TODO */ };

	/**
	 *
	 */
	virtual void
	enqueue(rofcore::cnetdev *netdev, std::vector<rofl::cpacket*> pkts) { /* TODO */ };


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

	rofl::cdpid 								dpid;
	std::map<caddress_gtp_in4, rofl::csocket*>	sockets_in4;
	std::map<caddress_gtp_in6, rofl::csocket*>	sockets_in6;
	std::map<std::string, rofcore::ctundev*>	tundevs;
	static std::map<rofl::cdpid, cgtprelay*>	gtprelays;
};

}; // end of namespace rofgtp

#endif /* CGTPRELAY_HPP_ */
