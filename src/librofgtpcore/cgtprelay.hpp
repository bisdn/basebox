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

#include "clogging.h"
#include "caddress_gtp.hpp"

namespace rofgtp {

class eGtpRelayBase : public std::runtime_error {
public:
	eGtpRelayBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eGtpRelayNotFound : public eGtpRelayBase {
public:
	eGtpRelayNotFound(const std::string& __arg) : eGtpRelayBase(__arg) {};
};

class cgtprelay : public rofl::csocket_owner {
public:

	/**
	 *
	 */
	cgtprelay() : mem((size_t)1526) {};

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
	add_socket(const caddress_gtp_in4& bindaddr) {
		if (sockets_in4.find(bindaddr) != sockets_in4.end()) {
			drop_socket(bindaddr);
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
	set_socket(const caddress_gtp_in4& bindaddr) {
		if (sockets_in4.find(bindaddr) == sockets_in4.end()) {
			throw eGtpRelayNotFound("cgtprelay::set_socket() bindaddr_in4 not found");
		}
		return *(sockets_in4[bindaddr]);
	};

	/**
	 *
	 */
	const rofl::csocket&
	get_socket(const caddress_gtp_in4& bindaddr) {
		if (sockets_in4.find(bindaddr) == sockets_in4.end()) {
			throw eGtpRelayNotFound("cgtprelay::get_socket() bindaddr_in4 not found");
		}
		return *(sockets_in4.at(bindaddr));
	};

	/**
	 *
	 */
	void
	drop_socket(const caddress_gtp_in4& bindaddr) {
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
	has_socket(const caddress_gtp_in4& bindaddr) {
		return (not (sockets_in4.find(bindaddr) == sockets_in4.end()));
	};


	/**
	 *
	 */
	void
	add_socket(const caddress_gtp_in6& bindaddr) {
		if (sockets_in6.find(bindaddr) != sockets_in6.end()) {
			drop_socket(bindaddr);
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
	set_socket(const caddress_gtp_in6& bindaddr) {
		if (sockets_in6.find(bindaddr) == sockets_in6.end()) {
			throw eGtpRelayNotFound("cgtprelay::set_socket() bindaddr_in6 not found");
		}
		return *(sockets_in6[bindaddr]);
	};

	/**
	 *
	 */
	const rofl::csocket&
	get_socket(const caddress_gtp_in6& bindaddr) {
		if (sockets_in6.find(bindaddr) == sockets_in6.end()) {
			throw eGtpRelayNotFound("cgtprelay::get_socket() bindaddr_in6 not found");
		}
		return *(sockets_in6.at(bindaddr));
	};

	/**
	 *
	 */
	void
	drop_socket(const caddress_gtp_in6& bindaddr) {
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
	has_socket(const caddress_gtp_in6& bindaddr) {
		return (not (sockets_in6.find(bindaddr) == sockets_in6.end()));
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

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cgtprelay& relay) {

		return os;
	};

private:

	std::map<caddress_gtp_in4, rofl::csocket*>	sockets_in4;
	std::map<caddress_gtp_in6, rofl::csocket*>	sockets_in6;

	rofl::cmemory mem;
};

}; // end of namespace rofgtp

#endif /* CGTPRELAY_HPP_ */
