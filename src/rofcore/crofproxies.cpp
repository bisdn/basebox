/*
 * crofproxies.cpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#include "crofproxies.hpp"

using namespace rofcore;

/*static*/std::set<crofproxies*> crofproxies::rofproxies;

/*static*/void
crofproxies::crofproxies_sa_handler(
		int signum)
{
	for (std::set<crofproxies*>::iterator
			it = crofproxies::rofproxies.begin(); it != crofproxies::rofproxies.end(); ++it) {
		(*(*it)).signal_handler(signum);
	}
}


crofproxies::crofproxies(
		enum crofproxy::rofproxy_type_t proxy_type,
		rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap) :
				rofl::crofbase(versionbitmap),
				proxy_type(proxy_type)
{
	crofproxies::rofproxies.insert(this);
}



crofproxies::~crofproxies()
{
	crofproxies::rofproxies.erase(this);
	clear();
}



crofproxies::crofproxies(
		crofproxies const& rofproxies)
{
	*this = rofproxies;
}



crofproxies&
crofproxies::operator= (
		crofproxies const& rofproxies)
{
	if (this == &rofproxies)
		return *this;

	clear();
	// TODO

	return *this;
}



void
crofproxies::signal_handler(
		int signum)
{
	for (std::map<cdptid, crofproxy*>::iterator
			it = dproxies.begin(); it != dproxies.end(); ++it) {
		it->second->signal_handler(signum);
	}
}



void
crofproxies::clear()
{
	for (std::map<cdptid, crofproxy*>::iterator
			it = dproxies.begin(); it != dproxies.end(); ++it) {
		delete it->second;
	}
	dproxies.clear();
}



crofproxy&
crofproxies::add_proxy(
		cdptid const& dpid)
{
	if (dproxies.find(dpid) != dproxies.end()) {
		delete dproxies[dpid];
	}
	dproxies[dpid] = crofproxy::crofproxy_factory(proxy_type);
	return dynamic_cast<crofproxy&>( *(dproxies[dpid]) );
}



crofproxy&
crofproxies::set_proxy(
		cdptid const& dpid)
{
	if (dproxies.find(dpid) == dproxies.end()) {
		dproxies[dpid] = crofproxy::crofproxy_factory(proxy_type);
	}
	return dynamic_cast<crofproxy&>( *(dproxies[dpid]) );
}



crofproxy const&
crofproxies::get_proxy(
		cdptid const& dpid) const
{
	if (dproxies.find(dpid) == dproxies.end()) {
		throw eRofProxyNotFound();
	}
	return dynamic_cast<crofproxy const&>( *(dproxies.at(dpid)) );
}



void
crofproxies::drop_proxy(
		cdptid const& dpid)
{
	if (dproxies.find(dpid) == dproxies.end()) {
		return;
	}
	delete dproxies[dpid];
	dproxies.erase(dpid);
}



bool
crofproxies::has_proxy(
		cdptid const& dpid) const
{
	return (not (dproxies.find(dpid) == dproxies.end()));
}



crofproxy&
crofproxies::add_proxy(
		cctlid const& ctlid)
{
	if (cproxies.find(ctlid) != cproxies.end()) {
		delete cproxies[ctlid];
	}
	cproxies[ctlid] = crofproxy::crofproxy_factory(proxy_type);
	return dynamic_cast<crofproxy&>( *(cproxies[ctlid]) );
}



crofproxy&
crofproxies::set_proxy(
		cctlid const& ctlid)
{
	if (cproxies.find(ctlid) == cproxies.end()) {
		cproxies[ctlid] = crofproxy::crofproxy_factory(proxy_type);
	}
	return dynamic_cast<crofproxy&>( *(cproxies[ctlid]) );
}



crofproxy const&
crofproxies::get_proxy(
		cctlid const& ctlid) const
{
	if (cproxies.find(ctlid) == cproxies.end()) {
		throw eRofProxyNotFound();
	}
	return dynamic_cast<crofproxy const&>( *(cproxies.at(ctlid)) );
}



void
crofproxies::drop_proxy(
		cctlid const& ctlid)
{
	if (cproxies.find(ctlid) == cproxies.end()) {
		return;
	}
	delete cproxies[ctlid];
	cproxies.erase(ctlid);
}



bool
crofproxies::has_proxy(
		cctlid const& ctlid) const
{
	return (not (cproxies.find(ctlid) == cproxies.end()));
}



