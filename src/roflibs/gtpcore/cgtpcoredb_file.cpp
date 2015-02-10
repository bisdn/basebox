/*
 * cgtpcoredb_file.cpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#include "cgtpcoredb_file.hpp"

using namespace roflibs::gtp;

/*static*/const  std::string cgtpcoredb_file::DEFAULT_CONFIG_FILE = std::string("/usr/local/etc/roflibs.conf");
/*static*/const  std::string cgtpcoredb_file::GTPCORE_CONFIG_DPT_LIST("roflibs.gtpcore.datapaths");

void
cgtpcoredb_file::read_config(const std::string& config_file, const std::string& prefix)
{
	if (config_file.empty()) {
		this->config_file = cgtpcoredb_file::DEFAULT_CONFIG_FILE;
	} else {
		this->config_file = config_file;
	}

	/*
	 * read configuration (here: from libconfig++ file)
	 */

	ethcore::cconfig& config = ethcore::cconfig::get_instance();
	config.open(this->config_file);

	std::string path = prefix + std::string(".") + cgtpcoredb_file::GTPCORE_CONFIG_DPT_LIST;
	if (config.exists(path)) {
		for (int i = 0; i < config.lookup(path).getLength(); i++) {
			try {
				parse_datapath(config, config.lookup(path)[i]);
			} catch (libconfig::SettingNotFoundException& e) {}
		}
	}

	rofcore::logging::debug << "[cgtpcoredb][file] config:" << std::endl << *this;
}


void
cgtpcoredb_file::parse_datapath(ethcore::cconfig& config, libconfig::Setting& datapath)
{
	// get data path dpid
	if (not datapath.exists("dpid")) {
		return; // as we do not know the data path dpid
	}
	rofl::cdpid dpid( (int)datapath["dpid"] );


	// extract all GTP relays
	const std::string relays("relays");
	if (datapath.exists(relays.c_str())) {
		for (int j = 0; j < datapath[relays.c_str()].getLength(); ++j) {
			libconfig::Setting& relay = datapath[relays.c_str()][j];
			parse_datapath_relay(config, relay, dpid);
		}
	}

	// extract all predefined GTP termination points
	const std::string terms("terms");
	if (datapath.exists(terms.c_str())) {
		for (int j = 0; j < datapath[terms.c_str()].getLength(); ++j) {
			libconfig::Setting& term = datapath[terms.c_str()][j];
			parse_datapath_term(config, term, dpid);
		}
	}
}


void
cgtpcoredb_file::parse_datapath_relay(
		ethcore::cconfig& config,
		libconfig::Setting& s_relay,
		const rofl::cdpid& dpid)
{
	cgtprelayentry entry;

	// relay_id
	if (not s_relay.exists("relay_id")) {
		return;
	}
	entry.set_relay_id((unsigned int)s_relay["relay_id"]);

	// gtp
	if (not s_relay.exists("gtp")) {
		return;
	}
	libconfig::Setting& s_relay_gtp = s_relay["gtp"];


	/*
	 *  incoming GTP label
	 */
	if (not s_relay_gtp.exists("incoming")) {
		return;
	}
	libconfig::Setting& s_relay_gtp_incoming = s_relay_gtp["incoming"];

	// IP version (mandatory)
	if (not s_relay_gtp_incoming.exists("version")) {
		return;
	}
	entry.set_incoming_label().set_version((int)s_relay_gtp_incoming["version"]);

	// src addr (mandatory)
	if (not s_relay_gtp_incoming.exists("saddr")) {
		return;
	}
	entry.set_incoming_label().set_src_addr((const char*)s_relay_gtp_incoming["saddr"]);

	// src port (optional)
	if (s_relay_gtp_incoming.exists("sport")) {
		entry.set_incoming_label().set_src_port((int)s_relay_gtp_incoming["sport"]);
	} else {
		entry.set_incoming_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not s_relay_gtp_incoming.exists("daddr")) {
		return;
	}
	entry.set_incoming_label().set_dst_addr((const char*)s_relay_gtp_incoming["daddr"]);

	// dst port (optional)
	if (s_relay_gtp_incoming.exists("dport")) {
		entry.set_incoming_label().set_dst_port((int)s_relay_gtp_incoming["dport"]);
	} else {
		entry.set_incoming_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not s_relay_gtp_incoming.exists("teid")) {
		return;
	}
	entry.set_incoming_label().set_teid((unsigned int)s_relay_gtp_incoming["teid"]);

	// TODO: sanity checks



	/*
	 *  outgoing GTP label
	 */
	if (not s_relay_gtp.exists("outgoing")) {
		return;
	}
	libconfig::Setting& s_relay_gtp_outgoing = s_relay_gtp["outgoing"];

	// IP version (mandatory)
	if (not s_relay_gtp_outgoing.exists("version")) {
		return;
	}
	entry.set_outgoing_label().set_version((int)s_relay_gtp_outgoing["version"]);

	// src addr (mandatory)
	if (not s_relay_gtp_outgoing.exists("saddr")) {
		return;
	}
	entry.set_outgoing_label().set_src_addr((const char*)s_relay_gtp_outgoing["saddr"]);

	// src port (optional)
	if (s_relay_gtp_outgoing.exists("sport")) {
		entry.set_outgoing_label().set_src_port((int)s_relay_gtp_outgoing["sport"]);
	} else {
		entry.set_outgoing_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not s_relay_gtp_outgoing.exists("daddr")) {
		return;
	}
	entry.set_outgoing_label().set_dst_addr((const char*)s_relay_gtp_outgoing["daddr"]);

	// dst port (optional)
	if (s_relay_gtp_outgoing.exists("dport")) {
		entry.set_outgoing_label().set_dst_port((int)s_relay_gtp_outgoing["dport"]);
	} else {
		entry.set_outgoing_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not s_relay_gtp_outgoing.exists("teid")) {
		return;
	}
	entry.set_outgoing_label().set_teid((unsigned int)s_relay_gtp_outgoing["teid"]);

	// TODO: sanity checks


	std::cerr << ">>> " << entry.str() << " <<<" << std::endl;
	cgtpcoredb::add_gtp_relay(dpid, entry);
}


