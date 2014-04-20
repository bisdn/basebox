/*
 * crofproxies.hpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#ifndef CROFPROXIES_HPP_
#define CROFPROXIES_HPP_

#include <assert.h>
#include <inttypes.h>

#include <map>
#include <string>
#include <iostream>

#include <rofl/common/crofbase.h>

#include "cdptid.hpp"
#include "cctlid.hpp"
#include "crofproxy.hpp"
#include "croflog.hpp"
#include "erofcorexcp.hpp"

namespace rofcore {

class crofproxies : public rofl::crofbase, public crofproxy_env {

	static std::set<crofproxies*>			rofproxies;
	enum crofproxy::rofproxy_type_t			proxy_type;
	std::map<cctlid, crofproxy*>			cproxies;
	std::map<cdptid, crofproxy*>			dproxies;

public:

	/**
	 *
	 * @param signum
	 */
	static void
	crofproxies_sa_handler(
			int signum);

public:

	/**
	 *
	 */
	crofproxies(
			enum crofproxy::rofproxy_type_t proxy_type,
			rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap);

	/**
	 *
	 */
	virtual
	~crofproxies();

	/**
	 *
	 */
	crofproxies(
			crofproxies const& rofproxies);

	/**
	 *
	 */
	crofproxies&
	operator= (
			crofproxies const& rofproxies);

public:

	/**
	 *
	 */
	virtual void
	signal_handler(
			int signum);

	/**
	 *
	 */
	void
	clear();

	/**
	 *
	 */
	crofproxy&
	add_proxy(
			cdptid const& dpid);

	/**
	 *
	 */
	crofproxy&
	set_proxy(
			cdptid const& dpid);

	/**
	 *
	 */
	crofproxy const&
	get_proxy(
			cdptid const& dpid) const;

	/**
	 *
	 */
	void
	drop_proxy(
			cdptid const& dpid);

	/**
	 *
	 */
	bool
	has_proxy(
			cdptid const& dpid) const;

	/**
	 *
	 */
	crofproxy&
	add_proxy(
			cctlid const& dpid);

	/**
	 *
	 */
	crofproxy&
	set_proxy(
			cctlid const& dpid);

	/**
	 *
	 */
	crofproxy const&
	get_proxy(
			cctlid const& dpid) const;

	/**
	 *
	 */
	void
	drop_proxy(
			cctlid const& dpid);

	/**
	 *
	 */
	bool
	has_proxy(
			cctlid const& dpid) const;

public:

	friend std::ostream&
	operator<< (std::ostream& os, crofproxies const& rofproxies) {
		// TODO
		return os;
	};

public:

	/**
	 *
	 */
	virtual cctlid const
	rpc_connect_to_ctl(
			crofproxy const* rofproxy,
			rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap,
			int reconnect_start_timeout,
			enum rofl::csocket::socket_type_t socket_type,
			rofl::cparams const& socket_param);

	/**
	 *
	 */
	virtual void
	rpc_disconnect_from_ctl(
			crofproxy const* rofproxy,
			cctlid const& ctlid);


	/**
	 *
	 */
	virtual void
	rpc_disconnect_from_dpt(
			crofproxy const* rofproxy,
			cdptid const& dpid);

private:

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
	virtual void
	handle_dpath_close(
			rofl::crofdpt& dpt);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_features_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_features_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a timer expires for a FEATURES.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_features_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_get_config_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_config_reply& msg, uint8_t aux_id = 0);



	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_get_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_set_config& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_desc_stats_request& msg, uint8_t aux_id);


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
	virtual void
	handle_table_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_stats_request& msg, uint8_t aux_id = 0);


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
	virtual void
	handle_port_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_stats_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_stats_request& msg, uint8_t aux_id);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_aggr_stats_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_aggr_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_stats_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_stats_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_desc_stats_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_desc_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_features_stats_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_features_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_features_stats_request& msg, uint8_t aux_id = 0);


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
	virtual void
	handle_port_desc_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_desc_stats_request& msg, uint8_t aux_id = 0);


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
	virtual void
	handle_experimenter_stats_request(
			rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter_stats_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter_stats_reply& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_packet_out& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_packet_in& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_barrier_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_barrier_reply& msg, uint8_t aux_id = 0);



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
			rofl::crofdpt& dpt, uint32_t xid);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_error& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_error& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_mod& msg, uint8_t aux_id = 0);



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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_group_mod& msg, uint8_t aux_id = 0);



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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_table_mod& msg, uint8_t aux_id = 0);



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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_port_mod& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_removed& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_status& msg, uint8_t aux_id = 0);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_get_config_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_get_config_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_queue_get_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a timer expires for a GET-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_experimenter_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_role_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_role_reply& msg, uint8_t aux_id = 0);



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
			rofl::crofdpt& dpt, uint32_t xid);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_get_async_config_request& msg, uint8_t aux_id = 0);


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
			rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_async_config_reply& msg, uint8_t aux_id = 0);


	/**
	 * @brief	Called once a timer expires for a GET-ASYNC-CONFIG.reply message.
	 *
	 * Default behaviour: deletes cofdpt instance, thus effectively closing the control connection.
	 *
	 * @param dpt pointer to cofdpt instance
	 */
	virtual void
	handle_get_async_config_reply_timeout(
			rofl::crofdpt& dpt, uint32_t xid);


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
			rofl::crofctl& ctl, rofl::openflow::cofmsg_set_async_config& msg, uint8_t aux_id = 0);

	/**@}*/
};

}; // end of namespace rofcore

#endif /* ROFPROXIES_HPP_ */


