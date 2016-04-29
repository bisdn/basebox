/*
 * crofbase.cpp
 *
 *  Created on: 05.08.2014
 *      Author: andreas
 */

#include "cbasebox.hpp"
#include <assert.h>
#include "cunixenv.hpp"

using namespace basebox;

/*static*/ bool cbasebox::keep_on_running = true;
/*static*/ std::bitset<64> cbasebox::flags;
/*static*/ const std::string cbasebox::BASEBOX_LOG_FILE =
    std::string("/var/log/baseboxd.log");
/*static*/ const std::string cbasebox::BASEBOX_PID_FILE =
    std::string("/var/run/baseboxd.pid");
/*static*/ const std::string cbasebox::BASEBOX_CONFIG_FILE =
    std::string("/usr/local/etc/baseboxd.conf");

/*static*/ std::string cbasebox::script_path_dpt_open =
    std::string("/var/lib/basebox/dpath-open.sh");
/*static*/ std::string cbasebox::script_path_dpt_close =
    std::string("/var/lib/basebox/dpath-close.sh");

/*static*/
int cbasebox::run(int argc, char **argv) {
  rofl::cunixenv env_parser(argc, argv);

  /* update defaults */
  // env_parser.update_default_option("logfile", ETHCORE_LOG_FILE);
  // env_parser.update_default_option("config-file", ETHCORE_CONFIG_FILE);
  env_parser.add_option(
      rofl::coption(true, NO_ARGUMENT, 'D', "daemonize", "daemonize", ""));
  env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile",
                                      "set log-file",
                                      std::string(BASEBOX_LOG_FILE)));
  env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c',
                                      "config-file", "set config-file",
                                      std::string(BASEBOX_CONFIG_FILE)));
  env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'i', "pidfile",
                                      "set pid-file",
                                      std::string(BASEBOX_PID_FILE)));
  env_parser.add_option(
      rofl::coption(true, REQUIRED_ARGUMENT, 'p', "port", "set port", "6653"));

  // command line arguments
  env_parser.parse_args();

  // configuration file
  ethcore::cconfig::get_instance().open(env_parser.get_arg("config-file"));

/*
 * read configuration file for roflibs related configuration
 */
#if 0
	// ethcore
	roflibs::eth::cethcoredb_file& ethcoredb =
			dynamic_cast<roflibs::eth::cethcoredb_file&>( roflibs::eth::cethcoredb::get_ethcoredb("file") );
	ethcoredb.read_config(env_parser.get_arg("config-file"), std::string("baseboxd"));

	// gtpcore
	roflibs::gtp::cgtpcoredb_file& gtpcoredb =
			dynamic_cast<roflibs::gtp::cgtpcoredb_file&>( roflibs::gtp::cgtpcoredb::get_gtpcoredb("file") );
	gtpcoredb.read_config(env_parser.get_arg("config-file"), std::string("baseboxd"));
