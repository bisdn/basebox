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

[1](http://www.roflibs.org/)
[2](https://github.com/bisdn/rofl-core)
[3](http://www.hyperrealm.com/libconfig/)
[4](http://www.infradead.org/~tgr/libnl/)

# Howto use it



