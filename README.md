# What it is

baseboxd is a tiny OpenFlow controller for mapping legacy protocol
stacks on an OpenFlow datapath element. It is merely a fun project
and a playground for testing architectures and strategies for 
emulating legacy protocol stacks on OpenFlow.

In its most basic form it turns an OpenFlow pipeline into a basic
routing switch with Layer 2 and Layer 3 forwarding capabilities.
baseboxd aims towards easy extensibility for adding new protocols
like GTP, GRE, and so on that typically require basic Ethernet and 
or IP support.

# Version

There is no stable release yet. Current development version is
v0.6.0dev and v0.6 is supposed to be the first useful release
of baseboxd.

# Requirements

- A modern GNU build-system (autoconf, automake, libtool, ...)
- GNU/Linux and libc development headers
- pkg-config
- ROFL libraries installed, see [1] and [2]
- Configuration File Library (libconfig) [3]
- Network Protocol Library Suite 3.2.x (libnl) [4]
- [optional] if you want to run automatic tests (make check), libcunit and libcppunit are required.

[1] http://www.roflibs.org/ 
[2] https://github.com/bisdn/rofl-core 
[3] http://www.hyperrealm.com/libconfig/ 
[4] http://www.infradead.org/~tgr/libnl/ 

# Installation

Install the dependencies and run:

        sh# ./autogen.sh
        sh# cd build
        sh# ../configure
        sh# make
        sh# make install

# Configuration

Currently, baseboxd uses a configuration file for storing control 
and management information. An example baseboxd.conf file is available
in directory `basebox/src/baseboxd`. See below:

```
# baseboxd.conf
#
baseboxd: {

        # common daemon parameters
        #
        daemon: {
                # daemonize
                #
                daemonize = false;

                # pid-file
                #
                pidfile = "/var/run/baseboxd.pid";
                
                # log-file
                #
                logfile = "/var/log/baseboxd.log";
                
                # debug level
		#
		# EMERG=0
		# ALERT=1
		# CRIT=2
		# ERROR=3
		# WARN=4
		# NOTICE=5
		# INFO=6
		# DBG=7
		# TRACE=8
                #
                logging: {
                        rofl: {
                                debug = 0;
                        };
                        core: { 
                                debug = 0;
                        };
                };
        };

        # usecase specific configuration
        #
        usecase: {
                # business logic to be loaded into baseboxd (optional)
                #
                #script = "/usr/local/sbin/baseboxd.py";
        };

        # OpenFlow endpoint configuration
        #
        openflow: {

                # set OpenFlow version (1.3)
                #
                version = 4;

                # binding address
                #
                bindaddr = "::";

                # listening port
                #
                bindport = 6653;
        };

        # configuration for roflibs libraries
        #
        roflibs:{
                # ethcore related configuration
                #
                ethcore:{
                        datapaths = ( {
                                # datapath identifier
                                dpid = 256;

                                # default port vid
                                default_pvid = 1;

                                # port vid (untagged) and vlan port memberships (tagged) for layer (n) ports
                                ports = ( { portno=1; pvid=16; vlans=(10, 20, 128); },
                                          { portno=2; pvid=32; vlans=(10, 128); },
                                          { portno=3; pvid=48; vlans=(128); },
                                          { portno=4; pvid=64; } );

                                # predefined endpoints for layer (n+1) ethernet communication
                                ethernet = ( { devname="ep1"; vid=16; hwaddr="00:11:11:11:11:11"; },
                                             { devname="ep2"; vid=32; hwaddr="00:22:22:22:22:22"; },
                                             { devname="ep3"; vid=48; hwaddr="00:33:33:33:33:33"; } );
                        } );
                };
        };
};
```

baseboxd is capable of handling multiple datapath elements in
parallel.  Upon establishment of an OpenFlow control connection,
baseboxd creates virtual Ethernet endpoints mapped to virtual ports
using device /dev/net/tun.  Please make sure to start baseboxd
with suitable administrative rights.  You may configure these new
virtual ports according to your needs, e.g., assigning IP addresses
and routes. baseboxd will map this information down to the attached
datapath element and establish shortcuts for packet forwarding.