#endif

  /*
   * extract debug level
   */
  int rofl_debug = 0, core_debug = 0;
  if (ethcore::cconfig::get_instance().exists(
          "baseboxd.daemon.logging.rofl.debug")) {
    rofl_debug = (int)ethcore::cconfig::get_instance().lookup(
        "baseboxd.daemon.logging.rofl.debug");
  }
  if (env_parser.is_arg_set("debug")) {
    core_debug = rofl_debug = atoi(env_parser.get_arg("debug").c_str());
  }

  if (ethcore::cconfig::get_instance().exists(
          "baseboxd.daemon.logging.core.debug")) {
    core_debug = (int)ethcore::cconfig::get_instance().lookup(
        "baseboxd.daemon.logging.core.debug");
  }
  /*
  * extract help flag
  */
  if (env_parser.is_arg_set("help")) {
    std::cout << env_parser.get_usage((char *)"baseboxd");
    exit(0);
  }

  /*
   * extract log-file
   */
  std::string logfile;
  if (env_parser.is_arg_set("logfile")) {
    logfile = env_parser.get_arg("logfile");
  } else if (ethcore::cconfig::get_instance().exists(
                 "baseboxd.daemon.logfile")) {
    logfile = (const char *)ethcore::cconfig::get_instance().lookup(
        "baseboxd.daemon.logfile");
  } else {
    logfile = std::string(BASEBOX_LOG_FILE); // default
  }

  /*
   * extract pid-file
   */
  std::string pidfile;
  if (env_parser.is_arg_set("pidfile")) {
    pidfile = env_parser.get_arg("pidfile");
  } else if (ethcore::cconfig::get_instance().exists(
                 "baseboxd.daemon.pidfile")) {
    pidfile = (const char *)ethcore::cconfig::get_instance().lookup(
        "baseboxd.daemon.pidfile");
  } else {
    pidfile = std::string(BASEBOX_PID_FILE); // default
  }

  /*
   * extract daemonize flag
   */
  bool daemonize = false;
  if (env_parser.is_arg_set("daemonize")) {
    daemonize = atoi(env_parser.get_arg("daemonize").c_str());
  } else if (ethcore::cconfig::get_instance().exists(
                 "baseboxd.daemon.daemonize")) {
    daemonize = (bool)ethcore::cconfig::get_instance().lookup(
        "baseboxd.daemon.daemonize");
  } else {
    /* default to false */
  }

  // FIXME get this working again
  /*
//	 * daemonize
//	 */
  //	if (daemonize) {
  //		rofl::cdaemon::daemonize(pidfile, logfile);
  //	}
  rofcore::logging::set_debug_level(core_debug);

  if (daemonize) {
    rofcore::logging::notice << "[baseboxd][main] daemonizing successful"
                             << std::endl;
  }

  rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
  versionbitmap.add_ofp_version(rofl::openflow13::OFP_VERSION);
  rofcore::logging::notice << "[baseboxd][main] using OpenFlow version-bitmap:"
                           << std::endl
                           << versionbitmap;
  basebox::cbasebox &box = basebox::cbasebox::get_instance(versionbitmap);

  // base.init(/*port-table-id=*/0, /*fib-in-table-id=*/1,
  // /*fib-out-table-id=*/2, /*default-vid=*/1);

  std::stringstream portno;
  if (env_parser.is_arg_set("port")) {
    portno << env_parser.get_arg("port").c_str();
  } else if (ethcore::cconfig::get_instance().exists(
                 "baseboxd.openflow.bindport")) {
    portno << (int)ethcore::cconfig::get_instance().lookup(
        "baseboxd.openflow.bindport");
  } else {
    portno << (int)6653;
  }

  std::stringstream bindaddr;
  if (ethcore::cconfig::get_instance().exists("baseboxd.openflow.bindaddr")) {
    bindaddr << (const char *)ethcore::cconfig::get_instance().lookup(
        "baseboxd.openflow.bindaddr");
  } else {
    bindaddr << "0.0.0.0";
  }
  rofl::csockaddr baddr(AF_INET, bindaddr.str(), atoi(portno.str().c_str()));
  box.dpt_sock_listen(baddr);

#if 0
	/*
	 * extract python script to execute as business/use case specific logic
	 */
	if (ethcore::cconfig::get_instance().exists("baseboxd.usecase.script")) {
		std::string script = (const char*)ethcore::cconfig::get_instance().lookup("baseboxd.usecase.script");
		box.set_python_script(script);

		if (not script.empty()) {
			roflibs::python::cpython::get_instance().run(script);
		}
	}
#endif

#ifdef OF_DPA
  // basic init

  // start netlink
  (void)rofcore::cnetlink::get_instance();
