/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef QMFAGENT_H_
#define QMFAGENT_H_ 

#include <inttypes.h>
#include <map>
#include <string>
#include <ostream>
#include <exception>

#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Duration.h>
#include <qmf/AgentSession.h>
#include <qmf/AgentEvent.h>
#include <qmf/Schema.h>
#include <qmf/SchemaProperty.h>
#include <qmf/SchemaMethod.h>
#include <qmf/Data.h>
#include <qmf/DataAddr.h>
#ifdef WITH_QMF2_024
#include <qmf/EventNotifier.h>
#else
#include <qmf/posix/EventNotifier.h>
#endif
#include <qpid/types/Variant.h>

#include <rofl/common/ciosrv.h>
#include <rofl/platform/unix/cunixenv.h>

#include "ethcore.h"

/**
* @file qmfagent.h
* @author Andreas Koepsel<andreas.koepsel (at) bisdn.de>
*
* @brief QMF management plugin file
*/

namespace qmf
{

class eQmfAgentBase 		: public std::exception {};
class eQmfAgentInval		: public eQmfAgentBase {};
class eQmfAgentInvalSubcmd	: public eQmfAgentInval {};

/**
* @brief Qpid Management Framework (QMF) management plugin
*
* @description This plugin exposes xDPd's Port and Switch management APIs via a QMF broker
* @ingroup cmm_mgmt_plugins
*/
class qmfagent :
		public rofl::ciosrv
{
private:

	std::string						broker_url;
	std::string						ethcore_id;

	qpid::messaging::Connection 	connection;
	qmf::AgentSession 				session;
	qmf::posix::EventNotifier		notifier;
	std::string						qmf_package;
	qmf::Schema						sch_exception;
	qmf::Schema						sch_ethcore;

	struct qmf_data_t {
		qmf::Data 					data;
		qmf::DataAddr 				addr;
	};

	struct qmf_data_t				qethcore;

private:

    static qmfagent          		*sqmfagent;

    /**
     * @brief	Make copy constructor private for singleton
     */
    qmfagent(qmfagent const& sqmfagent) {};

	/**
	 *
	 */
	qmfagent();

	/**
	 *
	 */
	virtual
	~qmfagent();

public:

	/**
	 *
	 */
    static qmfagent&
    get_instance();


public:

	/**
	 *
	 */
	virtual void init(int argc, char** argv);

	virtual std::string get_name(){
		return std::string("qmf_agent");
	}

protected:

	/**
	 *
	 */
	virtual void
	handle_timeout(int opaque);

	/**
	 *
	 */
	virtual void
	handle_revent(int fd);

private:

	/**
	 *
	 */
	void
	set_qmf_schema();

	/**
	 *
	 */
	bool
	method(qmf::AgentEvent& event);

	/**
	 *
	 */
	bool
	methodVlanAdd(qmf::AgentEvent& event);

	/**
	 *
	 */
	bool
	methodVlanDrop(qmf::AgentEvent& event);

	/**
	 *
	 */
	bool
	methodPortAdd(qmf::AgentEvent& event);

	/**
	 *
	 */
	bool
	methodPortDrop(qmf::AgentEvent& event);
};

}; // end of namespace

#endif /* QMFAGENT_H_ */