void
cgtpcoredb_file::parse_datapath_term(
		ethcore::cconfig& config,
		libconfig::Setting& s_term,
		const rofl::cdpid& dpid)
{
	cgtptermentry entry;

	// term_id
	if (not s_term.exists("term_id")) {
		return;
	}
	entry.set_term_id((unsigned int)s_term["term_id"]);

	// gtp
	if (not s_term.exists("gtp")) {
		return;
	}
	libconfig::Setting& s_term_gtp = s_term["gtp"];


	/*
	 *  ingress GTP label
	 */
	if (not s_term_gtp.exists("ingress")) {
		return;
	}
	libconfig::Setting& s_term_gtp_ingress = s_term_gtp["ingress"];

	// IP version (mandatory)
	if (not s_term_gtp_ingress.exists("version")) {
		return;
	}
	entry.set_ingress_label().set_version((int)s_term_gtp_ingress["version"]);

	// src addr (mandatory)
	if (not s_term_gtp_ingress.exists("saddr")) {
		return;
	}
	entry.set_ingress_label().set_src_addr((const char*)s_term_gtp_ingress["saddr"]);

	// src port (optional)
	if (s_term_gtp_ingress.exists("sport")) {
		entry.set_ingress_label().set_src_port((int)s_term_gtp_ingress["sport"]);
	} else {
		entry.set_ingress_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not s_term_gtp_ingress.exists("daddr")) {
		return;
	}
	entry.set_ingress_label().set_dst_addr((const char*)s_term_gtp_ingress["daddr"]);

	// dst port (optional)
	if (s_term_gtp_ingress.exists("dport")) {
		entry.set_ingress_label().set_dst_port((int)s_term_gtp_ingress["dport"]);
	} else {
		entry.set_ingress_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not s_term_gtp_ingress.exists("teid")) {
		return;
	}
	entry.set_ingress_label().set_teid((unsigned int)s_term_gtp_ingress["teid"]);

	// TODO: sanity checks



	/*
	 *  egress GTP label
	 */
	if (not s_term_gtp.exists("egress")) {
		return;
	}
	libconfig::Setting& s_term_gtp_egress = s_term_gtp["egress"];

	// IP version (mandatory)
	if (not s_term_gtp_egress.exists("version")) {
		return;
	}
	entry.set_egress_label().set_version((int)s_term_gtp_egress["version"]);

	// src addr (mandatory)
	if (not s_term_gtp_egress.exists("saddr")) {
		return;
	}
	entry.set_egress_label().set_src_addr((const char*)s_term_gtp_egress["saddr"]);

	// src port (optional)
	if (s_term_gtp_egress.exists("sport")) {
		entry.set_egress_label().set_src_port((int)s_term_gtp_egress["sport"]);
	} else {
		entry.set_egress_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not s_term_gtp_egress.exists("daddr")) {
		return;
	}
	entry.set_egress_label().set_dst_addr((const char*)s_term_gtp_egress["daddr"]);

	// dst port (optional)
	if (s_term_gtp_egress.exists("dport")) {
		entry.set_egress_label().set_dst_port((int)s_term_gtp_egress["dport"]);
	} else {
		entry.set_egress_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not s_term_gtp_egress.exists("teid")) {
		return;
	}
	entry.set_egress_label().set_teid((unsigned int)s_term_gtp_egress["teid"]);

	// TODO: sanity checks



	/*
	 * inject filter
	 */
	if (not s_term.exists("inject")) {
		return;
	}
	libconfig::Setting& s_term_inject = s_term["inject"];

	// devname
	if (not s_term_inject.exists("devname")) {
		return;
	}
	entry.set_inject_filter().set_devname((const char*)s_term_inject["devname"]);

	// IP version (mandatory)
	if (not s_term_inject.exists("version")) {
		return;
	}
	entry.set_inject_filter().set_version((int)s_term_inject["version"]);

	// route (mandatory)
	if (not s_term_inject.exists("route")) {
		return;
	}
	entry.set_inject_filter().set_route((const char*)s_term_inject["route"]);

	// netmask (mandatory)
	if (s_term_inject.exists("netmask")) {
		entry.set_inject_filter().set_netmask((const char*)s_term_inject["netmask"]);
	} else {
		entry.set_inject_filter().set_netmask("255.255.255.255");
	}




	std::cerr << ">>> " << entry.str() << " <<<" << std::endl;
	cgtpcoredb::add_gtp_term(dpid, entry);
}