#else
  /*
   * enable all cores by default
   */
  flags.set(FLAG_FLOWCORE);
  flags.set(FLAG_ETHCORE);
  flags.set(FLAG_IPCORE);
  flags.set(FLAG_GRECORE);
  flags.set(FLAG_GTPCORE);

  /*
   * extract flags for enabling/disabling the various roflibs
   */
  if (ethcore::cconfig::get_instance().exists(
          "baseboxd.roflibs.flowcore.enable")) {
    if (not(bool)ethcore::cconfig::get_instance().lookup(
            "baseboxd.roflibs.flowcore.enable")) {
      flags.reset(FLAG_FLOWCORE);
    }
  }
  if (ethcore::cconfig::get_instance().exists(
          "baseboxd.roflibs.ethcore.enable")) {
    if (not(bool)ethcore::cconfig::get_instance().lookup(
            "baseboxd.roflibs.ethcore.enable")) {
      flags.reset(FLAG_ETHCORE);
    }
  }
  if (ethcore::cconfig::get_instance().exists(
          "baseboxd.roflibs.ipcore.enable")) {
    if (not(bool)ethcore::cconfig::get_instance().lookup(
            "baseboxd.roflibs.ipcore.enable")) {
      flags.reset(FLAG_IPCORE);
    }
  }
  if (ethcore::cconfig::get_instance().exists(
          "baseboxd.roflibs.grecore.enable")) {
    if (not(bool)ethcore::cconfig::get_instance().lookup(
            "baseboxd.roflibs.grecore.enable")) {
      flags.reset(FLAG_GRECORE);
    }
  }
  if (ethcore::cconfig::get_instance().exists(
          "baseboxd.roflibs.gtpcore.enable")) {
    if (not(bool)ethcore::cconfig::get_instance().lookup(
            "baseboxd.roflibs.gtpcore.enable")) {
      flags.reset(FLAG_GTPCORE);
    }
  }

  /*
   * do a brief sanity check
   */
  if (flags.test(FLAG_ETHCORE) && not flags.test(FLAG_FLOWCORE)) {
    rofcore::logging::crit
        << "[baseboxd][main] must enable flowcore for using ethcore, aborting."
        << std::endl;
    return -1;
  }
  if (flags.test(FLAG_IPCORE) && not flags.test(FLAG_ETHCORE)) {
    rofcore::logging::crit
        << "[baseboxd][main] must enable ethcore for using ipcore, aborting."
        << std::endl;
    return -1;
  }
  if (flags.test(FLAG_GRECORE) && not flags.test(FLAG_IPCORE)) {
    rofcore::logging::crit
        << "[baseboxd][main] must enable ipcore for using grecore, aborting."
        << std::endl;
    return -1;
  }
  if (flags.test(FLAG_GTPCORE) && not flags.test(FLAG_IPCORE)) {
    rofcore::logging::crit
        << "[baseboxd][main] must enable ipcore for using gtpcore, aborting."
        << std::endl;
    return -1;
  }
#endif /* OF_DPA */

  while (keep_on_running) {
    try {
      // Launch main I/O loop
      struct timespec ts;
      ts.tv_sec = 1;
      ts.tv_nsec = 0;
      pselect(0, NULL, NULL, NULL, &ts, NULL);

    } catch (std::runtime_error &e) {
      std::cerr << "exception caught, what: " << e.what() << std::endl;
    }
  }

  return EXIT_SUCCESS;
}

/*static*/
void cbasebox::stop() {
  cbasebox::keep_on_running = false;
  //	thread.stop();
}

void cbasebox::handle_dpt_open(rofl::crofdpt &dpt) {

  if (rofl::openflow13::OFP_VERSION < dpt.get_version()) {
    rofcore::logging::error
        << "[cbasebox][handle_dpt_open] datapath "
        << "attached with invalid OpenFlow protocol version: "
        << (int)dpt.get_version() << std::endl;
    return;
  }

  if (sa) {
    delete sa;
    sa = NULL;
  }

#ifdef DEBUG
  dpt.set_conn(rofl::cauxid(0))
      .set_trace(true)
      .set_journal()
      .log_on_stderr(true)
      .set_max_entries(64);
  dpt.set_conn(rofl::cauxid(0))
      .set_tcp_journal()
      .log_on_stderr(true)
      .set_max_entries(16);
#endif

  rofcore::logging::debug << "[cbasebox][handle_dpt_open] dpid: "
                          << dpt.get_dpid().str() << std::endl;
  rofcore::logging::debug << "[cbasebox][handle_dpt_open] dpt: " << dpt
                          << std::endl;

#ifdef OF_DPA

#else
  dpid = dpt.get_dpid();
  dpt.flow_mod_reset();
  dpt.group_mod_reset();
#endif

  dpt.send_features_request(rofl::cauxid(0));
  dpt.send_desc_stats_request(rofl::cauxid(0), 0);
  dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);

  // todo timeout?
}

