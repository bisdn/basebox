/*
 * ctapdev.cc
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#include <ctapdev.h>

using namespace dptmap;

extern int errno;


ctapdev::ctapdev(
		cnetdev_owner *netdev_owner,
		std::string const& devname,
		rofl::cmacaddr const& hwaddr) :
		cnetdev(netdev_owner, devname),
		fd(-1),
		devname(devname),
		hwaddr(hwaddr)
{
	try {
		tap_open(devname, hwaddr);
	} catch (...) {
		port_open_timer_id = register_timer(CTAPDEV_TIMER_OPEN_PORT, rofl::ctimespec(0, 500000/* 500ms */))
	}
}



ctapdev::~ctapdev()
{
	tap_close();
}



void
ctapdev::tap_open(std::string const& devname, rofl::cmacaddr const& hwaddr)
{
	try {
		struct ifreq ifr;
		int rc;

		if (fd > -1) {
			tap_close();
		}

		if ((fd = open("/dev/net/tun", O_RDWR|O_NONBLOCK)) < 0) {
			throw eTapDevOpenFailed();
		}

		memset(&ifr, 0, sizeof(ifr));

		/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
		 *        IFF_TAP   - TAP device
		 *
		 *        IFF_NO_PI - Do not provide packet information
		 */
		ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
		strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ);

		if ((rc = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
			close(fd);
			throw eTapDevIoctlFailed();
		}

		set_hwaddr(hwaddr);

		enable_interface();

		//netdev_owner->netdev_open(this);

		register_filedesc_r(fd);


	} catch (eTapDevOpenFailed& e) {

		fprintf(stderr, "ctapdev::tap_open() open() failed: dev[%s] (%d:%s)\n",
				devname.c_str(), errno, strerror(errno));

		throw eNetDevCritical();

	} catch (eTapDevIoctlFailed& e) {

		fprintf(stderr, "ctapdev::tap_open() ioctl() failed: dev[%s] (%d:%s)\n",
				devname.c_str(), errno, strerror(errno));

		throw eNetDevCritical();

	}
}


void
ctapdev::tap_close()
{
	if (fd == -1) {
		return;
	}

	//netdev_owner->netdev_close(this);

	disable_interface();

	deregister_filedesc_r(fd);

	close(fd);

	fd = -1;
}



void
ctapdev::enqueue(rofl::cpacket *pkt)
{
	if (fd == -1) {
		cpacketpool::get_instance().release_pkt(pkt);
		return;
	}

	// store pkt in outgoing queue
	pout_queue.push_back(pkt);

	register_filedesc_w(fd);
}



void
ctapdev::enqueue(std::vector<rofl::cpacket*> pkts)
{
	if (fd == -1) {
		for (std::vector<rofl::cpacket*>::iterator
				it = pkts.begin(); it != pkts.end(); ++it) {
			cpacketpool::get_instance().release_pkt((*it));
		}
		return;
	}

	// store pkts in outgoing queue
	for (std::vector<rofl::cpacket*>::iterator
			it = pkts.begin(); it != pkts.end(); ++it) {
		pout_queue.push_back(*it);
	}

	register_filedesc_w(fd);
}



void
ctapdev::handle_revent(int fd)
{
	rofl::cpacket *pkt = (rofl::cpacket*)0;
	try {

		rofl::cmemory mem(1518);

		int rc = read(fd, mem.somem(), mem.memlen());

		// error occured (or non-blocking)
		if (rc < 0) {
			switch (errno) {
			case EAGAIN: 	throw eNetDevAgain();
			default: 		throw eNetDevCritical();
			}
		} else {
			pkt = cpacketpool::get_instance().acquire_pkt();

			pkt->unpack(/*ignore port_no*/0, mem.somem(), rc);

			netdev_owner->enqueue(this, pkt);
		}

	} catch (ePacketPoolExhausted& e) {

		fprintf(stderr, "ctapdev::handle_revent() packet pool exhausted, no idle slots available\n");

	} catch (eNetDevAgain& e) {

		fprintf(stderr, "ctapdev::handle_revent() (%d:%s) => "
				"retry later\n", errno, strerror(errno));

		cpacketpool::get_instance().release_pkt(pkt);

	} catch (eNetDevCritical& e) {

		fprintf(stderr, "ctapdev::handle_revent() critical error (%d:%s): "
				"calling self-destruction\n", errno, strerror(errno));

		cpacketpool::get_instance().release_pkt(pkt);

		delete this; return;
	}
}



void
ctapdev::handle_wevent(int fd)
{
	rofl::cpacket * pkt = (rofl::cpacket*)0;
	try {

		while (not pout_queue.empty()) {

			pkt = pout_queue.front();
			int rc = 0;
			if ((rc = write(fd, pkt->soframe(), pkt->framelen())) < 0) {
				switch (errno) {
				case EAGAIN: 	throw eNetDevAgain();
				default:		throw eNetDevCritical();
				}
			}

			cpacketpool::get_instance().release_pkt(pkt);

			pout_queue.pop_front();
		}

		if (pout_queue.empty()) {
			deregister_filedesc_w(fd);
		}


	} catch (eNetDevAgain& e) {

		// keep fd in wfds
		fprintf(stderr, "ctapdev::handle_wevent() (%d:%s) => "
				"retry later\n", errno, strerror(errno));

	} catch (eNetDevCritical& e) {

		fprintf(stderr, "ctapdev::handle_wevent() critical error (%d:%s): "
				"calling self-destruction\n", errno, strerror(errno));

		cpacketpool::get_instance().release_pkt(pkt);

		delete this; return;
	}
}



void
ctapdev::handle_timeout(int opaque, void* data = (void*)0)
{
	switch (opaque) {
	case CTAPDEV_TIMER_OPEN_PORT: {
		try {
			tap_open(devname, hwaddr);
		} catch (...) {
			port_open_timer_id = register_timer(CTAPDEV_TIMER_OPEN_PORT, rofl::ctimespec(0, 500000/* 500ms */))
		}
	} break;
	}
}


