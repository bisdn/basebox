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


	// extract all physical ports
	const std::string relays("relays");
	if (datapath.exists(relays.c_str())) {
		for (int j = 0; j < datapath[relays.c_str()].getLength(); ++j) {
			libconfig::Setting& relay = datapath[relays.c_str()][j];
			parse_datapath_relay(config, relay, dpid);
		}
	}

	// extract all predefined ethernet endpoints
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
		libconfig::Setting& relay,
		const rofl::cdpid& dpid)
{
	cgtprelayentry entry;

	// relay_id
	if (not relay.exists("relay_id")) {
		return;
	}
	entry.set_relay_id((unsigned int)relay["relay_id"]);

	// gtp
	if (not relay.exists("gtp")) {
		return;
	}

	/*
	 *  incoming GTP label
	 */
	if (not relay.exists("gtp.incoming")) {
		return;
	}

	// IP version (mandatory)
	if (not relay.exists("gtp.incoming.version")) {
		return;
	}
	entry.set_incoming_label().set_version((int)relay["gtp.incoming.version"]);

	// src addr (mandatory)
	if (not relay.exists("gtp.incoming.saddr")) {
		return;
	}
	entry.set_incoming_label().set_src_addr((const char*)relay["gtp.incoming.saddr"]);

	// src port (optional)
	if (relay.exists("gtp.incoming.sport")) {
		entry.set_incoming_label().set_src_port((int)relay["gtp.incoming.sport"]);
	} else {
		entry.set_incoming_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not relay.exists("gtp.incoming.daddr")) {
		return;
	}
	entry.set_incoming_label().set_dst_addr((const char*)relay["gtp.incoming.daddr"]);

	// dst port (optional)
	if (relay.exists("gtp.incoming.dport")) {
		entry.set_incoming_label().set_dst_port((int)relay["gtp.incoming.dport"]);
	} else {
		entry.set_incoming_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not relay.exists("gtp.incoming.teid")) {
		return;
	}
	entry.set_incoming_label().set_teid((unsigned int)relay["gtp.incoming.teid"]);

	// TODO: sanity checks



	/*
	 *  outgoing GTP label
	 */
	if (not relay.exists("gtp.outgoing")) {
		return;
	}

	// IP version (mandatory)
	if (not relay.exists("gtp.outgoing.version")) {
		return;
	}
	entry.set_outgoing_label().set_version((int)relay["gtp.outgoing.version"]);

	// src addr (mandatory)
	if (not relay.exists("gtp.outgoing.saddr")) {
		return;
	}
	entry.set_outgoing_label().set_src_addr((const char*)relay["gtp.outgoing.saddr"]);

	// src port (optional)
	if (relay.exists("gtp.outgoing.sport")) {
		entry.set_outgoing_label().set_src_port((int)relay["gtp.outgoing.sport"]);
	} else {
		entry.set_outgoing_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not relay.exists("gtp.outgoing.daddr")) {
		return;
	}
	entry.set_outgoing_label().set_dst_addr((const char*)relay["gtp.outgoing.daddr"]);

	// dst port (optional)
	if (relay.exists("gtp.outgoing.dport")) {
		entry.set_outgoing_label().set_dst_port((int)relay["gtp.outgoing.dport"]);
	} else {
		entry.set_outgoing_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not relay.exists("gtp.outgoing.teid")) {
		return;
	}
	entry.set_outgoing_label().set_teid((unsigned int)relay["gtp.outgoing.teid"]);

	// TODO: sanity checks



	cgtpcoredb::add_gtp_relay(dpid, entry);
}