void cbasebox::handle_dpt_close(const rofl::cdptid &dptid) {

  rofcore::logging::debug << "[cbasebox][handle_dpt_close] dpid: " << dpid.str()
                          << std::endl;

// call external scripting hook
// hook_dpt_detach(dptid);

#ifdef OF_DPA
  // FIXME implemente close
  if (sa) {
    sa->handle_dpt_close(dptid);
  }
#else
  if (flags.test(FLAG_FLOWCORE)) {
    roflibs::svc::cflowcore::set_flow_core(dptid).handle_dpt_close();
  }
#if 0
	if (flags.test(FLAG_GRECORE)) {
		roflibs::gre::cgrecore::set_gre_core(dptid).handle_dpt_close();
	}
#endif
  if (flags.test(FLAG_GTPCORE)) {
    roflibs::gtp::cgtprelay::set_gtp_relay(dptid).handle_dpt_close();
    roflibs::gtp::cgtpcore::set_gtp_core(dptid).handle_dpt_close();
  }
  if (flags.test(FLAG_ETHCORE)) {
    roflibs::eth::cethcore::set_eth_core(dptid).handle_dpt_close();
  }
  if (flags.test(FLAG_IPCORE)) {
    roflibs::ip::cipcore::set_ip_core(dptid).handle_dpt_close();
  }

#if 0
	if (flags.test(FLAG_GRECORE)) {
		roflibs::gre::cgrecore::set_gre_core(dpid).clear_gre_terms_in4();
		roflibs::gre::cgrecore::set_gre_core(dpid).clear_gre_terms_in6();
	}
#endif
#endif /* OF_DPA */
}

void cbasebox::handle_conn_terminated(rofl::crofdpt &dpt,
                                      const rofl::cauxid &auxid) {
  assert(0);
}

void cbasebox::handle_conn_refused(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid) {
  assert(0);
}

void cbasebox::handle_conn_failed(rofl::crofdpt &dpt,
                                  const rofl::cauxid &auxid) {
  assert(0);
}

void cbasebox::handle_conn_negotiation_failed(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid) {
  assert(0);
}

void cbasebox::handle_conn_congestion_occured(rofl::crofdpt &dpt,
                                              const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_conn_congestion_solved(rofl::crofdpt &dpt,
                                             const rofl::cauxid &auxid) {
  rofcore::logging::crit << "[cbasebox][" << __FUNCTION__
                         << "]: XXX not implemented" << std::endl;
}

void cbasebox::handle_features_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_features_reply &msg) {
#ifdef OF_DPA
// FIXME implement first checks
#else
  if (msg.get_n_tables() < (table_id_eth_dst + 1)) {
    rofcore::logging::error
        << "[cbasebox][handle_features_reply] datapath does not support "
        << "sufficient number of tables in pipeline, need "
        << (int)(table_id_eth_dst + 1) << ", found " << (int)msg.get_n_tables()
        << std::endl;
    return;
  }

#endif
  rofcore::logging::debug << "[cbasebox][handle_features_reply] dpid: "
                          << dpt.get_dpid().str() << std::endl
                          << msg;

#ifndef OF_DPA
  dpt.send_port_desc_stats_request(rofl::cauxid(0), 0);
#endif
}

void cbasebox::handle_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_desc_stats_reply &msg) {
  rofcore::logging::debug << "[cbasebox][handle_desc_stats_reply] dpt: "
                          << std::endl
                          << dpt << std::endl
                          << msg;
}

void cbasebox::handle_packet_in(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                rofl::openflow::cofmsg_packet_in &msg) {

  rofcore::logging::debug << "[cbasebox][handle_packet_in] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;

#ifdef OF_DPA
  if (sa) {
    sa->handle_packet_in(dpt, auxid, msg);
  }
#else
  roflibs::common::openflow::ccookiebox::get_instance().handle_packet_in(
      dpt, auxid, msg);
#endif
}

void cbasebox::handle_flow_removed(rofl::crofdpt &dpt,
                                   const rofl::cauxid &auxid,
                                   rofl::openflow::cofmsg_flow_removed &msg) {

  rofcore::logging::debug << "[cbasebox][handle_flow_removed] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;

#ifdef OF_DPA
  if (sa) {
    sa->handle_flow_removed(dpt, auxid, msg);
  }
#else
  roflibs::common::openflow::ccookiebox::get_instance().handle_flow_removed(
      dpt, auxid, msg);
#endif
}

