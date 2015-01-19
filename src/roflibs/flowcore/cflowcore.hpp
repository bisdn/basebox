/*
 * cflowcore.hpp
 *
 *  Created on: 01.11.2014
 *      Author: andreas
 */

#ifndef CFLOWCORE_HPP_
#define CFLOWCORE_HPP_

#include <map>
#include <iostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/thread_helper.h>
#include <rofl/common/openflow/cofflowmod.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/cauxid.h>
#include <rofl/common/openflow/messages/cofmsg_packet_in.h>
#include <rofl/common/openflow/messages/cofmsg_flow_removed.h>
#include <rofl/common/openflow/messages/cofmsg_error.h>

#include "roflibs/netlink/clogging.hpp"
#include "roflibs/flowcore/cflow.hpp"
#include "roflibs/netlink/ccookiebox.hpp"

namespace roflibs {
namespace svc {

class eFlowCoreBase : public std::runtime_error {
public:
	eFlowCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eFlowCoreNotFound : public eFlowCoreBase {
public:
	eFlowCoreNotFound(const std::string& __arg) : eFlowCoreBase(__arg) {};
};

class cflowcore : public roflibs::common::openflow::ccookie_owner {
public:

	/**
	 *
	 */
	static cflowcore&
	add_flow_core(const rofl::cdptid& dptid) {
		if (cflowcore::flowcores.find(dptid) != cflowcore::flowcores.end()) {
			delete cflowcore::flowcores[dptid];
			cflowcore::flowcores.erase(dptid);
		}
		cflowcore::flowcores[dptid] = new cflowcore(dptid);
		return *(cflowcore::flowcores[dptid]);
	};

	/**
	 *
	 */
	static cflowcore&
	set_flow_core(const rofl::cdptid& dptid) {
		if (cflowcore::flowcores.find(dptid) == cflowcore::flowcores.end()) {
			cflowcore::flowcores[dptid] = new cflowcore(dptid);
		}
		return *(cflowcore::flowcores[dptid]);
	};


	/**
	 *
	 */
	static const cflowcore&
	get_flow_core(const rofl::cdptid& dptid) {
		if (cflowcore::flowcores.find(dptid) == cflowcore::flowcores.end()) {
			throw eFlowCoreNotFound("cflowcore::get_flow_core() dpt not found");
		}
		return *(cflowcore::flowcores.at(dptid));
	};

	/**
	 *
	 */
	static void
	drop_flow_core(const rofl::cdptid& dptid) {
		if (cflowcore::flowcores.find(dptid) == cflowcore::flowcores.end()) {
			return;
		}
		delete cflowcore::flowcores[dptid];
		cflowcore::flowcores.erase(dptid);
	}

	/**
	 *
	 */
	static bool
	has_flow_core(const rofl::cdptid& dptid) {
		return (not (cflowcore::flowcores.find(dptid) == cflowcore::flowcores.end()));
	};

private:

	/**
	 *
	 */
	cflowcore(const rofl::cdptid& dptid) :
		state(STATE_DETACHED), dptid(dptid)
	{};

	/**
	 *
	 */
	virtual
	~cflowcore() {
		while (not flows.empty()) {
			uint32_t flow_id = flows.begin()->first;
			drop_svc_flow(flow_id);
		}
	};

public:

	/**
	 *
	 */
	void
	handle_dpt_open() {
		for (std::map<uint32_t, cflow*>::iterator
				it = flows.begin(); it != flows.end(); ++it) {
			it->second->handle_dpt_open();
		}
	};

	/**
	 *
	 */
	void
	handle_dpt_close() {
		for (std::map<uint32_t, cflow*>::iterator
				it = flows.begin(); it != flows.end(); ++it) {
			it->second->handle_dpt_close();
		}
	};

public:

	/**
	 *
	 */
	std::vector<uint32_t>
	get_svc_flows() const {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_READ);
		std::vector<uint32_t> termids;
		for (std::map<uint32_t, cflow*>::const_iterator
				it = flows.begin(); it != flows.end(); ++it) {
			termids.push_back(it->first);
		}
		return termids;
	};

	/**
	 *
	 */
	void
	clear_svc_flows() {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_WRITE);
		for (std::map<uint32_t, cflow*>::iterator
				it = flows.begin(); it != flows.end(); ++it) {
			delete it->second;
		}
		flows.clear();
	};

	/**
	 *
	 */
	cflow&
	add_svc_flow(uint32_t flow_id,
			const rofl::openflow::cofflowmod& flowmod) {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (flows.find(flow_id) != flows.end()) {
			delete flows[flow_id];
			flows.erase(flow_id);
		}
		flows[flow_id] = new cflow(dptid, flowmod);
		try {
			if (STATE_ATTACHED == state) {
				flows[flow_id]->handle_dpt_open();
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return *(flows[flow_id]);
	};

	/**
	 *
	 */
	cflow&
	set_svc_flow(uint32_t flow_id,
			const rofl::openflow::cofflowmod& flowmod) {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (flows.find(flow_id) == flows.end()) {
			flows[flow_id] = new cflow(dptid, flowmod);
		}
		try {
			if (STATE_ATTACHED == state) {
				flows[flow_id]->handle_dpt_open();
			}
		} catch (rofl::eRofDptNotFound& e) {};
		return *(flows[flow_id]);
	};

	/**
	 *
	 */
	cflow&
	set_svc_flow(uint32_t flow_id) {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_READ);
		if (flows.find(flow_id) == flows.end()) {
			throw eFlowNotFound("cgrecore::get_svc_flow() flow_id not found");
		}
		return *(flows[flow_id]);
	};

	/**
	 *
	 */
	const cflow&
	get_svc_flow(uint32_t flow_id) const {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_READ);
		if (flows.find(flow_id) == flows.end()) {
			throw eFlowNotFound("cgrecore::get_term_in4() flow_id not found");
		}
		return *(flows.at(flow_id));
	};

	/**
	 *
	 */
	void
	drop_svc_flow(uint32_t flow_id) {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_WRITE);
		if (flows.find(flow_id) == flows.end()) {
			return;
		}
		delete flows[flow_id];
		flows.erase(flow_id);
	};

	/**
	 *
	 */
	bool
	has_svc_flow(uint32_t flow_id) const {
		rofl::RwLock rwl(rwlock, rofl::RwLock::RWLOCK_READ);
		return (not (flows.find(flow_id) == flows.end()));
	};

public:

	/**
	 *
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, const rofl::cauxid& auxid, rofl::openflow::cofmsg_packet_in& msg) {};

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
	operator<< (std::ostream& os, const cflowcore& flowcore) {

		return os;
	};

private:

	static std::map<rofl::cdptid, cflowcore*>		flowcores;

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t				state;
	rofl::cdptid					dptid;

	std::map<uint32_t, cflow*>		flows;
	mutable rofl::PthreadRwLock		rwlock;
};

}; // end of namespace flow
}; // end of namespace roflibs

#endif /* CFLOWCORE_HPP_ */
