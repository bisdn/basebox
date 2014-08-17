/*
 * crofproxy.hpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#ifndef CROFPROXY_HPP_
#define CROFPROXY_HPP_

#include <set>
#include <string>
#include <bitset>

#include <rofl/common/ciosrv.h>
#include <rofl/common/crofctl.h>
#include <rofl/common/crofdpt.h>
#include <rofl/common/openflow/messages/cofmsg.h>
#include <rofl/common/openflow/messages/cofmsg_hello.h>
#include <rofl/common/openflow/messages/cofmsg_echo.h>
#include <rofl/common/openflow/messages/cofmsg_error.h>
#include <rofl/common/openflow/messages/cofmsg_features.h>
#include <rofl/common/openflow/messages/cofmsg_config.h>
#include <rofl/common/openflow/messages/cofmsg_packet_out.h>
#include <rofl/common/openflow/messages/cofmsg_packet_in.h>
#include <rofl/common/openflow/messages/cofmsg_flow_mod.h>
#include <rofl/common/openflow/messages/cofmsg_flow_removed.h>
#include <rofl/common/openflow/messages/cofmsg_group_mod.h>
#include <rofl/common/openflow/messages/cofmsg_table_mod.h>
#include <rofl/common/openflow/messages/cofmsg_port_mod.h>
#include <rofl/common/openflow/messages/cofmsg_port_status.h>
#include <rofl/common/openflow/messages/cofmsg_stats.h>
#include <rofl/common/openflow/messages/cofmsg_desc_stats.h>
#include <rofl/common/openflow/messages/cofmsg_flow_stats.h>
#include <rofl/common/openflow/messages/cofmsg_aggr_stats.h>
#include <rofl/common/openflow/messages/cofmsg_table_stats.h>
#include <rofl/common/openflow/messages/cofmsg_port_stats.h>
#include <rofl/common/openflow/messages/cofmsg_queue_stats.h>
#include <rofl/common/openflow/messages/cofmsg_group_stats.h>
#include <rofl/common/openflow/messages/cofmsg_group_desc_stats.h>
#include <rofl/common/openflow/messages/cofmsg_group_features_stats.h>
#include <rofl/common/openflow/messages/cofmsg_port_desc_stats.h>
#include <rofl/common/openflow/messages/cofmsg_experimenter_stats.h>
#include <rofl/common/openflow/messages/cofmsg_barrier.h>
#include <rofl/common/openflow/messages/cofmsg_queue_get_config.h>
#include <rofl/common/openflow/messages/cofmsg_role.h>
#include <rofl/common/openflow/messages/cofmsg_experimenter.h>
#include <rofl/common/openflow/messages/cofmsg_async_config.h>


#include "erofcorexcp.hpp"
#include "crofmodel.hpp"
#include "crofconf.hpp"
#include "cctlid.hpp"
#include "cdptid.hpp"
#include "croftransactions.hpp"

namespace rofcore {

class eRofProxyBase				: public eRofCoreXcp {};
class eRofProxyBusy				: public eRofProxyBase {};
class eRofProxyNotFound			: public eRofProxyBase {};
class eRofProxyPrerequisites	: public eRofProxyBase {};

class crofproxy;

class crofproxy_env {
public:
	/**
	 *
	 */
	virtual
	~crofproxy_env() {};

	/**
	 *
	 */
	virtual cctlid const
	rpc_connect_to_ctl(
			crofproxy const* rofproxy,
			rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap,
			int reconnect_start_timeout,
			enum rofl::csocket::socket_type_t socket_type,
			rofl::cparams const& socket_param) = 0;

	/**
	 *
	 */
	virtual void
	rpc_disconnect_from_ctl(
			crofproxy const* rofproxy,
			cctlid const& ctlid) = 0;


	/**
	 *
	 */
	virtual void
	rpc_disconnect_from_dpt(
			crofproxy const* rofproxy,
			cdptid const& dpid) = 0;
};


class crofproxy {

	enum crofproxy_flags_t {
		FLAG_IDLE				= 0,
		FLAG_FEATURES_SENT		= 1,
		FLAG_GET_CONFIG_SENT	= 2,
		FLAG_TABLE_SENT			= 3,
		FLAG_TABLE_FEATURES_SENT= 4,
		FLAG_PORTS_DESC_SENT	= 5,
		FLAG_ROLE_SENT			= 6,
		/*
		 * ...
		 */
		FLAG_ESTABLISHED		= 10,
		FLAG_ENABLE_PROXY		= 11,
	};

