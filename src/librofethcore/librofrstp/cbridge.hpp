/*
 * cbridge.hpp
 *
 *  Created on: 22.09.2014
 *      Author: andreas
 */

#ifndef CBRIDGE_HPP_
#define CBRIDGE_HPP_

#include <inttypes.h>
#include <map>
#include <list>
#include <ostream>
#include <exception>

#include <rofl/common/cpacket.h>
#include <rofl/common/ciosrv.h>

#include "cbridgeid.hpp"
#include "cport.hpp"

namespace rofeth {
namespace rstp {

class eBridgeBase : public std::runtime_error {
public:
	eBridgeBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eBridgeNotFound : public eBridgeBase {
public:
	eBridgeNotFound(const std::string& __arg) : eBridgeBase(__arg) {};
};


class cbridge_env {
public:
	virtual ~cbridge_env() {};
	virtual void port_enable(const cportid& portid) = 0;
	virtual void port_disable(const cportid& portid) = 0;
	virtual void send_bpdu_message(const cportid& portid, const rofl::cpacket& bpdu) = 0;
};

class cbridge : public cport_env, public rofl::ciosrv {

	static std::map<cbridgeid, cbridge*>	 	bridges;

	/**
	 *
	 */
	cbridge();
		// private and never used, not implemented

	/**
	 *
	 */
	cbridge(cbridge_env* env, const cbridgeid& bridgeid) :
		env(env),
		bridgeid(bridgeid),
		timer_interval_hello_when(DEFAULT_TIMER_INTERVAL_HELLO_WHEN)
	{
		timerid_hello_when = register_timer(TIMER_HELLO_WHEN, rofl::ctimespec(timer_interval_hello_when));
	};

	/**
	 *
	 */
	virtual
	~cbridge()
	{};

	/**
	 *
	 */
	cbridge(const cbridge& bridge);
		// private and never used, not implemented

	/**
	 *
	 */
	cbridge&
	operator= (const cbridge& bridge);
		// private and never used, not implemented

public:

	/**
	 *
	 */
	static cbridge&
	add_bridge(cbridge_env* env, const cbridgeid& bridgeid) {
		if (cbridge::bridges.find(bridgeid) != cbridge::bridges.end()) {
			delete cbridge::bridges[bridgeid];
		}
		cbridge::bridges[bridgeid] = new cbridge(env, bridgeid);
		return *(cbridge::bridges[bridgeid]);
	};

	/**
	 *
	 */
	static cbridge&
	set_bridge(cbridge_env* env, const cbridgeid& bridgeid) {
		if (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()) {
			cbridge::bridges[bridgeid] = new cbridge(env, bridgeid);
		}
		return *(cbridge::bridges[bridgeid]);
	};

	/**
	 *
	 */
	static cbridge&
	set_bridge(const cbridgeid& bridgeid) {
		if (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()) {
			throw eBridgeNotFound("cbridge::set_bridge()");
		}
		return *(cbridge::bridges[bridgeid]);
	};

	/**
	 *
	 */
	static const cbridge&
	get_bridge(const cbridgeid& bridgeid) {
		if (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()) {
			throw eBridgeNotFound("cbridge::get_bridge()");
		}
		return *(cbridge::bridges.at(bridgeid));
	};

	/**
	 *
	 */
	static void
	drop_bridge(const cbridgeid& bridgeid) {
		if (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()) {
			return;
		}
		delete cbridge::bridges[bridgeid];
		cbridge::bridges.erase(bridgeid);
	};

	/**
	 *
	 */
	static bool
	has_bridge(const cbridgeid& bridgeid) {
		return (not (cbridge::bridges.find(bridgeid) == cbridge::bridges.end()));
	};

public:

	/**
	 *
	 */
	cport&
	add_port(const cportid& portid) {
		if (ports.find(portid) != ports.end()) {
			delete ports[portid];
		}
		ports[portid] = new cport(this, portid);
		return *(ports[portid]);
	};

	/**
	 *
	 */
	cport&
	set_port(const cportid& portid) {
		if (ports.find(portid) == ports.end()) {
			ports[portid] = new cport(this, portid);
		}
		return *(ports[portid]);
	};

	/**
	 *
	 */
	const cport&
	get_port(const cportid& portid) const {
		if (ports.find(portid) == ports.end()) {
			throw ePortNotFound("cbridge::get_port()");
		}
		return *(ports.at(portid));
	};

	/**
	 *
	 */
	void
	drop_port(const cportid& portid) {
		if (ports.find(portid) == ports.end()) {
			return;
		}
		delete ports[portid];
		ports.erase(portid);
	};

	/**
	 *
	 */
	bool
	has_port(const cportid& portid) const {
		return (not (ports.find(portid) == ports.end()));
	};

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbridge& bridge) {
		return os;
	};

private:

	enum cbridge_timer_t {
		TIMER_HELLO_WHEN		= 1,
	};

	enum cbridge_event_t {
		EVENT_HELLO_WHEN		= 1,
	};

	cbridge_env*					env;
	cbridgeid						bridgeid;
	std::map<cportid, cport*>		ports;
	std::list<enum cbridge_event_t>	events;

	// HELLO_WHEN timer
	static const unsigned int DEFAULT_TIMER_INTERVAL_HELLO_WHEN = 2; // seconds
	unsigned int 					timer_interval_hello_when;
	rofl::ctimerid					timerid_hello_when;

private:

	friend class rofl::ciosrv;
	friend class cport;

	/**
	 *
	 */
	virtual void
	handle_timeout(int opaque, void* data = (void*)0);

	/**
	 *
	 */
	virtual void
	send_bpdu_message(const cportid& portid, const rofl::cpacket& bpdu) {
		if (env) env->send_bpdu_message(portid, bpdu);
	};

	/**
	 *
	 */
	void
	run_engine(enum cbridge_event_t event) {
		events.push_back(event);
		while (not events.empty()) {
			enum cbridge_event_t nxtevent = events.front(); events.pop_front();
			switch (nxtevent) {
			case EVENT_HELLO_WHEN: event_hello_when(); break;
			default: /* unhandled event? oops ... */ {};
			}
		}
	};

	/**
	 *
	 */
	void
	event_hello_when();
};

}; // end of namespace rstp
}; // end of namespace rofeth

#endif /* CBRIDGE_HPP_ */