void
crofproxies::handle_dpath_open(
		rofl::crofdpt& dpt)
{
	rofl::logging::info << "[crofproxies] dpt attached with dpid: " << dpt.get_dpid_s() << std::endl;

	try {
		add_proxy(cdptid(dpt.get_dpid())).handle_dpath_open(dpt);
	} catch (eRofProxyBusy& e) {

		rofl::logging::error << "[crofproxies] unable to create node instance" << std::endl;
		throw;

	}
}



void
crofproxies::handle_dpath_close(
		rofl::crofdpt& dpt)
{
	rofl::logging::info << "[crofproxies] dpt detached with dpid: " << dpt.get_dpid_s() << std::endl;

	if (has_proxy(cdptid(dpt.get_dpid()))) {
		set_proxy(cdptid(dpt.get_dpid())).handle_dpath_close(dpt);
		drop_proxy(cdptid(dpt.get_dpid()));
	}
}



void
crofproxies::handle_features_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_features_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_features_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_features_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_features_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_features_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_features_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_features_reply_timeout(dpt, xid);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_get_config_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_get_config_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_get_config_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_get_config_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_config_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_get_config_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_get_config_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_get_config_reply_timeout(dpt, xid);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_desc_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_desc_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_desc_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_desc_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_desc_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_desc_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_table_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_table_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_table_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_table_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_table_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_port_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_port_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_port_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_port_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_port_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_flow_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_flow_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_flow_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_flow_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_aggregate_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_aggr_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_aggregate_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_aggregate_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_aggr_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_aggregate_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_queue_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_queue_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_queue_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_queue_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_group_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_group_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_group_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_group_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_group_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_group_desc_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_group_desc_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_group_desc_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_group_desc_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_desc_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_group_desc_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_group_features_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_group_features_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_group_features_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_group_features_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_group_features_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_group_features_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_table_features_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_table_features_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_table_features_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_table_features_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_features_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_table_features_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_port_desc_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_port_desc_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_port_desc_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_port_desc_stats_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_desc_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_port_desc_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_experimenter_stats_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter_stats_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_experimenter_stats_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_experimenter_stats_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter_stats_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_experimenter_stats_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_packet_out(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_packet_out& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_packet_out(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_packet_in(rofl::crofdpt& dpt, rofl::openflow::cofmsg_packet_in& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_packet_in(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_barrier_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_barrier_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_barrier_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_barrier_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_barrier_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_barrier_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_barrier_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_barrier_reply_timeout(dpt, xid);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_error_message(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_error& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_error_message(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_error_message(rofl::crofdpt& dpt, rofl::openflow::cofmsg_error& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_error_message(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_flow_mod(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_flow_mod& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_flow_mod(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_group_mod(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_group_mod& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_group_mod(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_table_mod(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_table_mod& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_table_mod(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_port_mod(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_port_mod& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_port_mod(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_flow_removed(rofl::crofdpt& dpt, rofl::openflow::cofmsg_flow_removed& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_flow_removed(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_port_status(rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_status& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_port_status(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_queue_get_config_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_queue_get_config_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_queue_get_config_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_queue_get_config_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_queue_get_config_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_queue_get_config_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_queue_get_config_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_queue_get_config_reply_timeout(dpt, xid);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_set_config(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_set_config& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_set_config(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_experimenter_message(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_experimenter_message(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_experimenter_message(rofl::crofdpt& dpt, rofl::openflow::cofmsg_experimenter& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_experimenter_message(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_experimenter_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_experimenter_timeout(dpt, xid);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}




void
crofproxies::handle_role_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_role_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_role_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_role_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_role_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_role_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_role_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_role_reply_timeout(dpt, xid);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_get_async_config_request(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_get_async_config_request& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_get_async_config_request(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_get_async_config_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_async_config_reply& msg, uint8_t aux_id)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_get_async_config_reply(dpt, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}
}



void
crofproxies::handle_get_async_config_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	try {
		set_proxy(cdptid(dpt.get_dpid())).handle_get_async_config_reply_timeout(dpt, xid);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for dpid:" << dpt.get_dpid() << " not found" << std::endl;
	}

}



void
crofproxies::handle_set_async_config(
		rofl::crofctl& ctl, rofl::openflow::cofmsg_set_async_config& msg, uint8_t aux_id)
{
	try {
		set_proxy(cctlid(ctl.get_ctlid())).handle_set_async_config(ctl, msg, aux_id);
	} catch (eRofProxyNotFound& e) {
		rofl::logging::error << "[crofproxies] crofproxy instance for ctlid:" << ctl.get_ctlid() << " not found" << std::endl;
	}
}