	enum crofproxy_event_t {
		EVENT_NONE									= 0,
		EVENT_DISCONNECTED							= 1,
		EVENT_CONNECTED								= 2,
		EVENT_FEATURES_REPLY_RCVD					= 3,
		EVENT_FEATURES_REQUEST_EXPIRED				= 4,
		EVENT_GET_CONFIG_REPLY_RCVD					= 5,
		EVENT_GET_CONFIG_REQUEST_EXPIRED			= 6,
		EVENT_TABLE_STATS_REPLY_RCVD				= 7,
		EVENT_TABLE_STATS_REQUEST_EXPIRED			= 8,
		EVENT_TABLE_FEATURES_STATS_REPLY_RCVD		= 9,
		EVENT_TABLE_FEATURES_STATS_REQUEST_EXPIRED	= 10,
		EVENT_PORT_DESC_STATS_REPLY_RCVD			= 11,
		EVENT_PORT_DESC_STATS_REQUEST_EXPIRED		= 12,
		EVENT_ROLE_REPLY_RCVD						= 13,
		EVENT_ROLE_REQUEST_EXPIRED					= 14,
		EVENT_ESTABLISHED							= 15,
	};

	static const unsigned int 			REQUIRED_MISS_SEND_LEN;

	crofproxy_env						*rofproxy_env;
	cctlid								cid;	// controller id of attached control entity
	cdptid								did;	// data path id of attached data path entity
	crofmodel							cmodel;	// model exposed to control entities
	crofmodel							dmodel;	// model received from data path entity
	std::deque<enum crofproxy_event_t> 	events;
	std::bitset<64>						flags;
	rofl::openflow::cofrole				role;
	croftransactions					tas;

public:

	/**
	 * @brief	Existing proxy types
	 */
	enum rofproxy_type_t {
		PROXY_TYPE_ETHCORE		= 1,
		PROXY_TYPE_IPCORE		= 2,
		// add more proxy types here ...
		PROXY_TYPE_MAX,
	};

	/**
	 *
	 */
	static crofproxy*
	crofproxy_factory(
			enum rofproxy_type_t proxy_type);

public:

	/**
	 *
	 */
	crofproxy(
			crofproxy_env* rofproxy_env);

	/**
	 *
	 */
	virtual
	~crofproxy();

	/**
	 *
	 */
	crofproxy(
			crofproxy const& rofproxy);

	/**
	 *
	 */
	crofproxy&
	operator= (
			crofproxy const& rofproxy);

public:

	/**
	 *
	 */
	virtual void
	signal_handler(
			int signum);

	// TODO

public:

	friend std::ostream&
	operator<< (std::ostream& os, crofproxy const& rofproxy) {
		// TODO
		return os;
	};

public:

	/**
	 * @name Event handlers
	 *
	 * The following methods should be overwritten by a derived class
	 * in order to get reception notifications for the various OF
	 * packets. While crofbase handles most of the lower layer details,
	 * a derived class must provide higher layer functionality.
	 */

	/**@{*/

	/**
	 * @brief	Called by rofl::crofbase once a new data path element has connected
	 *
	 * This method creates two new instances of type \see{cnode} and \see{sgwctl} for
	 * a new connecting data path element and informs the qmfagent singleton.
	 *
	 * @param dpt rofl handle to the data path element
	 */
	void
	recv_dpath_open(
			rofl::crofdpt& dpt);

	/**
	 * @brief	Called by rofl::crofbase once a control connection to a data path element was lost
	 *
	 * This method deletes existing instances of class \see{cnode} and \see{sgwctl} for
	 * the disconnected data path element and informs the qmfagent singleton.
	 * Do not use the dpt handle except for removing any references.
	 *
	 * @param dpt rofl handle to the data path element
	 */
	void
	recv_dpath_close(
			rofl::crofdpt& dpt);


	/**
	 * @brief	Called once a FEATURES.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the FEATURES.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_features_request message containing the received message
	 */
	void
	recv_features_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_features_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_features_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a FEATURES.reply message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt Pointer to cofdpt instance from which the FEATURES.reply was received
	 * @param msg Pointer to rofl::openflow::cofmsg_features_reply message containing the received message
	 */
	void
	recv_features_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_features_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a timer expires for a FEATURES.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	void
	recv_features_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


