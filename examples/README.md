# Examples

This folder serves the purpose of adding example configuration, using the iproute2 and systemd-networkds tools.

These examples are meant to show the features supported in baseboxd. So that configuration can be understood more easily, each test contains the setup to be used in the SDN controller and the servers.

## Physical Topology

In terms of physical topology, these examples are designed to be used with a single switch and a single physical server. Namespaces are used to improve the flexibility in designing the examples, emulating
two (or more) physical hosts; and showing off the features of baseboxd.

## Overview

### bridging

These examples describe the process of adding a VLAN-aware bridge interface to the Linux environment, and attaching ports to this bridge.

* [server](./bridging/01-server)
* [controller](./bridging/01-controller)

### routing

As a L3-enabled SDN controller, baseboxd can be configured for routing purposes. Examples in this folder are added to show how IP addresses (IPv4 and IPv6) and routes can be attached to certain interfaces. 

#### IPv4
* [server](./routing/IPv4/01-server)
* [controller](./routing/IPv4/01-controller)

#### IPv6
* [server](./routing/IPv6/01-server)
* [controller](./routing/IPv6/01-controller)


### Free Range Routing

[Free Range Routing](https://github.com/FRRouting/frr), or FRR, is a routing agent for Linux/Unix plaforms, that aggregates several routing daemons, like bgpd and ospfd. 

Currently FRR support in baseboxd, is only tested with bgpd, and in the Free Range Routing folder, a few configuration examples can be seen. Installing FRR can be done, on Fedora and CentOS distributions, by enabling the BISDN FRR copr repository, available [here](https://copr.fedorainfracloud.org/coprs/bisdn/frr/), or the testing version [here](https://copr.fedorainfracloud.org/coprs/bisdn/frr-testing/).

### networkd

As examples for configuration using systemd-networkd, the files are available under the networkd folder. These files will generate a configuration for 2 leaf - 2 spines setup for VxLAN-EVPN deployment.
