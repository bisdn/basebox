/*
 * cportconf_file.cpp
 *
 *  Created on: 16.10.2014
 *      Author: andreas
 */

#include "cportdb_file.hpp"

using namespace roflibs::ethernet;

/*static*/const  std::string cportdb_file::DEFAULT_CONFIG_FILE = std::string("/usr/local/etc/roflibs.conf");
/*static*/const 	uint16_t cportdb_file::DEFAULT_PORT_VID = 1;

void
cportdb_file::read_config()
{
	/*
	 * read configuration (for now: from libconfig++ file)
	 */

	ethcore::cconfig& config = ethcore::cconfig::get_instance();

	std::string ETHCORE_CONFIG_DPT_LIST("roflibs.ethcore.datapaths");
	if (config.exists(ETHCORE_CONFIG_DPT_LIST)) {
		for (int i = 0; i < config.lookup(ETHCORE_CONFIG_DPT_LIST).getLength(); i++) {
			try {
				libconfig::Setting& datapath = config.lookup(ETHCORE_CONFIG_DPT_LIST)[i];

				// get data path dpid
				if (not datapath.exists("dpid")) {
					continue; // as we do not know the data path dpid
				}
				rofl::cdpid dpid( (int)datapath["dpid"] );

				// get default port vid
				uint16_t default_pvid = port_vid;
				if (datapath.exists("default_pvid")) {
					default_pvid = (int)datapath["default_pvid"];
				}

				// this is the cethcore instance for this data path
				//roflibs::ethernet::cethcore& ethcore = roflibs::ethernet::cethcore::set_eth_core(dpid, default_pvid, 0, 1, 5);

				// create vlan instance for default_pvid, just in case, there are no member ports defined
				//ethcore.set_vlan(default_pvid);

				// extract all ports
				if (datapath.exists("ports")) {
					for (int j = 0; j < datapath["ports"].getLength(); ++j) {
						libconfig::Setting& port = datapath["ports"][j];

						// portno
						uint32_t portno(0);
						if (not port.exists("portno")) {
							continue; // no portno, skip this port
						} else {
							portno = (uint32_t)port["portno"];
						}

						// pvid
						int pvid(default_pvid);
						if (port.exists("pvid")) {
							pvid = (int)port["pvid"];
						}

						// devname
						std::string devname;
						if (port.exists("devname")) {
							devname = (const char*)port["devname"];
						}

						// hwaddr
						rofl::caddress_ll hwaddr;
						if (port.exists("hwaddr")) {
							hwaddr = rofl::caddress_ll((const char*)port["hwaddr"]);
						}

						// add port "portno" in untagged mode to vlan pvid
						//ethcore.set_vlan(pvid).add_port(portno, false);
						add_port_entry(dpid, portno, devname, hwaddr, pvid);
					}
				}

				// extract all vlans (= tagged port memberships)
				if (datapath.exists("vlans")) {
					for (int j = 0; j < datapath["vlans"].getLength(); ++j) {
						libconfig::Setting& vlan = datapath["vlans"][j];


						// vid
						uint16_t vid(0);
						if (not vlan.exists("vid")) {
							continue; // no vid, skip this vlan
						} else {
							vid = (int)vlan["vid"];
						}

#if 0
						// create vlan instance, just in case, there are no member ports defined
						ethcore.set_vlan(vid);
#endif

						// tagged port memberships
						if (vlan.exists("tagged")) {
							for (int k = 0; k < vlan["tagged"].getLength(); ++k) {
								libconfig::Setting& port = vlan["tagged"][k];

								// portno
								uint32_t portno = (uint32_t)port;

								set_port_entry(dpid, portno).add_vid(vid);
#if 0
								// check whether port already exists in vlan
								if (ethcore.has_vlan(vid) && ethcore.get_vlan(vid).has_port(portno)) {
									continue;
								}

								// add port "portno" in tagged mode to vlan vid
								ethcore.set_vlan(vid).add_port(portno, true);
#endif
							}
						}
					}
				}

				rofcore::logging::debug << "after config:" << std::endl << *this;


			} catch (libconfig::SettingNotFoundException& e) {

			}
		}
	}
}



