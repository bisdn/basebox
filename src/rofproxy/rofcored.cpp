#include <signal.h>

#include <rofl/common/ciosrv.h>
#include <rofl/common/csocket.h>
#include <rofl/common/openflow/cofhelloelemversionbitmap.h>
#include <rofl/platform/unix/cdaemon.h>
#include <rofl/platform/unix/cunixenv.h>

#include "crofproxies.hpp"
#include "crofconf.hpp"
#include "croflog.hpp"

#define ROFCORED_LOG_FILE "/var/log/rofcored.log"
#define ROFCORED_PID_FILE "/var/run/rofcored.pid"
#define ROFCORED_CONF_FILE "/etc/rofcored/rofcored.conf"

std::string show_version();

int
main(int argc, char** argv)
{
	rofl::cunixenv env_parser(argc, argv);

	/* update defaults */
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'l', "logfile", "set log-file", std::string(ROFCORED_LOG_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'p', "pidfile", "set pid-file", std::string(ROFCORED_PID_FILE)));
	env_parser.add_option(rofl::coption(true, REQUIRED_ARGUMENT, 'c', "conffile", "set conf-file", std::string(ROFCORED_CONF_FILE)));
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT, 'D', "daemon", "daemonize", ""));
	env_parser.add_option(rofl::coption(true, NO_ARGUMENT, 'V', "version", "show version and build number", ""));

	/* parse command line options */
	env_parser.parse_args();

	/* show version */
	if (env_parser.is_arg_set("version")) {
		std::cout << show_version();
		exit(0);
	}

	/* show usage */
	if (env_parser.is_arg_set("help")) {
		std::cout << env_parser.get_usage(argv[0]);
		exit(0);
	}


	try {
		rofcore::crofconf::get_instance().open(env_parser.get_arg('c'));
	} catch (...) {};


	/* extract verbosity */
	unsigned int rofcore_verbosity = 0;
	unsigned int rofl_verbosity = 0;
	if (env_parser.is_arg_set("debug")) {
		rofcore_verbosity = atoi(env_parser.get_arg("debug").c_str());
		rofl_verbosity = atoi(env_parser.get_arg("debug").c_str());
	} else
	/* read from configuration file */{
		if (rofcore::crofconf::get_instance().exists("sgwuctld.mgmt.daemon.verbosity.saser")) {
			rofcore_verbosity = rofcore::crofconf::get_instance().lookup("sgwuctld.mgmt.daemon.verbosity.saser");
		}
		if (rofcore::crofconf::get_instance().exists("sgwuctld.mgmt.daemon.verbosity.rofl")) {
			rofl_verbosity = rofcore::crofconf::get_instance().lookup("sgwuctld.mgmt.daemon.verbosity.rofl");
		}
	}


	if (not env_parser.is_arg_set("daemon")) {

		rofl::logging::set_debug_level(rofl_verbosity);
		rofcore::logging::set_debug_level(rofcore_verbosity);

	} else {

		std::string pidfile(ROFCORED_PID_FILE);
		if (env_parser.is_arg_set("pidfile")) {
			pidfile = env_parser.get_arg("pidfile");
		} else
		if (rofcore::crofconf::get_instance().exists("sgwuctld.mgmt.daemon.pidfile")) {
			pidfile = (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.mgmt.daemon.pidfile");
		}

		std::string logfile(ROFCORED_LOG_FILE);
		if (env_parser.is_arg_set("logfile")) {
			logfile = env_parser.get_arg("logfile");
		} else
		if (rofcore::crofconf::get_instance().exists("sgwuctld.mgmt.daemon.logfile")) {
			logfile = (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.mgmt.daemon.logfile");
		}

		rofl::cdaemon::daemonize(pidfile, logfile);
		rofl::logging::set_debug_level(rofl_verbosity);
		rofcore::logging::set_debug_level(rofcore_verbosity);
		rofcore::logging::notice << "[sgwuctld][main] daemonizing successful" << std::endl;
	}


	rofl::openflow::cofhello_elem_versionbitmap versionbitmap;
	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.version")) {
		versionbitmap.add_ofp_version((int)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.version"));
	} else {
		versionbitmap.add_ofp_version(rofl::openflow12::OFP_VERSION);
	}


	enum rofcore::crofproxy::rofproxy_type_t proxy_type = rofcore::crofproxy::PROXY_TYPE_IPCORE;
	rofcore::crofproxies rofproxies(proxy_type, versionbitmap);


	/*
	 * extract socket parametrs from config file
	 */
	rofl::cparams socket_params;
	rofl::csocket::socket_type_t socket_type;
	std::string s_bind("");
	std::string s_port("6653");
	std::string s_domain("inet-any");
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

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.enable_ssl")) {
		enable_ssl 	= rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.enable_ssl");
	}

	socket_type 	= (enable_ssl) ? rofl::csocket::SOCKET_TYPE_OPENSSL : rofl::csocket::SOCKET_TYPE_PLAIN;
	socket_params 	= rofl::csocket::get_default_params(socket_type);

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.bindaddr")) {
		s_bind 		= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.bindaddr");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.bindport")) {
		s_port 		= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.bindport");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.domain")) {
		s_domain	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.domain");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.type")) {
		s_type		= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.type");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.protocol")) {
		s_protocol	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.protocol");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.ssl-cafile")) {
		s_cafile	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.ssl-cafile");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.ssl-certificate")) {
		s_certificate	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.ssl-certificate");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.ssl-private-key")) {
		s_private_key	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.ssl-private-key");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.ssl-pswdfile")) {
		s_pswdfile	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.ssl-pswdfile");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.ssl-verify-mode")) {
		s_verify_mode	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.ssl-verify-mode");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.ssl-verify-depth")) {
		s_verify_depth	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.ssl-verify-depth");
	}

	if (rofcore::crofconf::get_instance().exists("sgwuctld.openflow.socket.ssl-ciphers")) {
		s_ciphers	= (const char*)rofcore::crofconf::get_instance().lookup("sgwuctld.openflow.socket.ssl-ciphers");
	}

	socket_params.set_param(rofl::csocket::PARAM_KEY_DOMAIN).set_string(s_domain);
	socket_params.set_param(rofl::csocket::PARAM_KEY_TYPE).set_string(s_type);
	socket_params.set_param(rofl::csocket::PARAM_KEY_PROTOCOL).set_string(s_protocol);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_HOSTNAME).set_string(s_bind);
	socket_params.set_param(rofl::csocket::PARAM_KEY_LOCAL_PORT).set_string(s_port);
	if (enable_ssl) {
		socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_CA_FILE).set_string(s_cafile);
		socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_CERT).set_string(s_certificate);
		socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_PRIVATE_KEY).set_string(s_private_key);
		socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_PRIVATE_KEY_PASSWORD).set_string(s_pswdfile);
		socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_VERIFY_MODE).set_string(s_verify_mode);
		socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_VERIFY_DEPTH).set_string(s_verify_depth);
		socket_params.set_param(rofl::csocket::PARAM_SSL_KEY_CIPHERS).set_string(s_ciphers);
	}

	rofcore::logging::info << "[sgwuctld][init] listening for data path entities on " << s_bind << ":" << s_port << std::endl;

	rofproxies.rpc_listen_for_dpts(socket_type, socket_params);

	struct sigaction saction;
	memset((uint8_t*)&saction, 0, sizeof(saction));
	saction.sa_handler = &rofcore::crofproxies::crofproxies_sa_handler;
	sigaction(SIGUSR1, &saction, NULL);

	rofl::cioloop::run();

	rofl::cioloop::shutdown();

	return 0;
}


std::string
show_version(){

        std::stringstream ss("");

        ss << std::endl << "rofcored:" << std::endl;
        ss << "Version: "<< std::string(ROFCORED_VERSION) << std::endl;

#ifdef ROFCORED_BUILD
        ss << "Build: " << ROFCORED_BUILD << std::endl;
        ss << "Compiled in branch: " << ROFCORED_BRANCH << std::endl;
        ss << "Detailed build information:" << ROFCORED_DESCRIBE << std::endl;
#endif

        return ss.str();
}