void cbasebox::handle_port_status(rofl::crofdpt &dpt, const rofl::cauxid &auxid,
                                  rofl::openflow::cofmsg_port_status &msg) {
  rofcore::logging::debug << "[cbasebox][handle_port_status] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;

// const rofl::openflow::cofport& port = msg.get_port();
// uint32_t ofp_port_no = msg.get_port().get_port_no();

#if 0
		switch (msg.get_reason()) {
		case rofl::openflow::OFPPR_ADD: {
			dpt.set_ports().set_port(msg.get_port().get_port_no()) = msg.get_port();
			// shall we do this? port-hooks refer refer to ethernet endpoints, not physical ports
			//hook_port_up(dpt, msg.get_port().get_name());

		} break;
		case rofl::openflow::OFPPR_MODIFY: {
			dpt.set_ports().set_port(msg.get_port().get_port_no()) = msg.get_port();

		} break;
		case rofl::openflow::OFPPR_DELETE: {
			dpt.set_ports().drop_port(msg.get_port().get_name());
			// shall we do this? port-hooks refer to ethernet endpoints, not physical ports
			//hook_port_down(dpt, msg.get_port().get_name());

		} break;
		default: {
			// invalid/unknown reason
		} return;
		}
#endif

#ifdef OF_DPA
// FIXME handle port status
#else
  if (flags.test(FLAG_ETHCORE) &&
      roflibs::eth::cethcore::has_eth_core(dpt.get_dptid())) {
    roflibs::eth::cethcore::set_eth_core(dpt.get_dptid())
        .handle_port_status(dpt, auxid, msg);
  }
#endif // OF_DPA
}

void cbasebox::handle_error_message(rofl::crofdpt &dpt,
                                    const rofl::cauxid &auxid,
                                    rofl::openflow::cofmsg_error &msg) {

  rofcore::logging::info << "[cbasebox][handle_error_message] dpid: "
                         << dpt.get_dpid().str()
                         << " pkt received: " << std::endl
                         << msg;

#ifdef OF_DPA
  if (sa) {
    sa->handle_error_message(dpt, auxid, msg);
  }
#else
  if (flags.test(FLAG_FLOWCORE) &&
      roflibs::svc::cflowcore::has_flow_core(dpt.get_dptid())) {
    roflibs::svc::cflowcore::set_flow_core(dpt.get_dptid())
        .handle_error_message(dpt, auxid, msg);
  }
  if (flags.test(FLAG_ETHCORE) &&
      roflibs::eth::cethcore::has_eth_core(dpt.get_dptid())) {
    roflibs::eth::cethcore::set_eth_core(dpt.get_dptid())
        .handle_error_message(dpt, auxid, msg);
  }
  if (flags.test(FLAG_IPCORE) &&
      roflibs::ip::cipcore::has_ip_core(dpt.get_dptid())) {
    roflibs::ip::cipcore::set_ip_core(dpt.get_dptid())
        .handle_error_message(dpt, auxid, msg);
  }
#if 0
	if (flags.test(FLAG_GRECORE) && roflibs::gre::cgrecore::has_gre_core(dpt.get_dptid())) {
		roflibs::gre::cgrecore::set_gre_core(dpt.get_dptid()).handle_error_message(dpt, auxid, msg);
	}
#endif
  if (flags.test(FLAG_GTPCORE) &&
      roflibs::gtp::cgtpcore::has_gtp_core(dpt.get_dptid())) {
    roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dptid())
        .handle_error_message(dpt, auxid, msg);
  }
#endif // OF_DPA
}

void cbasebox::handle_port_desc_stats_reply(
    rofl::crofdpt &dpt, const rofl::cauxid &auxid,
    rofl::openflow::cofmsg_port_desc_stats_reply &msg) {

  rofcore::logging::debug << "[cbasebox][handle_port_desc_stats_reply] dpid: "
                          << dpt.get_dpid().str()
                          << " pkt received: " << std::endl
                          << msg;

#ifdef OF_DPA

  /* init behavior */ // todo behavior based on features/descs/stats
  switch_behavior *tmp = this->sa;
  this->sa = switch_behavior_fabric::get_behavior(1, dpt);

  if (tmp) {
    delete tmp;
  }

#else

#if 0
	dpt.set_ports() = msg.get_ports();
#endif

  if (flags.test(FLAG_FLOWCORE)) {
    roflibs::svc::cflowcore::set_flow_core(dpt.get_dptid()).handle_dpt_close();
  }

  if (flags.test(FLAG_IPCORE)) {
    roflibs::ip::cipcore::set_ip_core(dpt.get_dptid(), table_id_ip_local,
                                      table_id_ip_fwd)
        .handle_dpt_close();
  }

  if (flags.test(FLAG_ETHCORE)) {
    roflibs::eth::cethcore::set_eth_core(
        dpt.get_dptid(), table_id_eth_port_membership, table_id_eth_src,
        table_id_eth_local, table_id_eth_dst, default_pvid)
        .handle_dpt_close();
  }

  if (flags.test(FLAG_GTPCORE)) {
    roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dptid(), table_id_svc_flows,
                                         table_id_ip_local,
                                         table_id_gtp_local)
        .handle_dpt_close(); // yes, same as local for cipcore

    roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dptid(),
                                           table_id_ip_local)
        .handle_dpt_close(); // yes, same as local for cipcore
  }