	/**
	 * @brief	Called once a GET-CONFIG.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the GET-CONFIG.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_config_request message containing the received message
	 */
	void
	recv_get_config_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_get_config_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_get_config_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GET-CONFIG.reply message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt Pointer to cofdpt instance from which the GET-CONFIG.reply was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_config_reply message containing the received message
	 */
	void
	recv_get_config_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_config_reply& msg, uint8_t aux_id = 0);



	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	void
	recv_get_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


	/**
	 * @brief	Called once a set-config.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the EXPERIMENTER.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	void
	recv_set_config(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_set_config& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_set_config(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a DESC-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the DESC-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_desc_stats_request message containing the received message
	 */
	void
	recv_desc_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_desc_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_desc_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a DESC-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the DESC-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_desc_stats_reply message containing the received message
	 */
	void
	recv_desc_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_desc_stats_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a TABLE-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the TABLE-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_table_stats_request message containing the received message
	 */
	void
	recv_table_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_table_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a TABLE-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the TABLE-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_table_stats_reply message containing the received message
	 */
	void
	recv_table_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_stats_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a PORT-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the PORT-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_port_stats_request message containing the received message
	 */
	void
	recv_port_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_port_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a PORT-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PORT-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_port_stats_reply message containing the received message
	 */
	void
	recv_port_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_port_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a FLOW-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the FLOW-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_flow_stats_request message containing the received message
	 */
	void
	recv_flow_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_flow_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a FLOW-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the FLOW-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_flow_stats_reply message containing the received message
	 */
	void
	recv_flow_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_flow_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once an AGGREGATE-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the AGGREGATE-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_aggr_stats_request message containing the received message
	 */
	void
	recv_aggregate_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_aggr_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_aggregate_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once an AGGREGATE-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the AGGREGATE-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_aggregate_stats_reply message containing the received message
	 */
	void
	recv_aggregate_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_aggr_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_aggregate_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a QUEUE-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the QUEUE-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_queue_stats_request message containing the received message
	 */
	void
	recv_queue_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_queue_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a QUEUE-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the QUEUE-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_queue_stats_reply message containing the received message
	 */
	void
	recv_queue_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_queue_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GROUP-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_stats_request message containing the received message
	 */
	void
	recv_group_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_group_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GROUP-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the GROUP-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_group_stats_reply message containing the received message
	 */
	void
	recv_group_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_group_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GROUP-DESC-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-DESC-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_desc_stats_request message containing the received message
	 */
	void
	recv_group_desc_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_desc_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_group_desc_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GROUP-DESC-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the GROUP-DESC-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_group_desc_stats_reply message containing the received message
	 */
	void
	recv_group_desc_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_desc_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_group_desc_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GROUP-FEATURES-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-FEATURES-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_features_stats_request message containing the received message
	 */
	void
	recv_group_features_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_features_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_group_features_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GROUP-FEATURES-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the GROUP-FEATURES-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_group_features_stats_reply message containing the received message
	 */
	void
	recv_group_features_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_features_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_group_features_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a TABLE-FEATURES-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the TABLE-FEATURES-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_table_features_request message containing the received message
	 */
	void
	recv_table_features_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_features_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_table_features_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a TABLE-FEATURES-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the TABLE-FEATURES-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_table_features_reply message containing the received message
	 */
	void
	recv_table_features_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_features_stats_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a PORT-DESC-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the PORT-DESC-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_port_desc_stats_request message containing the received message
	 */
	void
	recv_port_desc_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_desc_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_port_desc_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a PORT-DESC-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PORT-DESC-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_port_desc_stats_reply message containing the received message
	 */
	void
	recv_port_desc_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_desc_stats_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a EXPERIMENTER-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the EXPERIMENTER-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_experimenter_stats_request message containing the received message
	 */
	void
	recv_experimenter_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter_stats_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_experimenter_stats_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once an EXPERIMENTER-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the EXPERIMENTER-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter_stats_reply message containing the received message
	 */
	void
	recv_experimenter_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter_stats_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_experimenter_stats_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a timer has expired for a STATS.reply message.
	 *
	 * To be overwritten by derived class. Default behaviour: ignores event.
	 *
	 * @param dpt pointer to cofdpt instance where the timeout occured.
	 * @param xid transaction ID of STATS.request previously sent to data path element.
	 */
	void
	recv_multipart_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type);


	/**
	 * @brief	Called once a PACKET-OUT.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the PACKET-OUT.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_packet_out message containing the received message
	 */
	void
	recv_packet_out(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_packet_out& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_packet_out(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a PACKET-IN.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PACKET-IN.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_packet_in message containing the received message
	 */
	void
	recv_packet_in(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_packet_in& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_packet_in(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a BARRIER.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the BARRIER.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_barrier_request message containing the received message
	 */
	void
	recv_barrier_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_barrier_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_barrier_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a BARRIER.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the BARRIER.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_barrier_reply message containing the received message
	 */
	void
	recv_barrier_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_barrier_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_barrier_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a timer has expired for a BARRIER.reply message.
	 *
	 * To be overwritten by derived class. Default behaviour: ignores event.
	 *
	 * @param dpt pointer to cofdpt instance where the timeout occured.
	 * @param xid transaction ID of BARRIER.request previously sent to data path element.
	 */
	void
	recv_barrier_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_barrier_reply_timeout(dpt, xid);
		}
	};


	/**
	 * @brief	Called once an ERROR.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl pointer to cofctl instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	void
	recv_error_message(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_error& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_error_message(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once an ERROR.message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	void
	recv_error_message(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_error& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_error_message(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a FLOW-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the FLOW-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_flow_mod message containing the received message
	 */
	void
	recv_flow_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_mod& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_flow_mod(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GROUP-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_mod message containing the received message
	 */
	void
	recv_group_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_mod& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_group_mod(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a TABLE-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the TABLE-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_table_mod message containing the received message
	 */
	void
	recv_table_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_mod& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_table_mod(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a PORT-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the PORT-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_port_mod message containing the received message
	 */
	void
	recv_port_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_mod& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_port_mod(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a FLOW-REMOVED.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the FLOW-REMOVED.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_flow_removed message containing the received message
	 */
	void
	recv_flow_removed(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_removed& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_flow_removed(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a PORT-STATUS.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PORT-STATUS.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_port_status message containing the received message
	 */
	void
	recv_port_status(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_status& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a QUEUE-GET-CONFIG.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the QUEUE-GET-CONFIG.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_queue_get_config_request message containing the received message
	 */
	void
	recv_queue_get_config_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_get_config_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_queue_get_config_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a QUEUE-GET-CONFIG.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the QUEUE-GET-CONFIG.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_queue_get_config_reply message containing the received message
	 */
	void
	recv_queue_get_config_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_get_config_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_queue_get_config_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	void
	recv_queue_get_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_queue_get_config_reply_timeout(dpt, xid);
		}
	};


	/**
	 * @brief	Called once an EXPERIMENTER.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl pointer to cofctl instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	void
	recv_experimenter_message(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_experimenter_message(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once an EXPERIMENTER.message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	void
	recv_experimenter_message(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_experimenter_message(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	void
	recv_experimenter_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_experimenter_timeout(dpt, xid);
		}
	};


	/**
	 * @brief	Called once a ROLE.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the ROLE.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_role_request message containing the received message
	 */
	void
	recv_role_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_role_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_role_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a ROLE.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the ROLE.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_role_reply message containing the received message
	 */
	void
	recv_role_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_role_reply& msg, uint8_t aux_id = 0);



	/**
	 * @brief	Called once a timer has expired for a ROLE.reply.
	 *
	 * To be overwritten by derived class. Default behaviour: ignores event.
	 *
	 * @param dpt pointer to cofdpt instance where the timeout occured.
	 * @param xid transaction ID of ROLE.reply message previously sent to data path element.
	 */
	void
	recv_role_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


	/**
	 * @brief	Called once a GET-ASYNC-CONFIG.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the GET-ASYNC-CONFIG.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_async_config_request message containing the received message
	 */
	void
	recv_get_async_config_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_get_async_config_request& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_get_async_config_request(ctl, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a GET-ASYNC-CONFIG.reply message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt Pointer to cofdpt instance from which the GET-ASYNC-CONFIG.reply was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_async_config_reply message containing the received message
	 */
	void
	recv_get_async_config_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_async_config_reply& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_get_async_config_reply(dpt, msg, aux_id);
		}
	};


	/**
	 * @brief	Called once a timer expires for a GET-ASYNC-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	void
	recv_get_async_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_get_async_config_reply_timeout(dpt, xid);
		}
	};


	/**
	 * @brief	Called once an SET-ASYNC-CONFIG.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the SET-ASYNC-MESSAGE.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_set_async_config message containing the received message
	 */
	void
	recv_set_async_config(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_set_async_config& msg, uint8_t aux_id = 0) {
		if (flags.test(FLAG_ESTABLISHED)) {
			handle_set_async_config(ctl, msg, aux_id);
		}
	};

	/**@}*/


protected:

	/**
	 *
	 */
	crofproxy_env&
	get_env() {
		return *rofproxy_env;
	};

	/**
	 *
	 */
	rofl::crofdpt&
	get_dpt() {
		return rofl::crofdpt::get_dpt(did.get_dpid());
	};

	/**
	 *
	 */
	rofl::crofctl&
	get_ctl() {
		return rofl::crofctl::get_ctl(cid.get_ctlid());
	};

	/**
	 *
	 */
	virtual cctlid const
	rpc_connect_to_ctl(
			rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap,
			int reconnect_start_timeout,
			enum rofl::csocket::socket_type_t socket_type,
			rofl::cparams const& socket_params) {
		cid = get_env().rpc_connect_to_ctl(
				this, versionbitmap, reconnect_start_timeout, socket_type, socket_params);
		return cid;
	};

	/**
	 *
	 */
	virtual void
	rpc_disconnect_from_ctl(
			cctlid const& ctlid) {
		get_env().rpc_disconnect_from_ctl(this, ctlid);
	};

	/**
	 *
	 */
	virtual void
	rpc_disconnect_from_dpt(
			cdptid const& dptid) {
		get_env().rpc_disconnect_from_dpt(this, dptid);
	};

public:

	/**
	 * @name Event handlers
	 *
	 * The following methods should be overwritten by a derived class
	 * in order to get reception notifications for the various OF
	 * packets. While crofbase handles most of the lower layer details,
	 * a derived class must provide higher layer functionality.
	 */

	/**@{*/

	/**
	 * @brief	Called by rofl::crofbase once a new data path element has connected
	 *
	 * This method creates two new instances of type \see{cnode} and \see{sgwctl} for
	 * a new connecting data path element and informs the qmfagent singleton.
	 *
	 * @param dpt rofl handle to the data path element
	 */
	virtual void
	handle_dpath_open(
			rofl::crofdpt& dpt) = 0;

	/**
	 * @brief	Called by rofl::crofbase once a control connection to a data path element was lost
	 *
	 * This method deletes existing instances of class \see{cnode} and \see{sgwctl} for
	 * the disconnected data path element and informs the qmfagent singleton.
	 * Do not use the dpt handle except for removing any references.
	 *
	 * @param dpt rofl handle to the data path element
	 */
	virtual void
	handle_dpath_close(
			rofl::crofdpt& dpt) = 0;


	/**
	 * @brief	Called once a FEATURES.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the FEATURES.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_features_request message containing the received message
	 */
	virtual void
	handle_features_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_features_request& msg, uint8_t aux_id = 0) {
		ctl.send_features_reply(msg.get_xid(), did.get_dpid(), cmodel.get_n_buffers(),
				cmodel.get_n_tables(),	cmodel.get_capabilities(),	cmodel.get_auxiliary_id(),
					0, cmodel.get_ports());
	};


	/**
	 * @brief	Called once a FEATURES.reply message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt Pointer to cofdpt instance from which the FEATURES.reply was received
	 * @param msg Pointer to rofl::openflow::cofmsg_features_reply message containing the received message
	 */
	virtual void
	handle_features_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_features_reply& msg, uint8_t aux_id = 0) {
		// handled by derived proxy class
	};


	/**
	 * @brief	Called once a timer expires for a FEATURES.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_features_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {};


	/**
	 * @brief	Called once a GET-CONFIG.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the GET-CONFIG.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_config_request message containing the received message
	 */
	virtual void
	handle_get_config_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_get_config_request& msg, uint8_t aux_id = 0) {
		ctl.send_get_config_reply(msg.get_xid(), cmodel.get_flags(), cmodel.get_miss_send_len());
	};


	/**
	 * @brief	Called once a GET-CONFIG.reply message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt Pointer to cofdpt instance from which the GET-CONFIG.reply was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_config_reply message containing the received message
	 */
	virtual void
	handle_get_config_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_config_reply& msg, uint8_t aux_id = 0) {
		// handled by derived proxy class
	};


	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_get_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {};


	/**
	 * @brief	Called once a set-config.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the EXPERIMENTER.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	virtual void
	handle_set_config(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_set_config& msg, uint8_t aux_id = 0) {
		cmodel.set_flags(msg.get_flags());
		cmodel.set_miss_send_len(msg.get_miss_send_len());
		get_dpt().send_set_config_message(msg.get_flags(), msg.get_miss_send_len());
	};


	/**
	 * @brief	Called once a DESC-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the DESC-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_desc_stats_request message containing the received message
	 */
	virtual void
	handle_desc_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_desc_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_desc_stats_request(0);
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a DESC-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the DESC-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_desc_stats_reply message containing the received message
	 */
	virtual void
	handle_desc_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_desc_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_desc_stats_reply(ctlxid, msg.get_desc_stats(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a desc-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_desc_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a TABLE-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the TABLE-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_table_stats_request message containing the received message
	 */
	virtual void
	handle_table_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_table_stats_request(0);
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a TABLE-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the TABLE-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_table_stats_reply message containing the received message
	 */
	virtual void
	handle_table_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_table_stats_reply(ctlxid, msg.get_table_stats_array(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a table-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_table_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a PORT-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the PORT-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_port_stats_request message containing the received message
	 */
	virtual void
	handle_port_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_port_stats_request(msg.get_stats_flags(), msg.get_port_stats());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a PORT-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PORT-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_port_stats_reply message containing the received message
	 */
	virtual void
	handle_port_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_port_stats_reply(ctlxid, msg.get_port_stats_array(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a port-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_port_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a FLOW-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the FLOW-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_flow_stats_request message containing the received message
	 */
	virtual void
	handle_flow_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_flow_stats_request(msg.get_stats_flags(), msg.get_flow_stats());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a FLOW-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the FLOW-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_flow_stats_reply message containing the received message
	 */
	virtual void
	handle_flow_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_flow_stats_reply(ctlxid, msg.get_flow_stats_array(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a flow-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_flow_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once an AGGREGATE-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the AGGREGATE-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_aggr_stats_request message containing the received message
	 */
	virtual void
	handle_aggregate_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_aggr_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_aggr_stats_request(msg.get_stats_flags(), msg.get_aggr_stats());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once an AGGREGATE-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the AGGREGATE-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_aggregate_stats_reply message containing the received message
	 */
	virtual void
	handle_aggregate_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_aggr_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_aggr_stats_reply(ctlxid, msg.get_aggr_stats(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a aggregate-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_aggregate_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a QUEUE-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the QUEUE-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_queue_stats_request message containing the received message
	 */
	virtual void
	handle_queue_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_queue_stats_request(msg.get_stats_flags(), msg.get_queue_stats());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a QUEUE-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the QUEUE-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_queue_stats_reply message containing the received message
	 */
	virtual void
	handle_queue_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_queue_stats_reply(ctlxid, msg.get_queue_stats_array(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a queue-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_queue_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a GROUP-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_stats_request message containing the received message
	 */
	virtual void
	handle_group_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_group_stats_request(msg.get_stats_flags(), msg.get_group_stats());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a GROUP-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the GROUP-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_group_stats_reply message containing the received message
	 */
	virtual void
	handle_group_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_group_stats_reply(ctlxid, msg.get_group_stats_array(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a group-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_group_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a GROUP-DESC-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-DESC-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_desc_stats_request message containing the received message
	 */
	virtual void
	handle_group_desc_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_desc_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_group_desc_stats_request(msg.get_stats_flags());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a GROUP-DESC-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the GROUP-DESC-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_group_desc_stats_reply message containing the received message
	 */
	virtual void
	handle_group_desc_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_desc_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_group_desc_stats_reply(ctlxid, msg.get_group_desc_stats_array(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a group-desc-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_group_desc_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a GROUP-FEATURES-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-FEATURES-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_features_stats_request message containing the received message
	 */
	virtual void
	handle_group_features_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_features_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_group_features_stats_request(msg.get_stats_flags());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a GROUP-FEATURES-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the GROUP-FEATURES-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_group_features_stats_reply message containing the received message
	 */
	virtual void
	handle_group_features_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_features_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_group_features_stats_reply(ctlxid, msg.get_group_features_stats(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a group-features-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_group_features_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a TABLE-FEATURES-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the TABLE-FEATURES-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_table_features_request message containing the received message
	 */
	virtual void
	handle_table_features_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_features_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_table_features_stats_request(msg.get_stats_flags());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a TABLE-FEATURES-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the TABLE-FEATURES-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_table_features_reply message containing the received message
	 */
	virtual void
	handle_table_features_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_features_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_table_features_stats_reply(ctlxid, msg.get_tables(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a table-features-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_table_features_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a PORT-DESC-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the PORT-DESC-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_port_desc_stats_request message containing the received message
	 */
	virtual void
	handle_port_desc_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_desc_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_port_desc_stats_request(msg.get_stats_flags());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a PORT-DESC-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PORT-DESC-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_port_desc_stats_reply message containing the received message
	 */
	virtual void
	handle_port_desc_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_desc_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_port_desc_stats_reply(ctlxid, msg.get_ports(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a port-desc-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_port_desc_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a EXPERIMENTER-STATS.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: throws eBadRequestBadStat resulting in removal
	 * of msg from heap and generation of proper error message sent to controller entity.
	 *
	 * @param ctl Pointer to cofctl instance from which the EXPERIMENTER-STATS.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_experimenter_stats_request message containing the received message
	 */
	virtual void
	handle_experimenter_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter_stats_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_experimenter_stats_request(msg.get_stats_flags(), msg.get_exp_id(), msg.get_exp_type(), msg.get_body());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once an EXPERIMENTER-STATS.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the EXPERIMENTER-STATS.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter_stats_reply message containing the received message
	 */
	virtual void
	handle_experimenter_stats_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter_stats_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_experimenter_stats_reply(ctlxid, msg.get_exp_id(), msg.get_exp_type(), msg.get_body(), msg.get_stats_flags());
	};


	/**
	 * @brief	Called when a timeout event occurs for a experimenter-stats request
	 *
	 * @param ctl
	 * @param msg
	 * @param aux_id
	 */
	virtual void
	handle_experimenter_stats_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a timer has expired for a STATS.reply message.
	 *
	 * To be overwritten by derived class. Default behaviour: ignores event.
	 *
	 * @param dpt pointer to cofdpt instance where the timeout occured.
	 * @param xid transaction ID of STATS.request previously sent to data path element.
	 */
	virtual void
	handle_multipart_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type) {
		switch (msg_sub_type) {
		case rofl::openflow::OFPMP_DESC:			handle_desc_stats_reply_timeout(dpt, xid, msg_sub_type); 			break;
		case rofl::openflow::OFPMP_FLOW:			handle_flow_stats_reply_timeout(dpt, xid, msg_sub_type);			break;
		case rofl::openflow::OFPMP_AGGREGATE:		handle_aggregate_stats_reply_timeout(dpt, xid, msg_sub_type);		break;
		case rofl::openflow::OFPMP_TABLE:			handle_table_stats_reply_timeout(dpt, xid, msg_sub_type);			break;
		case rofl::openflow::OFPMP_PORT_STATS:		handle_port_stats_reply_timeout(dpt, xid, msg_sub_type);			break;
		case rofl::openflow::OFPMP_QUEUE:			handle_queue_stats_reply_timeout(dpt, xid, msg_sub_type);			break;
		case rofl::openflow::OFPMP_GROUP:			handle_group_stats_reply_timeout(dpt, xid, msg_sub_type);			break;
		case rofl::openflow::OFPMP_GROUP_DESC:		handle_group_desc_stats_reply_timeout(dpt, xid, msg_sub_type);		break;
		case rofl::openflow::OFPMP_GROUP_FEATURES:	handle_group_features_stats_reply_timeout(dpt, xid, msg_sub_type); 	break;
		case rofl::openflow::OFPMP_METER:			/* TODO */ break;
		case rofl::openflow::OFPMP_METER_CONFIG:	/* TODO */ break;
		case rofl::openflow::OFPMP_METER_FEATURES:	/* TODO */ break;
		case rofl::openflow::OFPMP_TABLE_FEATURES:	handle_table_features_stats_reply_timeout(dpt, xid, msg_sub_type);	break;
		case rofl::openflow::OFPMP_PORT_DESC:		handle_port_desc_stats_reply_timeout(dpt, xid, msg_sub_type);		break;
		default:									handle_experimenter_stats_reply_timeout(dpt, xid, msg_sub_type);	break;
		}
	};


	/**
	 * @brief	Called once a PACKET-OUT.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the PACKET-OUT.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_packet_out message containing the received message
	 */
	virtual void
	handle_packet_out(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_packet_out& msg, uint8_t aux_id = 0) = 0;


	/**
	 * @brief	Called once a PACKET-IN.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PACKET-IN.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_packet_in message containing the received message
	 */
	virtual void
	handle_packet_in(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_packet_in& msg, uint8_t aux_id = 0) = 0;


	/**
	 * @brief	Called once a BARRIER.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the BARRIER.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_barrier_request message containing the received message
	 */
	virtual void
	handle_barrier_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_barrier_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_barrier_request();
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a BARRIER.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the BARRIER.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_barrier_reply message containing the received message
	 */
	virtual void
	handle_barrier_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_barrier_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_barrier_reply(ctlxid);
	};



	/**
	 * @brief	Called once a timer has expired for a BARRIER.reply message.
	 *
	 * To be overwritten by derived class. Default behaviour: ignores event.
	 *
	 * @param dpt pointer to cofdpt instance where the timeout occured.
	 * @param xid transaction ID of BARRIER.request previously sent to data path element.
	 */
	virtual void
	handle_barrier_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once an ERROR.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl pointer to cofctl instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	virtual void
	handle_error_message(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_error& msg, uint8_t aux_id = 0) = 0;


	/**
	 * @brief	Called once an ERROR.message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	virtual void
	handle_error_message(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_error& msg, uint8_t aux_id = 0) = 0;


	/**
	 * @brief	Called once a FLOW-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the FLOW-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_flow_mod message containing the received message
	 */
	virtual void
	handle_flow_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_mod& msg, uint8_t aux_id = 0) = 0;



	/**
	 * @brief	Called once a GROUP-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the GROUP-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_group_mod message containing the received message
	 */
	virtual void
	handle_group_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_mod& msg, uint8_t aux_id = 0) = 0;



	/**
	 * @brief	Called once a TABLE-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the TABLE-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_table_mod message containing the received message
	 */
	virtual void
	handle_table_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_mod& msg, uint8_t aux_id = 0) = 0;



	/**
	 * @brief	Called once a PORT-MOD.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the PORT-MOD.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_port_mod message containing the received message
	 */
	virtual void
	handle_port_mod(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_mod& msg, uint8_t aux_id = 0) = 0;


	/**
	 * @brief	Called once a FLOW-REMOVED.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the FLOW-REMOVED.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_flow_removed message containing the received message
	 */
	virtual void
	handle_flow_removed(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_removed& msg, uint8_t aux_id = 0) = 0;


	/**
	 * @brief	Called once a PORT-STATUS.message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the PORT-STATUS.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_port_status message containing the received message
	 */
	virtual void
	handle_port_status(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_status& msg, uint8_t aux_id = 0) = 0;


	/**
	 * @brief	Called once a QUEUE-GET-CONFIG.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the QUEUE-GET-CONFIG.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_queue_get_config_request message containing the received message
	 */
	virtual void
	handle_queue_get_config_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_get_config_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_queue_get_config_request(msg.get_port_no());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a QUEUE-GET-CONFIG.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the QUEUE-GET-CONFIG.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_queue_get_config_reply message containing the received message
	 */
	virtual void
	handle_queue_get_config_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_get_config_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_queue_get_config_reply(ctlxid, msg.get_port_no(), msg.get_queues());
	};


	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_queue_get_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once an EXPERIMENTER.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl pointer to cofctl instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	virtual void
	handle_experimenter_message(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_experimenter_message(
				msg.get_experimenter_id(), msg.get_experimenter_type(), msg.get_body().somem(), msg.get_body().memlen());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once an EXPERIMENTER.message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the EXPERIMENTER.message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_experimenter message containing the received message
	 */
	virtual void
	handle_experimenter_message(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_experimenter_message(ctlxid,
				msg.get_experimenter_id(), msg.get_experimenter_type(), msg.get_body().somem(), msg.get_body().memlen());
	};


	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_experimenter_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a ROLE.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the ROLE.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_role_request message containing the received message
	 */
	virtual void
	handle_role_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_role_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_role_request(msg.get_role());
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a ROLE.reply message was received.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt pointer to cofdpt instance from which the ROLE.reply message was received.
	 * @param msg pointer to rofl::openflow::cofmsg_role_reply message containing the received message
	 */
	virtual void
	handle_role_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_role_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_role_reply(ctlxid, msg.get_role());
	};


	/**
	 * @brief	Called once a timer has expired for a ROLE.reply.
	 *
	 * To be overwritten by derived class. Default behaviour: ignores event.
	 *
	 * @param dpt pointer to cofdpt instance where the timeout occured.
	 * @param xid transaction ID of ROLE.reply message previously sent to data path element.
	 */
	virtual void
	handle_role_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once a GET-ASYNC-CONFIG.request message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the GET-ASYNC-CONFIG.request was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_async_config_request message containing the received message
	 */
	virtual void
	handle_get_async_config_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_get_async_config_request& msg, uint8_t aux_id = 0) {
		uint32_t dptxid = get_dpt().send_get_async_config_request();
		tas.add_ta(cctlxid(msg.get_xid())) = croftransaction(msg.get_type(), cctlxid(msg.get_xid()), cdptxid(dptxid));
	};


	/**
	 * @brief	Called once a GET-ASYNC-CONFIG.reply message was received from a data path element.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param dpt Pointer to cofdpt instance from which the GET-ASYNC-CONFIG.reply was received
	 * @param msg Pointer to rofl::openflow::cofmsg_get_async_config_reply message containing the received message
	 */
	virtual void
	handle_get_async_config_reply(
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_async_config_reply& msg, uint8_t aux_id = 0) {
		if (not tas.has_ta(cdptxid(msg.get_xid()))) {
			rofcore::logging::error << "[crofproxy] unable to find transaction" << std::endl;
			return;
		}
		uint32_t ctlxid = tas.get_ta(cdptxid(msg.get_xid())).get_ctlxid().get_xid();
		tas.drop_ta(cdptxid(msg.get_xid()));
		get_ctl().send_get_async_config_reply(ctlxid, msg.get_async_config());
	};


	/**
	 * @brief	Called once a timer expires for a GET-ASYNC-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_get_async_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid) {
		tas.drop_ta(cdptxid(xid));
	};


	/**
	 * @brief	Called once an SET-ASYNC-CONFIG.message was received from a controller entity.
	 *
	 * To be overwritten by derived class. Default behavior: removes msg from heap.
	 *
	 * @param ctl Pointer to cofctl instance from which the SET-ASYNC-MESSAGE.message was received
	 * @param msg Pointer to rofl::openflow::cofmsg_set_async_config message containing the received message
	 */
	virtual void
	handle_set_async_config(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_set_async_config& msg, uint8_t aux_id = 0) {
		get_dpt().send_set_async_config_message(msg.get_async_config());
	};

	/**@}*/

protected:

	/**
	 * @brief	Define data model exposed towards controller entity. Default: model received from data path.
	 */
	virtual void
	set_data_model();

private:

	/**
	 *
	 */
	void
	run_engine(
			enum crofproxy_event_t event = EVENT_NONE);

	/**
	 *
	 */
	void
	event_disconnected();

	/**
	 *
	 */
	void
	event_connected();

	/**
	 *
	 */
	void
	event_features_reply_rcvd();

	/**
	 *
	 */
	void
	event_features_request_expired();

	/**
	 *
	 */
	void
	event_get_config_reply_rcvd();

	/**
	 *
	 */
	void
	event_get_config_request_expired();


	/**
	 *
	 */
	void
	event_table_stats_reply_rcvd();

	/**
	 *
	 */
	void
	event_table_stats_request_expired();

	/**
	 *
	 */
	void
	event_table_features_stats_reply_rcvd();

	/**
	 *
	 */
	void
	event_table_features_stats_request_expired();

	/**
	 *
	 */
	void
	event_port_desc_reply_rcvd();

	/**
	 *
	 */
	void
	event_port_desc_request_expired();

	/**
	 *
	 */
	void
	event_role_reply_rcvd();

	/**
	 *
	 */
	void
	event_role_request_expired();

	/**
	 *
	 */
	void
	event_established();

};

}; // end of namespace rofcore

#endif /* CROFPROXY_HPP_ */
