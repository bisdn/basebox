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

### Other distros

Currently only installation from source is supported. To build baseboxd you
need the following dependencies installed:

#### Build system

* [meson](https://gitub.com/mesonbuild/meson/)
* [clang-format](https://clang.llvm.org/docs/ClangFormat.html) for code formatting
* [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) for linting (optionally)

#### Libraries

* [libnl3](https://github.com/thom311/libnl) (> 3.4)
* [rofl-common](https://github.com/bisdn/rofl-common) (>= 0.13.1)
* [rofl-ofdpa](https://github.com/bisdn/rofl-ofdpa) (>= 1.0.0)
* [gflags](https://github.com/gflags/gflags)
* [glog](https://github.com/google/glog) (>= 0.3.3)
* [protobuf](https://github.com/google/protobuf) (>= 3.5.0)
* [grpc](https://github.com/grpc/grpc)
* [grpc cpp plugin](https://github.com/grpc/grpc/blob/master/examples/cpp/cpptutorial.md)

Then you can install baseboxd:

```
git clone --recursive https://github.com/bisdn/basebox.git
cd basebox
meson build
ninja -C build
```

### Docker

Running baseboxd as a service inside of a Docker container is currently under
heavy development, but will probably be fully supported in the future. At the
moment there are three builds available that you can directly pull from
https://hub.docker.com/u/bisdn.

- bisdn/baseboxd: contains the latest baseboxd package and its dependencies from
  our stable repositories. 
- bisdn/baseboxd-testing: contains the latest baseboxd package and its
  dependencies from our testing repositories. 
- bisdn/baseboxd-source: contains baseboxd and its dependencies build from
  source from the master branch of this repository.

To get the containers you can simply pull them with:

```
docker pull bisdn/baseboxd
docker pull bisdn/baseboxd-testing
docker pull bisdn/baseboxd-source
```

Since baseboxd needs to create kernel network interfaces, the container needs to
run in privilged mode. To allow the direct orchestration of baseboxd via netlink
events from the host itself, the container additionally needs to run within the
host network namespace. All configuration parameters for baseboxd can be handed
over to baseboxd via environment variables while starting the container.

To run the current stable version of baseboxd on port 6654 with verbose logging
and log output to stderr you can start it with:

```
docker run -d --privileged --network=host -e FLAGS_port="6654" -e GLOG_logtostderr="1" -e GLOG_v="2" bisdn/baseboxd
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