#if 0
	if (flags.test(FLAG_GRECORE)) {
		roflibs::gre::cgrecore::set_gre_core(dpt.get_dptid(),
												table_id_eth_port_membership,
												table_id_ip_local,
												table_id_gre_local,
												table_id_ip_fwd).handle_dpt_close();
	}
#endif

  // purge all entries, definitly
  dpt.flow_mod_reset();
  dpt.group_mod_reset();

  if (flags.test(FLAG_FLOWCORE)) {
    rofl::openflow::cofflowmod fm(dpt.get_version_negotiated());
    fm.set_table_id(table_id_svc_flows);
    fm.set_priority(0);
    fm.set_instructions().set_inst_goto_table().set_table_id(
        table_id_svc_flows + 1);
    roflibs::svc::cflowcore::set_flow_core(dpt.get_dptid())
        .add_svc_flow(0xff000000, fm);
  }

  /*
   * notify core instances
   */
  if (flags.test(FLAG_FLOWCORE)) {
    roflibs::svc::cflowcore::set_flow_core(dpt.get_dptid()).handle_dpt_open();
  }
  if (flags.test(FLAG_ETHCORE)) {
    roflibs::eth::cethcore::set_eth_core(dpt.get_dptid()).handle_dpt_open();
  }
  if (flags.test(FLAG_IPCORE)) {
    roflibs::ip::cipcore::set_ip_core(dpt.get_dptid()).handle_dpt_open();
  }
  if (flags.test(FLAG_GTPCORE)) {
    roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dptid()).handle_dpt_open();
    roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dptid()).handle_dpt_open();
  }
#if 0
	if (flags.test(FLAG_GRECORE)) {
		roflibs::gre::cgrecore::set_gre_core(dpt.get_dptid()).handle_dpt_open();
	}
#endif
#endif /* OF_DPA */
       // call external scripting hook
  // hook_dpt_attach(dpt.get_dptid());

  // test_gtp(dpt);
}

void cbasebox::handle_port_desc_stats_reply_timeout(rofl::crofdpt &dpt,
                                                    uint32_t xid) {}

#if 0 // fixme temporary disabled hooks

void
cbasebox::hook_dpt_attach(const rofl::cdptid& dptid)
{
	try {

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << rofl::crofdpt::get_dpt(dptid).get_dpid();
		envp.push_back(s_dpid.str());

		cbasebox::execute(cbasebox::script_path_dpt_open, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_dpt_attach] script execution failed" << std::endl;
	}
}



void
cbasebox::hook_dpt_detach(const rofl::cdptid& dptid)
{
	try {

		std::vector<std::string> argv;
		std::vector<std::string> envp;

		std::stringstream s_dpid;
		s_dpid << "DPID=" << rofl::crofdpt::get_dpt(dptid).get_dpid();
		envp.push_back(s_dpid.str());

		cbasebox::execute(cbasebox::script_path_dpt_close, argv, envp);

	} catch (rofl::eRofDptNotFound& e) {
		rofcore::logging::error << "[cbasebox][hook_dpt_detach] script execution failed" << std::endl;
	}
}
#endif

void cbasebox::execute(std::string const &executable,
                       std::vector<std::string> argv,
                       std::vector<std::string> envp) {
  pid_t pid = 0;

  if ((pid = fork()) < 0) {
    rofcore::logging::error
        << "[cbasebox][execute] syscall error fork(): " << errno << ":"
        << strerror(errno) << std::endl;
    return;
  }

  if (pid > 0) { // father process
    int status;
    waitpid(pid, &status, 0);
    return;
  }

  // child process

  std::vector<const char *> vctargv;
  for (std::vector<std::string>::iterator it = argv.begin(); it != argv.end();
       ++it) {
    vctargv.push_back((*it).c_str());
  }
  vctargv.push_back(NULL);

  std::vector<const char *> vctenvp;
  for (std::vector<std::string>::iterator it = envp.begin(); it != envp.end();
       ++it) {
    vctenvp.push_back((*it).c_str());
  }
  vctenvp.push_back(NULL);

  execvpe(executable.c_str(), (char *const *)&vctargv[0],
          (char *const *)&vctenvp[0]);

  exit(1); // just in case execvpe fails
}