void
cgtpcoredb_file::parse_datapath_term(
		ethcore::cconfig& config,
		libconfig::Setting& term,
		const rofl::cdpid& dpid)
{
	cgtptermentry entry;

	// term_id
	if (not term.exists("term_id")) {
		return;
	}
	entry.set_term_id((unsigned int)term["term_id"]);

	// gtp
	if (not term.exists("gtp")) {
		return;
	}


	/*
	 *  ingress GTP label
	 */
	if (not term.exists("gtp.ingress")) {
		return;
	}

	// IP version (mandatory)
	if (not term.exists("gtp.ingress.version")) {
		return;
	}
	entry.set_ingress_label().set_version((int)term["gtp.ingress.version"]);

	// src addr (mandatory)
	if (not term.exists("gtp.ingress.saddr")) {
		return;
	}
	entry.set_ingress_label().set_src_addr((const char*)term["gtp.ingress.saddr"]);

	// src port (optional)
	if (term.exists("gtp.ingress.sport")) {
		entry.set_ingress_label().set_src_port((int)term["gtp.ingress.sport"]);
	} else {
		entry.set_ingress_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not term.exists("gtp.ingress.daddr")) {
		return;
	}
	entry.set_ingress_label().set_dst_addr((const char*)term["gtp.ingress.daddr"]);

	// dst port (optional)
	if (term.exists("gtp.ingress.dport")) {
		entry.set_ingress_label().set_dst_port((int)term["gtp.ingress.dport"]);
	} else {
		entry.set_ingress_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not term.exists("gtp.ingress.teid")) {
		return;
	}
	entry.set_ingress_label().set_teid((unsigned int)term["gtp.ingress.teid"]);

	// TODO: sanity checks



	/*
	 *  egress GTP label
	 */
	if (not term.exists("gtp.egress")) {
		return;
	}

	// IP version (mandatory)
	if (not term.exists("gtp.egress.version")) {
		return;
	}
	entry.set_egress_label().set_version((int)term["gtp.egress.version"]);

	// src addr (mandatory)
	if (not term.exists("gtp.egress.saddr")) {
		return;
	}
	entry.set_egress_label().set_src_addr((const char*)term["gtp.egress.saddr"]);

	// src port (optional)
	if (term.exists("gtp.egress.sport")) {
		entry.set_egress_label().set_src_port((int)term["gtp.egress.sport"]);
	} else {
		entry.set_egress_label().set_src_port(2152);
	}

	// dst addr (mandatory)
	if (not term.exists("gtp.egress.daddr")) {
		return;
	}
	entry.set_egress_label().set_dst_addr((const char*)term["gtp.egress.daddr"]);

	// dst port (optional)
	if (term.exists("gtp.egress.dport")) {
		entry.set_egress_label().set_dst_port((int)term["gtp.egress.dport"]);
	} else {
		entry.set_egress_label().set_dst_port(2152);
	}

	// teid (mandatory)
	if (not term.exists("gtp.egress.teid")) {
		return;
	}
	entry.set_egress_label().set_teid((unsigned int)term["gtp.egress.teid"]);

	// TODO: sanity checks




	/*
	 * inject filter
	 */
	if (not term.exists("gtp.inject")) {
		return;
	}

	// IP version (mandatory)
	if (not term.exists("gtp.inject.version")) {
		return;
	}
	entry.set_inject_filter().set_version((int)term["gtp.inject.version"]);

	// dst addr (mandatory)
	if (not term.exists("gtp.inject.daddr")) {
		return;
	}
	entry.set_inject_filter().set_dst_addr((const char*)term["gtp.inject.daddr"]);

	// dst mask (optional)
	if (term.exists("gtp.inject.dmask")) {
		entry.set_inject_filter().set_dst_mask((const char*)term["gtp.inject.dmask"]);
	} else {
		entry.set_inject_filter().set_dst_mask("255.255.255.255");
	}

	// src addr (optional)
	if (term.exists("gtp.inject.saddr")) {
		entry.set_inject_filter().set_src_addr((const char*)term["gtp.inject.saddr"]);
	} else {
		entry.set_inject_filter().set_src_addr("0.0.0.0");
	}

	// src mask (optional)
	if (term.exists("gtp.inject.smask")) {
		entry.set_inject_filter().set_src_mask((const char*)term["gtp.inject.smask"]);
	} else {
		entry.set_inject_filter().set_src_mask("0.0.0.0");
	}


	cgtpcoredb::add_gtp_term(dpid, entry);
}



