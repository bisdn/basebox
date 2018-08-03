# baseboxd

## Installation

### Fedora

Currently packages are built only for the latest supported releases. The
packages can be installed using dnf:

```
dnf -y copr enable bisdn/rofl
dnf -y copr enable bisdn/baseboxd
dnf -y install baseboxd
```

Copr repos are provided at https://copr.fedorainfracloud.org/coprs/bisdn/
The "-testing" repos contain the latest builds for testing purposes and might not be as stable as the release versions.

### Vagrant

A Vagrant file is included that let's you create a VM with all dependencies installed. This is an easy way to build baseboxd without having to worry about libraries.
```
git clone https://github.com/bisdn/basebox.git
cd basebox
git submodule update --init
vagrant up
```
More information how to use the Vagrant VM can be found [here](https://www.vagrantup.com/).

### Other distros

Currently only intallation from source is supported. To build baseboxd you need
the following dependencies installed:

* [libnl3](https://github.com/thom311/libnl) (>= 3.2.29)
* [rofl-common](https://github.com/bisdn/rofl-common) (>= 0.13.1)
* [rofl-ofdpa](https://github.com/bisdn/rofl-ofdpa) (>= 0.12)
* [gflags](https://github.com/gflags/gflags)
* [glog](https://github.com/google/glog) (>= 0.3.3)
* [protobuf](https://github.com/google/protobuf) (>= 3.4.0)
* [libgrpc](https://github.com/grpc/grpc) (>=4.0.0)
* [grpc cpp plugin](https://github.com/grpc/grpc/blob/master/examples/cpp/cpptutorial.md)

Then you can install baseboxd:

```
git clone https://github.com/bisdn/basebox.git
cd basebox
git submodule update --init
./autogen.sh
./configure
make install
```

## Usage

### bridge setup

```
ip link add type bridge
ip link set bridge0 type bridge vlan_filtering 1
ip link set bridge0 up
```

### adding ports to a bridge

```
ip link set port1 master bridge0
ip link set port2 master bridge0
```

### remove ports from a bridge

```
ip link set port1 nomaster
ip link set port2 nomaster
```

### adding vlans to bridge port

```
bridge vlan add vid 2 dev port1
bridge vlan add vid 2-4 dev port2
```

### remove vlans from bridge port

```
bridge vlan del vid 2 dev port1
bridge vlan del vid 2-4 dev port2
```

### adding a specific mac address to a bridge port

```
bridge fdb add 68:05:ca:30:63:69 dev port1 master vlan 1
```

### remove a specific mac address from a bridge port

```
bridge fdb del 68:05:ca:30:63:69 dev port1 master vlan 1
```

## High level architecture

```
.------------------.
|     netlink      |
'------------------'
.------------------.
| adaptation layer |
'------------------'
.------------------.
|     OpenFlow     |
'------------------'
```

## License

baseboxd is licensed under the [Mozilla Public License
Version 2.0](https://www.mozilla.org/en-US/MPL/2.0/). A local copy can be found
[here](COPYING)

## Notes

If you are looking for older versions of basebox(d) for controlling
ovs and xdpd, you find those in the swx branch on github [1].

[1] https://github.com/bisdn/basebox/tree/swx
