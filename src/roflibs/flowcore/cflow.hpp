/*
 * cflow.hpp
 *
 *  Created on: 01.11.2014
 *      Author: andreas
 */

#ifndef CFLOW_HPP_
#define CFLOW_HPP_

#include <bitset>
#include <vector>
#include <string>
#include <ostream>
#include <exception>

#include <rofl/common/crofdpt.h>
#include <rofl/common/cdptid.h>
#include <rofl/common/openflow/cofflowmod.h>

#include "roflibs/netlink/clogging.hpp"

namespace roflibs {
namespace svc {

class eFlowBase : public std::runtime_error {
public:
	eFlowBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eFlowNotFound : public eFlowBase {
public:
	eFlowNotFound(const std::string& __arg) : eFlowBase(__arg) {};
};

class cflow {
public:

	/**
	 *
	 */
	cflow() :
		state(STATE_DETACHED)
	{};

	/**
	 *
	 */
	~cflow()
	{};

	/**
	 *
	 */
	cflow(const rofl::cdpid& dpid,
			const rofl::openflow::cofflowmod& flowmod) :
		state(STATE_DETACHED), dpid(dpid), flowmod(flowmod)
	{};

	/**
	 *
	 */
	cflow(const cflow& flow)
	{ *this = flow; };

	/**
	 *
	 */
	cflow&
	operator= (const cflow& flow) {
		if (this == &flow)
			return *this;
		state 		= flow.state;
		dpid 		= flow.dpid;
		flowmod		= flow.flowmod;
		return *this;
	};

public:

	/**
	 *
	 */
	void
	handle_dpt_open(rofl::crofdpt& dpt) {
		try {

			flowmod.set_version(dpt.get_version());
			flowmod.set_command(rofl::openflow::OFPFC_ADD);
			dpt.send_flow_mod_message(rofl::cauxid(0), flowmod);

			state = STATE_ATTACHED;

		} catch (rofl::eRofDptNotFound& e) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_open] unable to find data path" << e.what() << std::endl;
		} catch (rofl::eRofSockTxAgain& e) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_open] control channel congested" << e.what() << std::endl;
		} catch (rofl::eRofBaseNotConnected& e) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_open] control channel is down" << e.what() << std::endl;
		} catch (...) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_open] unexpected error" << std::endl;
		}
	};

	/**
	 *
	 */
	void
	handle_dpt_close(rofl::crofdpt& dpt) {
		try {

			flowmod.set_version(dpt.get_version());
			flowmod.set_command(rofl::openflow::OFPFC_DELETE_STRICT);
			dpt.send_flow_mod_message(rofl::cauxid(0), flowmod);

			state = STATE_DETACHED;

		} catch (rofl::eRofDptNotFound& e) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_close] unable to find data path" << e.what() << std::endl;
		} catch (rofl::eRofSockTxAgain& e) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_close] control channel congested" << e.what() << std::endl;
		} catch (rofl::eRofBaseNotConnected& e) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_close] control channel is down" << e.what() << std::endl;
		} catch (...) {
			rofcore::logging::error << "[roflibs][svc][cflow][handle_dpt_close] unexpected error" << std::endl;
		}
	};

public:

	/**
	 *
	 */
	const rofl::openflow::cofflowmod&
	get_flowmod() const { return flowmod; };

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cflow& greterm) {
		os << rofcore::indent(0) << "<cflow >" << std::endl;
		return os;
	};

protected:

	enum ofp_state_t {
		STATE_DETACHED = 1,
		STATE_ATTACHED = 2,
	};
	enum ofp_state_t 			state;

	rofl::cdpid 				dpid;
	rofl::openflow::cofflowmod 	flowmod;
};

}; // end of namespace flow
}; // end of namespace roflibs

#endif /* CFLOW_HPP_ */
