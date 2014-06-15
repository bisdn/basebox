/*
 * crofproxy.cpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#include "crofproxy.hpp"

using namespace rofcore;

/*static*/const unsigned int crofproxy::REQUIRED_MISS_SEND_LEN = 1518;

crofproxy::crofproxy(
		crofproxy_env* rofproxy_env) :
				rofproxy_env(rofproxy_env),
				cid(0),
				did(0)
{

}



crofproxy::~crofproxy()
{

}



crofproxy::crofproxy(
			crofproxy const& rofproxy)
{
	*this = rofproxy;
}



crofproxy&
crofproxy::operator= (
			crofproxy const& rofproxy)
{
	if (this == &rofproxy)
		return *this;

	cid			= rofproxy.cid;
	did			= rofproxy.did;
	cmodel		= rofproxy.cmodel;
	dmodel		= rofproxy.dmodel;
	flags		= rofproxy.flags;

	return *this;
}



void
crofproxy::signal_handler(
		int signum)
{

}





crofproxy*
crofproxy::crofproxy_factory(
		enum rofproxy_type_t proxy_type)
{
	switch (proxy_type) {
	case PROXY_TYPE_ETHCORE: {
		//return new cethcore();
	} break;
	case PROXY_TYPE_IPCORE: {
		// return new cipcore();
	} break;
	}
	throw eRofProxyNotFound();
}




void
crofproxy::run_engine(enum crofproxy_event_t event)
{
	if (EVENT_NONE != event) {
		events.push_back(event);
	}

	while (not events.empty()) {
		enum crofproxy_event_t event = events.front();
		events.pop_front();

		switch (event) {
		case EVENT_CONNECTED: {
			event_connected();
		} break;
		case EVENT_DISCONNECTED: {
			event_disconnected();
		} return;
		case EVENT_FEATURES_REPLY_RCVD: {
			event_features_reply_rcvd();
		} break;
		case EVENT_FEATURES_REQUEST_EXPIRED: {
			event_features_request_expired();
		} break;
		case EVENT_GET_CONFIG_REPLY_RCVD: {
			event_get_config_reply_rcvd();
		} break;
		case EVENT_GET_CONFIG_REQUEST_EXPIRED: {
			event_get_config_request_expired();
		} break;
		case EVENT_TABLE_STATS_REPLY_RCVD: {
			event_table_stats_reply_rcvd();
		} break;
		case EVENT_TABLE_STATS_REQUEST_EXPIRED: {
			event_table_stats_request_expired();
		} break;
		case EVENT_TABLE_FEATURES_STATS_REPLY_RCVD: {
			event_table_features_stats_reply_rcvd();
		} break;
		case EVENT_TABLE_FEATURES_STATS_REQUEST_EXPIRED: {
			event_table_features_stats_request_expired();
		} break;
		case EVENT_PORT_DESC_STATS_REPLY_RCVD: {
			event_port_desc_reply_rcvd();
		} break;
		case EVENT_PORT_DESC_STATS_REQUEST_EXPIRED: {
			event_port_desc_request_expired();
		} break;
		case EVENT_ROLE_REPLY_RCVD: {
			event_role_reply_rcvd();
		} break;
		case EVENT_ROLE_REQUEST_EXPIRED: {
			event_role_request_expired();
		} break;
		case EVENT_ESTABLISHED: {
			event_established();
		} break;
		default: {
			rofcore::logging::error << "[crofproxy] unknown event seen, internal error" << std::endl << *this;
		};
		}
	}
}



void
crofproxy::event_disconnected()
{
	if (flags.test(FLAG_ENABLE_PROXY) && (0 != cid.get_ctlid())) {
		rofcore::logging::debug << "[crofproxy] disconnecting from controller ctlid:" << cid << std::endl;
		rpc_disconnect_from_ctl(cid);
		cid = cctlid(0);
	}

	flags.reset();
	dmodel.clear();
	cmodel.clear();
}



