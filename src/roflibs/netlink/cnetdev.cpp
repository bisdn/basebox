/*
 * cnetdev.cc
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#include "cnetdev.hpp"

using namespace rofcore;

void
cnetdev_owner::netdev_created(cnetdev* netdev)
{
	if (0 == netdev) {
		throw eNetDevInvalNetDev("cnetdev_owner::netdev_created() 0 == netdev");
	}
	if (devs.find(netdev->get_devname()) != devs.end()) {
		throw eNetDevExists("cnetdev_owner::netdev_created() devname exists");
	}
	devs[netdev->get_devname()] = netdev;
}



void
cnetdev_owner::netdev_removed(cnetdev* netdev)
{
	if (0 == netdev) {
		throw eNetDevInvalNetDev("cnetdev_owner::netdev_removed() 0 == netdev");
	}
	if (devs.find(netdev->get_devname()) == devs.end()) {
		return;
	}
	devs.erase(netdev->get_devname());
}



void
cnetdev_owner::enqueue(cnetdev *netdev, rofl::cpacket* pkt)
{
	if (0 != pkt) {
		cpacketpool::get_instance().release_pkt(pkt);
	}
}



void
cnetdev_owner::enqueue(cnetdev *netdev, std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		cpacketpool::get_instance().release_pkt(*it);
	}
}




cnetdev::cnetdev(cnetdev_owner *netdev_owner, std::string const& devname, pthread_t tid) :
		rofl::ciosrv(tid),
		devname(devname),
		netdev_owner(netdev_owner),
		ifindex(0)
{
	if (0 == netdev_owner) {
		throw eNetDevInvalOwner("cnetdev::cnetdev() no netdev_owner");
	}
	netdev_owner->netdev_created(this);
}



cnetdev::~cnetdev()
{
	netdev_owner->netdev_removed(this);
}



std::string const&
cnetdev::get_devname() const
{
	return devname;
}




void
cnetdev::enqueue(rofl::cpacket *pkt)
{
	if (0 != pkt) {
		cpacketpool::get_instance().release_pkt(pkt);
	}
}



void
cnetdev::enqueue(std::vector<rofl::cpacket*> pkts)
{
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		cpacketpool::get_instance().release_pkt(*it);
	}
}



void cnetdev::enable_interface()
{
	struct ifreq ifr;
	int sd, rc;

	if ((sd = socket(AF_PACKET, SOCK_RAW, 0)) < 0)
		throw eNetDevSocket("cnetdev::enable_interface() socket() syscall failed");

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, devname.c_str());

	if ((rc = ioctl(sd, SIOCGIFINDEX, &ifr)) < 0)
		throw eNetDevIoctl("cnetdev::enable_interface() ioctl() syscall SIOCGIFINDEX failed");

	if ((rc = ioctl(sd, SIOCGIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl("cnetdev::enable_interface() ioctl() syscall SIOCGIFFLAGS failed");
	}

	ifr.ifr_flags |= IFF_UP;

	if ((rc = ioctl(sd, SIOCSIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl("cnetdev::enable_interface() ioctl() syscall SIOCSIFFLAGS failed");
	}

	close(sd);
}



void
cnetdev::disable_interface()
{
	struct ifreq ifr;
	int sd, rc;

	if ((sd = socket(AF_PACKET, SOCK_RAW, 0)) < 0)
		throw eNetDevSocket("cnetdev::disable_interface() socket() syscall failed");

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, devname.c_str());

	if ((rc = ioctl(sd, SIOCGIFINDEX, &ifr)) < 0)
		throw eNetDevIoctl("cnetdev::disable_interface() ioctl() syscall SIOCGIFINDEX failed");

	if ((rc = ioctl(sd, SIOCGIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl("cnetdev::disable_interface() ioctl() syscall SIOCGIFFLAGS failed");
	}

	ifr.ifr_flags &= ~IFF_UP;

	if ((rc = ioctl(sd, SIOCSIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl("cnetdev::disable_interface() ioctl() syscall SIOCSIFFLAGS failed");
	}

	close(sd);
}



unsigned int
cnetdev::get_ifindex()
{
	// get ifindex for devname
	int sd, rc;
	if ((sd = socket(AF_PACKET, SOCK_RAW, 0)) < 0)
		throw eNetDevSocket("cnetdev::get_ifindex() socket() syscall failed");

	struct ifreq ifr;
	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ);

	if ((rc = ioctl(sd, SIOCGIFINDEX, &ifr)) < 0)
		throw eNetDevIoctl("cnetdev::get_ifindex() ioctl() syscall SIOCGIFINDEX failed");

	close(sd);

	ifindex = ifr.ifr_ifindex;

	return ifindex;
}



rofl::cmacaddr
cnetdev::get_hwaddr()
{
	// get ifr_ifhwaddr for devname
	int sd, rc;
	if ((sd = socket(AF_PACKET, SOCK_RAW, 0)) < 0)
		throw eNetDevSocket("cnetdev::get_hwaddr() socket() syscall failed");

	struct ifreq ifr;
	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ);

	if ((rc = ioctl(sd, SIOCGIFHWADDR, &ifr)) < 0)
		throw eNetDevIoctl("cnetdev::get_hwaddr() ioctl() syscall SIOCGIFHWADDR failed");

	memcpy(hwaddr.somem(), ifr.ifr_hwaddr.sa_data, OFP_ETH_ALEN);

	close(sd);

	return hwaddr;
}



void
cnetdev::set_hwaddr(rofl::cmacaddr const& hwaddr)
{
	// get ifr_ifhwaddr for devname
	int sd, rc;
	if ((sd = socket(AF_PACKET, SOCK_RAW, 0)) < 0)
		throw eNetDevSocket("cnetdev::set_hwaddr() socket() syscall failed");

	struct ifreq ifr;
	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ);

	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(ifr.ifr_hwaddr.sa_data, hwaddr.somem(), OFP_ETH_ALEN);

	if ((rc = ioctl(sd, SIOCSIFHWADDR, &ifr)) < 0) {
		throw eNetDevIoctl("cnetdev::set_hwaddr() ioctl() syscall SIOCSIFHWADDR failed");
	}

	this->hwaddr = hwaddr;

	close(sd);
}



