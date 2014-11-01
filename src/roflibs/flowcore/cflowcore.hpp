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

#include <roflibs/netlink/clogging.hpp>

namespace roflibs {
namespace flow {

class eFlowCoreBase : public std::runtime_error {
public:
	eFlowCoreBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eFlowCoreNotFound : public eFlowCoreBase {
public:
	eFlowCoreNotFound(const std::string& __arg) : eFlowCoreBase(__arg) {};
};

class cflowcore {
public:

	/**
	 *
	 */
	static cflowcore&
	add_flow_core(const rofl::cdpid& dpid, uint8_t flow_table_id) {
		if (cflowcore::flowcores.find(dpid) != cflowcore::flowcores.end()) {
			delete cflowcore::flowcores[dpid];
			cflowcore::flowcores.erase(dpid);
		}
		cflowcore::flowcores[dpid] = new cflowcore(dpid, flow_table_id);
		return *(cflowcore::flowcores[dpid]);
	};

	/**
	 *
	 */
	static cflowcore&
	set_flow_core(const rofl::cdpid& dpid, uint8_t flow_table_id) {
		if (cflowcore::flowcores.find(dpid) == cflowcore::flowcores.end()) {
			cflowcore::flowcores[dpid] = new cflowcore(dpid, flow_table_id);
		}
		return *(cflowcore::flowcores[dpid]);
	};


	/**
	 *
	 */
	static cflowcore&
	set_flow_core(const rofl::cdpid& dpid) {
		if (cflowcore::flowcores.find(dpid) == cflowcore::flowcores.end()) {
			throw eFlowCoreNotFound("cflowcore::set_flow_core() dpt not found");
		}
		return *(cflowcore::flowcores[dpid]);
	};

	/**
	 *
	 */
	static const cflowcore&
	get_flow_core(const rofl::cdpid& dpid) {
		if (cflowcore::flowcores.find(dpid) == cflowcore::flowcores.end()) {
			throw eFlowCoreNotFound("cflowcore::get_flow_core() dpt not found");
		}
		return *(cflowcore::flowcores.at(dpid));
	};

	/**
	 *
	 */
	static void
	drop_flow_core(const rofl::cdpid& dpid) {
		if (cflowcore::flowcores.find(dpid) == cflowcore::flowcores.end()) {
			return;
		}
		delete cflowcore::flowcores[dpid];
		cflowcore::flowcores.erase(dpid);
	}

	/**
	 *
	 */
	static bool
	has_flow_core(const rofl::cdpid& dpid) {
		return (not (cflowcore::flowcores.find(dpid) == cflowcore::flowcores.end()));
	};

private:

	/**
	 *
	 */
	cflowcore(const rofl::cdpid& dpid,
			uint8_t flow_table_id) :
		state(STATE_DETACHED), dpid(dpid),
		flow_table_id(flow_table_id) {};

	/**
	 *
	 */
	~cflowcore() {
#if 0
		while (not terms_in4.empty()) {
			uint32_t term_id = terms_in4.begin()->first;
			drop_gre_term_in4(term_id);
		}
#endif
	};

private:

	static std::map<rofl::cdpid, cflowcore*>		flowcores;

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};

	enum ofp_state_t				state;
	rofl::cdpid						dpid;
	uint8_t							flow_table_id;

	//std::map<uint32_t, cgreterm_in4*>			terms_in4;
	mutable rofl::PthreadRwLock					rwlock_in4;
};

}; // end of namespace flow
}; // end of namespace roflibs

#endif /* CFLOWCORE_HPP_ */
