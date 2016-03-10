/*
 * cportconf_file.cpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#include "cethcoredb_file.hpp"

using namespace roflibs::eth;

/*static*/ const std::string cethcoredb_file::DEFAULT_CONFIG_FILE =
    std::string("/usr/local/etc/roflibs.conf");
/*static*/ const uint16_t cethcoredb_file::DEFAULT_PORT_VID = 1;
/*static*/ const std::string
    cethcoredb_file::ETHCORE_CONFIG_DPT_LIST("roflibs.ethcore.datapaths");

void cethcoredb_file::read_config(const std::string &config_file,
                                  const std::string &prefix) {
  if (config_file.empty()) {
    this->config_file = cethcoredb_file::DEFAULT_CONFIG_FILE;
  } else {
    this->config_file = config_file;
  }

  /*
   * read configuration (here: from libconfig++ file)
   */

  ethcore::cconfig &config = ethcore::cconfig::get_instance();
  config.open(this->config_file);

  std::string path =
      prefix + std::string(".") + cethcoredb_file::ETHCORE_CONFIG_DPT_LIST;
  if (config.exists(path)) {
    for (int i = 0; i < config.lookup(path).getLength(); i++) {
      try {
        parse_datapath(config, config.lookup(path)[i]);
      } catch (libconfig::SettingNotFoundException &e) {
      }
    }
  }

  rofcore::logging::debug << "[cethcoredb][file] config:" << std::endl << *this;
}

void cethcoredb_file::parse_datapath(ethcore::cconfig &config,
                                     libconfig::Setting &datapath) {
  // get data path dpid
  if (not datapath.exists("dpid")) {
    return; // as we do not know the data path dpid
  }
  rofl::cdpid dpid((int)datapath["dpid"]);

  // get default port vid
  uint16_t default_pvid = port_vid;
  if (datapath.exists("default_pvid")) {
    default_pvid = (int)datapath["default_pvid"];
  }
  set_default_pvid(dpid, default_pvid);

  // extract all physical ports
  const std::string phyports("ports");
  if (datapath.exists(phyports.c_str())) {
    for (int j = 0; j < datapath[phyports.c_str()].getLength(); ++j) {
      libconfig::Setting &port = datapath[phyports.c_str()][j];
      parse_datapath_phy_port(config, port, dpid, default_pvid);
    }
  }

  // extract all predefined ethernet endpoints
  const std::string ethendpnts("ethernet");
  if (datapath.exists(ethendpnts.c_str())) {
    for (int j = 0; j < datapath[ethendpnts.c_str()].getLength(); ++j) {
      libconfig::Setting &endpnt = datapath[ethendpnts.c_str()][j];
      parse_datapath_eth_endpnt(config, endpnt, dpid, default_pvid);
    }
  }
}

void cethcoredb_file::parse_datapath_phy_port(ethcore::cconfig &config,
                                              libconfig::Setting &port,
                                              const rofl::cdpid &dpid,
                                              uint16_t default_pvid) {
  // portno
  uint32_t portno(0);
  if (not port.exists("portno")) {
    return; // no portno, skip this port
  } else {
    portno = (uint32_t)port["portno"];
  }

  // pvid
  int pvid(default_pvid);
  if (port.exists("pvid")) {
    pvid = (int)port["pvid"];
  }

  cportentry &portentry = add_port_entry(dpid, portno, pvid);

  // extract all vlans (= tagged port memberships)
  if (port.exists("vlans")) {
    for (int j = 0; j < port["vlans"].getLength(); ++j) {
      portentry.add_vid((int)port["vlans"][j]);
    }
  }
}

void cethcoredb_file::parse_datapath_eth_endpnt(ethcore::cconfig &config,
                                                libconfig::Setting &endpnt,
                                                const rofl::cdpid &dpid,
                                                uint16_t default_pvid) {
  // devname
  std::string devname;
  if (not endpnt.exists("devname")) {
    rofcore::logging::error
        << "[cethcoredb][file] no devname found for ethernet entry"
        << std::endl;
    return;
  } else {
    devname = (const char *)endpnt["devname"];
  }

  // vid
  int vid(default_pvid);
  if (endpnt.exists("vid")) {
    vid = (int)endpnt["vid"];
  }

  // hwaddr
  rofl::caddress_ll hwaddr;
  if (not endpnt.exists("hwaddr")) {
    rofcore::logging::error
        << "[cethcoredb][file] no hwaddr found for ethernet entry" << std::endl;
    return;
  } else {
    hwaddr = rofl::caddress_ll((const char *)endpnt["hwaddr"]);
  }

  add_eth_entry(dpid, devname, hwaddr, vid);
}