void
crofproxy::event_connected()
{
	try {
		if (not flags.test(FLAG_ESTABLISHED)) {

			if (get_dpt().get_version() < rofl::openflow12::OFP_VERSION) {
				rofcore::logging::debug << "[crofproxy] dpid:" << did
								<< " insufficient OFP version:" << get_dpt().get_version()
								<< ", terminating connection." << std::endl;
				run_engine(EVENT_DISCONNECTED);
				return;
			}

			dmodel.set_version(get_dpt().get_version());
			role.set_version(get_dpt().get_version());

			flags.reset(); flags.set(FLAG_IDLE);

			rofcore::logging::debug << "[crofproxy] dpid:" << did
					<< " sending features-request" << std::endl;

			get_dpt().send_features_request();

			flags.set(FLAG_FEATURES_SENT);
		}

	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}



void
crofproxy::event_features_reply_rcvd()
{
	try {
		// check capabilities of data path
		switch (get_dpt().get_version()) {
		case rofl::openflow12::OFP_VERSION: {
			if (not (dmodel.get_capabilities() & rofl::openflow12::OFPC_TABLE_STATS)) {
				rofcore::logging::debug << "[crofproxy] dpid:" << did
						<< " data path does not support handling of table-stats requests, terminating connection." << std::endl;
				run_engine(EVENT_DISCONNECTED);
				return;
			}

		} break;
		case rofl::openflow13::OFP_VERSION: {
			// table-features-stats must be supported always

		} break;
		}


		rofcore::logging::debug << "[crofproxy] dpid:" << did
				<< " sending set-config" << std::endl;

		get_dpt().send_set_config_message(0, 1518);

		rofcore::logging::debug << "[crofproxy] dpid:" << did
				<< " sending get-config-request" << std::endl;

		get_dpt().send_get_config_request();

		flags.set(FLAG_GET_CONFIG_SENT);

	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}



void
crofproxy::event_features_request_expired()
{
	rofcore::logging::crit << "[crofproxy] dpid:" << did
			<< ", features-request timeout: terminating connection." << std::endl;
	run_engine(EVENT_DISCONNECTED);
}



void
crofproxy::event_get_config_reply_rcvd()
{
	try {

		if (dmodel.get_miss_send_len() != REQUIRED_MISS_SEND_LEN) {
			rofcore::logging::crit << "[crofproxy] unable to set miss-send-len parameter for dpid:"
					<< did << ", terminating connection " << std::endl;
			run_engine(EVENT_DISCONNECTED);
			return;
		}


		switch (get_dpt().get_version()) {
		case rofl::openflow12::OFP_VERSION: {
			rofcore::logging::debug << "[crofproxy] dpid:" << did
					<< " sending table-stats-request" << std::endl;

			get_dpt().send_table_stats_request(0);

			flags.set(FLAG_TABLE_SENT);

		} break;
		case rofl::openflow13::OFP_VERSION: {
			rofcore::logging::debug << "[crofproxy] dpid:" << did
					<< " sending table-features-stats-request" << std::endl;

			get_dpt().send_table_features_stats_request(0);

			flags.set(FLAG_TABLE_FEATURES_SENT);

		} break;
		}


	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}



void
crofproxy::event_get_config_request_expired()
{
	rofcore::logging::crit << "[crofproxy] dpid:" << did
			<< ", get-config-request timeout: terminating connection." << std::endl;
	run_engine(EVENT_DISCONNECTED);
}



void
crofproxy::event_table_stats_reply_rcvd()
{
	try {
		set_data_model();

		// direct jump to next stage for OF12, ports already rcvd in features-reply

		rofcore::logging::debug << "[crofproxy] dpid:" << did
				<< " sending role-request" << std::endl;

		role.set_role(rofl::openflow12::OFPCR_ROLE_MASTER);
		get_dpt().send_role_request(role);

		flags.set(FLAG_ROLE_SENT);


	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);

	} catch (eRofProxyPrerequisites& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did
				<< " insufficient resources, terminating connection" << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}



void
crofproxy::event_table_stats_request_expired()
{
	rofcore::logging::crit << "[crofproxy] dpid:" << did
			<< ", table-stats-request timeout: terminating connection." << std::endl;
	run_engine(EVENT_DISCONNECTED);
}



void
crofproxy::event_table_features_stats_reply_rcvd()
{
	try {
		set_data_model();

		rofcore::logging::debug << "[crofproxy] dpid:" << did
				<< " sending port-desc-stats-request" << std::endl;

		get_dpt().send_port_desc_stats_request(0);

		flags.set(FLAG_PORTS_DESC_SENT);


	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);

	} catch (eRofProxyPrerequisites& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did
				<< " insufficient resources, terminating connection" << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}



void
crofproxy::event_table_features_stats_request_expired()
{
	rofcore::logging::crit << "[crofproxy] dpid:" << did
			<< ", table-features-stats-request timeout: terminating connection." << std::endl;
	run_engine(EVENT_DISCONNECTED);
}



void
crofproxy::event_port_desc_reply_rcvd()
{
	try {
		rofcore::logging::debug << "[crofproxy] dpid:" << did
				<< " sending role-request" << std::endl;

		role.set_role(rofl::openflow13::OFPCR_ROLE_MASTER);
		get_dpt().send_role_request(role);

		flags.set(FLAG_ROLE_SENT);


	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}



void
crofproxy::event_port_desc_request_expired()
{
	rofcore::logging::crit << "[crofproxy] dpid:" << did
			<< ", port-desc-stats-request timeout: terminating connection." << std::endl;
	run_engine(EVENT_DISCONNECTED);
}



void
crofproxy::event_role_reply_rcvd()
{
	try {
		if (role.get_role() != rofl::openflow13::OFPCR_ROLE_MASTER) {
			rofcore::logging::crit << "[crofproxy] unable to acquire MASTER role for dpid:"
					<< did << ", terminating connection " << std::endl;
			run_engine(EVENT_DISCONNECTED);
			return;
		}

		rofcore::logging::debug << "[crofproxy] dpid:" << did
				<< " data path connection fully established" << std::endl;

		flags.set(FLAG_ESTABLISHED);

		run_engine(EVENT_ESTABLISHED);

	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}



void
crofproxy::event_role_request_expired()
{
	rofcore::logging::crit << "[crofproxy] dpid:" << did
			<< ", role-request timeout: terminating connection." << std::endl;
	run_engine(EVENT_DISCONNECTED);
}



void
crofproxy::event_established()
{
	try {
		flags.set(FLAG_ESTABLISHED);

		rofcore::logging::debug << "[crofproxy] connection from data path fully established: " << cid << std::endl;

		handle_dpath_open(get_dpt());

#if 0
		rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
		versionbitmap.add_ofp_version(get_dpt().get_version());

		/*
		 * extract socket parametrs from config file
		 */
		rofl::cparams socket_params;
		rofl::csocket::socket_type_t socket_type;
		std::string s_peeraddr("0.0.0.0");
		std::string s_peerport("6633");
		std::string s_domain("inet");
		std::string s_type("stream");
		std::string s_protocol("tcp");
		std::string s_cafile("./ca.pem");
		std::string s_certificate("./cert.pem");
		std::string s_private_key("./key.pem");
		std::string s_pswdfile("./passwd.txt");
		std::string s_verify_mode("PEER");
		std::string s_verify_depth("1");
		std::string s_ciphers("EECDH+ECDSA+AESGCM EECDH+aRSA+AESGCM EECDH+ECDSA+SHA256 EECDH+aRSA+RC4 EDH+aRSA EECDH RC4 !aNULL !eNULL !LOW !3DES !MD5 !EXP !PSK !SRP !DSS");

		bool enable_ssl = false;
		bool enable_proxy = false;


		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.enable_proxying")) {
			enable_proxy= crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.enable_proxying");
		}

		if (!enable_proxy) {
			flags.reset(FLAG_ENABLE_PROXY);
			return;
		}
		flags.set(FLAG_ENABLE_PROXY);


		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.enable_ssl")) {
			enable_ssl 	= crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.enable_ssl");
		}

		socket_type 	= (enable_ssl) ? rofl::csocket::SOCKET_TYPE_OPENSSL : rofl::csocket::SOCKET_TYPE_PLAIN;
		socket_params 	= rofl::csocket::get_default_params(socket_type);

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.peeraddr")) {
			s_peeraddr 		= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.peeraddr");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.peerport")) {
			s_peerport 		= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.peerport");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.domain")) {
			s_domain	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.domain");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.type")) {
			s_type		= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.type");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.protocol")) {
			s_protocol	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.protocol");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.ssl-cafile")) {
			s_cafile	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.ssl-cafile");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.ssl-certificate")) {
			s_certificate	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.ssl-certificate");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.ssl-private-key")) {
			s_private_key	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.ssl-private-key");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.ssl-pswdfile")) {
			s_pswdfile	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.ssl-pswdfile");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.ssl-verify-mode")) {
			s_verify_mode	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.ssl-verify-mode");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.ssl-verify-depth")) {
			s_verify_depth	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.ssl-verify-depth");
		}

		if (crofconf::get_instance().exists("sgwuctld.proxy.ipctl.socket.ssl-ciphers")) {
			s_ciphers	= (const char*)crofconf::get_instance().lookup("sgwuctld.proxy.ipctl.socket.ssl-ciphers");
		}

		socket_params.set_param(rofl::csocket::PARAM_KEY_DOMAIN).set_string(s_domain);
		socket_params.set_param(rofl::csocket::PARAM_KEY_TYPE).set_string(s_type);
		socket_params.set_param(rofl::csocket::PARAM_KEY_PROTOCOL).set_string(s_protocol);
		socket_params.set_param(rofl::csocket::PARAM_KEY_REMOTE_HOSTNAME).set_string(s_peeraddr);
		socket_params.set_param(rofl::csocket::PARAM_KEY_REMOTE_PORT).set_string(s_peerport);
		if (enable_ssl) {
			socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_CA_FILE).set_string(s_cafile);
			socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_CERT).set_string(s_certificate);
			socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_PRIVATE_KEY).set_string(s_private_key);
			socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_PRIVATE_KEY_PASSWORD).set_string(s_pswdfile);
			socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_VERIFY_MODE).set_string(s_verify_mode);
			socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_VERIFY_DEPTH).set_string(s_verify_depth);
			socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_CIPHERS).set_string(s_ciphers);
		}

		cid = rpc_connect_to_ctl(versionbitmap, 2, socket_type, socket_params);

		rofcore::logging::debug << "[crofproxy] connecting to controller ctlid:" << cid << std::endl << socket_params;