#ifndef OF_DPA
void cbasebox::test_gtp(rofl::crofdpt &dpt) {

  uint32_t teid = 0;

  for (int i = 1; i < 10000; i++) {

    std::cerr << ">>> GTP test i=" << i << " <<<" << std::endl;

    roflibs::gtp::clabel_in4 label_in(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.10"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(i));

    roflibs::gtp::clabel_in4 label_out(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.2.2.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.2.2.20"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(i));

    roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dptid())
        .add_relay_in4(i, label_in, label_out)
        .handle_dpt_open();
  }
}

void cbasebox::test_workflow(rofl::crofdpt &dpt) {
  //	bool gre_test = false;
  bool gtp_test = false;

#if 0
	/*
	 * GRE test
	 */
	if (gre_test) {

		uint32_t term_id = 1;
		uint32_t gre_portno = 3;
		rofl::caddress_in4 laddr("10.3.3.1");
		rofl::caddress_in4 raddr("10.3.3.30");
		uint32_t gre_key = 0x11223344;

		roflibs::gre::cgrecore::set_gre_core(dpt.get_dptid()).
				add_gre_term_in4(term_id, gre_portno, laddr, raddr, gre_key);
	}

	/*
	 * GRE test
	 */
	if (0) {

		uint32_t term_id = 1;
		uint32_t gre_portno = 3;
		rofl::caddress_in4 laddr("10.1.1.1");
		rofl::caddress_in4 raddr("10.1.1.10");
		uint32_t gre_key = 0x11223344;

		roflibs::gre::cgrecore::set_gre_core(dpt.get_dptid()).
				add_gre_term_in4(term_id, gre_portno, laddr, raddr, gre_key);
	}
#endif

  /*
   * GTP test
   */
  if (gtp_test) {

    roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dptid())
        .set_termdev("tun57")
        .add_prefix_in4(
            rofcore::cprefix_in4(rofl::caddress_in4("192.168.2.1"), 24));

    roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dptid())
        .set_termdev("tun57")
        .add_prefix_in4(
            rofcore::cprefix_in4(rofl::caddress_in4("192.168.4.1"), 24));

    roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dptid())
        .set_termdev("tun57")
        .add_prefix_in6(
            rofcore::cprefix_in6(rofl::caddress_in6("3000::1"), 64));

    rofcore::logging::debug
        << roflibs::gtp::cgtprelay::set_gtp_relay(dpt.get_dptid())
               .get_termdev("tun57");
  }
  if (gtp_test) {
    roflibs::gtp::clabel_in4 label_egress(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.10"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(111111));

    roflibs::gtp::clabel_in4 label_ingress(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.10"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(111111));

    rofl::openflow::cofmatch tft_match(dpt.get_version_negotiated());
    tft_match.set_eth_type(rofl::fipv4frame::IPV4_ETHER);
    tft_match.set_ipv4_src(rofl::caddress_in4("10.2.2.20"));
    tft_match.set_ipv4_dst(rofl::caddress_in4("192.168.4.33"));

    roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dptid())
        .add_term_in4(0, "tun57", label_egress, label_ingress, tft_match);
  }
  if (gtp_test) {
    roflibs::gtp::clabel_in4 label_in(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.10"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(111111));
    roflibs::gtp::clabel_in4 label_out(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.2.2.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.2.2.20"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(222222));
    roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dptid())
        .add_relay_in4(0, label_in, label_out);
  }
  if (gtp_test) {
    roflibs::gtp::clabel_in4 label_in(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.2.2.20"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.2.2.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(222222));
    roflibs::gtp::clabel_in4 label_out(
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.1"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::caddress_gtp_in4(
            rofl::caddress_in4("10.1.1.10"),
            roflibs::gtp::cport(roflibs::gtp::cgtpcore::DEFAULT_GTPU_PORT)),
        roflibs::gtp::cteid(111111));
    roflibs::gtp::cgtpcore::set_gtp_core(dpt.get_dptid())
        .add_relay_in4(1, label_in, label_out);

    rofcore::logging::debug
        << roflibs::gtp::cgtpcore::get_gtp_core(dpt.get_dptid());
  }
}

#endif
