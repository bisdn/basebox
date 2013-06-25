/*
 * cnetdev.cc
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#include <cnetdev.h>

using namespace dptmap;

void
cnetdev_owner::netdev_created(cnetdev* netdev)
{
	if (0 == netdev) {
		throw eNetDevInvalNetDev();
	}
	if (devs.find(netdev->get_devname()) != devs.end()) {
		throw eNetDevExists();
	}
	devs[netdev->get_devname()] = netdev;
}



void
cnetdev_owner::netdev_removed(cnetdev* netdev)
{
	if (0 == netdev) {
		throw eNetDevInvalNetDev();
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




cnetdev::cnetdev(cnetdev_owner *netdev_owner, std::string const& devname, uint32_t port_no) :
	devname(devname),
	netdev_owner(netdev_owner),
	port_no(port_no)
{
	if (0 == netdev_owner) {
		throw eNetDevInvalOwner();
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



uint32_t
cnetdev::get_port_no() const
{
	return port_no;
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
		throw eNetDevSocket();

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, devname.c_str());

	if ((rc = ioctl(sd, SIOCGIFINDEX, &ifr)) < 0)
		throw eNetDevIoctl();

	if ((rc = ioctl(sd, SIOCGIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl();
	}

	ifr.ifr_flags |= IFF_UP;

	if ((rc = ioctl(sd, SIOCSIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl();
	}

	close(sd);
}



void
cnetdev::disable_interface()
{
	struct ifreq ifr;
	int sd, rc;

	if ((sd = socket(AF_PACKET, SOCK_RAW, 0)) < 0)
		throw eNetDevSocket();

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, devname.c_str());

	if ((rc = ioctl(sd, SIOCGIFINDEX, &ifr)) < 0)
		throw eNetDevIoctl();

	if ((rc = ioctl(sd, SIOCGIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl();
	}

	ifr.ifr_flags &= ~IFF_UP;

	if ((rc = ioctl(sd, SIOCSIFFLAGS, &ifr)) < 0) {
		close(sd);
		throw eNetDevIoctl();
	}

	close(sd);
}