#endif


	} catch (rofl::eRofBaseNotFound& e) {
		rofcore::logging::error << "[crofproxy] dpid:" << did << " not found, aborting." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	}
}




void
crofproxy::recv_dpath_open(rofl::crofdpt& dpt)
{
	if (cdptid(0) == did) {
		throw eRofProxyBusy();
	}

	did = cdptid(dpt.get_dpid());

	run_engine(EVENT_CONNECTED);
}



void
crofproxy::recv_dpath_close(rofl::crofdpt& dpt)
{
	if (dpt.get_dpid() != did.get_dpid())
		return;

	did = cdptid(0);

	run_engine(EVENT_DISCONNECTED);
}



void
crofproxy::recv_features_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_features_reply& msg, uint8_t aux_id)
{
	rofcore::logging::debug << "[crofproxy] features-reply rcvd for dpid:" << did << std::endl;

	switch (msg.get_version()) {
	case rofl::openflow12::OFP_VERSION: {
		dmodel.set_n_buffers(msg.get_n_buffers());
		dmodel.set_n_tables(msg.get_n_tables());
		dmodel.set_auxiliary_id(0);
		dmodel.set_capabilities(msg.get_capabilities());
		dmodel.set_ports() = msg.get_ports();

	} break;
	case rofl::openflow13::OFP_VERSION: {
		dmodel.set_n_buffers(msg.get_n_buffers());
		dmodel.set_n_tables(msg.get_n_tables());
		dmodel.set_auxiliary_id(0);
		dmodel.set_capabilities(msg.get_capabilities());

		// no ports information in OF13 features reply

	} break;
	default: {
		rofcore::logging::crit << "[crofproxy] rcvd features-reply with invalid OFP version: "
				<< (int)msg.get_version() << ", terminating connection." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	} return;
	}

	run_engine(EVENT_FEATURES_REPLY_RCVD);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_features_reply(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_features_reply_timeout(
		rofl::crofdpt& dpt, uint32_t xid)
{
	run_engine(EVENT_FEATURES_REQUEST_EXPIRED);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_features_reply_timeout(dpt, xid);
	}
}



void
crofproxy::recv_get_config_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_get_config_reply& msg, uint8_t aux_id)
{
	switch (msg.get_version()) {
	case rofl::openflow12::OFP_VERSION: {
		dmodel.set_flags(msg.get_flags());
		dmodel.set_miss_send_len(msg.get_miss_send_len());

	} break;
	case rofl::openflow13::OFP_VERSION: {
		dmodel.set_flags(msg.get_flags());
		dmodel.set_miss_send_len(msg.get_miss_send_len());

	} break;
	default: {
		rofcore::logging::crit << "[crofproxy] rcvd get-config-reply with invalid OFP version: "
				<< (int)msg.get_version() << ", terminating connection." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	} return;
	}

	run_engine(EVENT_GET_CONFIG_REPLY_RCVD);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_get_config_reply(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_get_config_reply_timeout(
		rofl::crofdpt& dpt, uint32_t xid)
{
	rofcore::logging::debug << "[crofproxy] get-config-reply timeout for dpid:" << did << std::endl;
	run_engine(EVENT_GET_CONFIG_REQUEST_EXPIRED);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_get_config_reply_timeout(dpt, xid);
	}
}



void
crofproxy::recv_desc_stats_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_desc_stats_reply& msg, uint8_t aux_id)
{
	dmodel.set_desc_stats() = msg.get_desc_stats();

	if (flags.test(FLAG_ESTABLISHED)) {
		handle_desc_stats_reply(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_table_stats_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_stats_reply& msg, uint8_t aux_id)
{
	switch (msg.get_version()) {
	case rofl::openflow12::OFP_VERSION: {

#if 0
		map_tablestatsarray_to_tables(msg.set_table_stats_array(), dtables);
#endif

	} break;
	default: {
		rofcore::logging::crit << "[crofproxy] rcvd table-stats-reply with invalid OFP version: "
				<< (int)msg.get_version() << ", terminating connection." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	} return;
	}

	run_engine(EVENT_TABLE_STATS_REPLY_RCVD);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_table_stats_reply(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_table_features_stats_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_table_features_stats_reply& msg, uint8_t aux_id)
{
	switch (msg.get_version()) {
	case rofl::openflow13::OFP_VERSION: {

		dmodel.set_tables() = msg.get_tables();

	} break;
	default: {
		rofcore::logging::crit << "[crofproxy] rcvd table-features-stats-reply with invalid OFP version: "
				<< (int)msg.get_version() << ", terminating connection." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	} return;
	}

	run_engine(EVENT_TABLE_FEATURES_STATS_REPLY_RCVD);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_table_features_stats_reply(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_port_desc_stats_reply(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_desc_stats_reply& msg, uint8_t aux_id)
{
	switch (msg.get_version()) {
	case rofl::openflow13::OFP_VERSION: {

		dmodel.set_ports() = msg.get_ports();

	} break;
	default: {
		rofcore::logging::crit << "[crofproxy] rcvd port-desc-stats-reply with invalid OFP version: "
				<< (int)msg.get_version() << ", terminating connection." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	} return;
	}

	run_engine(EVENT_PORT_DESC_STATS_REPLY_RCVD);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_port_desc_stats_reply(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_multipart_reply_timeout(
		rofl::crofdpt& dpt, uint32_t xid, uint8_t msg_sub_type)
{
	rofcore::logging::debug << "[crofproxy] multipart-reply timeout for dpid:" << did << std::endl;
	if (flags.test(FLAG_PORTS_DESC_SENT)) {
		run_engine(EVENT_PORT_DESC_STATS_REQUEST_EXPIRED);
	} else
	if (flags.test(FLAG_TABLE_FEATURES_SENT)) {
		run_engine(EVENT_TABLE_FEATURES_STATS_REQUEST_EXPIRED);
	} else
	if (flags.test(FLAG_TABLE_SENT)) {
		run_engine(EVENT_TABLE_STATS_REQUEST_EXPIRED);
	} else {
		run_engine(EVENT_DISCONNECTED);
	}

	if (flags.test(FLAG_ESTABLISHED)) {
		handle_multipart_reply_timeout(dpt, xid, msg_sub_type);
	}
}



void
crofproxy::recv_port_status(
		rofl::crofdpt& dpt, rofl::openflow::cofmsg_port_status& msg, uint8_t aux_id)
{
	switch (msg.get_reason()) {
	case rofl::openflow::OFPPR_ADD: {
		dmodel.set_ports().add_port(msg.get_port().get_port_no()) = msg.get_port();
	} break;
	case rofl::openflow::OFPPR_MODIFY: {
		dmodel.set_ports().set_port(msg.get_port().get_port_no()) = msg.get_port();
	} break;
	case rofl::openflow::OFPPR_DELETE: {
		dmodel.set_ports().drop_port(msg.get_port().get_port_no());
	} break;
	default: {
		// do nothing
	} break;
	}

	rofcore::logging::debug << "[crofproxy] dpid:" << did
			<< " port-status message received:" << std::endl;

	if (flags.test(FLAG_ESTABLISHED)) {
		handle_port_status(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_role_reply(rofl::crofdpt& dpt, rofl::openflow::cofmsg_role_reply& msg, uint8_t aux_id)
{
	switch (msg.get_version()) {
	case rofl::openflow12::OFP_VERSION:
	case rofl::openflow13::OFP_VERSION: {

	} break;
	default: {
		rofcore::logging::crit << "[crofproxy] rcvd role-reply with invalid OFP version: "
				<< (int)msg.get_version() << ", terminating connection." << std::endl;
		run_engine(EVENT_DISCONNECTED);
	} return;
	}

	run_engine(EVENT_ROLE_REPLY_RCVD);
	if (flags.test(FLAG_ESTABLISHED)) {
		handle_role_reply(dpt, msg, aux_id);
	}
}



void
crofproxy::recv_role_reply_timeout(rofl::crofdpt& dpt, uint32_t xid)
{
	rofcore::logging::debug << "[crofproxy] role-reply timeout for dpid:" << did << std::endl;
	run_engine(EVENT_ROLE_REQUEST_EXPIRED);

	if (flags.test(FLAG_ESTABLISHED)) {
		handle_role_reply_timeout(dpt, xid);
	}
}



void
crofproxy::set_data_model()
{
	cmodel = dmodel;
}



