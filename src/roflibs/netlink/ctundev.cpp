/*
 * ctundev.cc
 *
 *  Created on: 25.08.2014
 *      Author: andreas
 */


#include "ctundev.hpp"

using namespace rofcore;

extern int errno;


ctundev::ctundev(
		cnetdev_owner *netdev_owner,
		std::string const& devname) :
		cnetdev(netdev_owner, devname),
		fd(-1),
		devname(devname),
		thread(this)
{
	try {
		tun_open(devname);
	} catch (...) {
		thread.add_timer(CTUNDEV_TIMER_OPEN_PORT, rofl::ctimespec().expire_in(1));
	}
}



ctundev::~ctundev()
{
	tun_close();
	thread.stop();
}



void
ctundev::tun_open(std::string const& devname)
{
	try {
		struct ifreq ifr;
		int rc;

		if (fd > -1) {
			tun_close();
		}

		if ((fd = open("/dev/net/tun", O_RDWR|O_NONBLOCK)) < 0) {
			throw eTunDevSysCallFailed("[ctundev][tun_open] open()");
		}

		memset(&ifr, 0, sizeof(ifr));

		/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
		 *        IFF_TAP   - TAP device
		 *
		 *        IFF_NO_PI - Do not provide packet information
		 */
		ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
		strncpy(ifr.ifr_name, devname.c_str(), IFNAMSIZ);

		if ((rc = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
			close(fd);
			throw eTunDevSysCallFailed("[ctundev][tun_open] ioctl()");
		}

		enable_interface();

		thread.start();
		thread.add_read_fd(fd);

	} catch (std::runtime_error& e) {
		rofcore::logging::debug << "[ctundev][tun_open] exception caught: " << e.what() << std::endl;
		throw;
	}
}


void
ctundev::tun_close()
{
	try {
		if (fd == -1) {
			return;
		}

		//netdev_owner->netdev_close(this);

		disable_interface();

		thread.drop_read_fd(fd);

	} catch (std::runtime_error& e) {
		rofcore::logging::debug << "[ctundev][tun_close] exception caught: " << e.what() << std::endl;

	}

	close(fd);

	fd = -1;
}



void
ctundev::enqueue(rofl::cpacket *pkt)
{
	if (fd == -1) {
		cpacketpool::get_instance().release_pkt(pkt);
		return;
	}

	// store pkt in outgoing queue
	pout_queue.push_back(pkt);

	thread.add_write_fd(fd);
}



void
ctundev::enqueue(std::vector<rofl::cpacket*> pkts)
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

	thread.add_write_fd(fd);
}



void
ctundev::handle_read_event(rofl::cthread& thread, int fd)
{
	rofl::cpacket *pkt = (rofl::cpacket*)0;
	try {

		rofl::cmemory mem(1518);

		int rc = read(fd, mem.somem(), mem.memlen());

		// error occured (or non-blocking)
		if (rc < 0) {
			switch (errno) {
			case EAGAIN: 	throw eNetDevAgain("ctundev::handle_revent() EAGAIN, retrying later");
			default: 		throw eNetDevCritical("ctundev::handle_revent() error occured");
			}
		} else {
			pkt = cpacketpool::get_instance().acquire_pkt();

			pkt->unpack(mem.somem(), rc);

			netdev_owner->enqueue(this, pkt);
		}

	} catch (ePacketPoolExhausted& e) {

		rofcore::logging::debug << "[ctundev][handle_revent] packet pool exhausted, no idle slots available" << std::endl;

	} catch (eNetDevAgain& e) {

		rofcore::logging::debug << "[ctundev][handle_revent] EAGAIN, retrying later" << std::endl;

		cpacketpool::get_instance().release_pkt(pkt);

	} catch (eNetDevCritical& e) {

		rofcore::logging::debug << "[ctundev][handle_revent] error occured" << std::endl;

		cpacketpool::get_instance().release_pkt(pkt);

		//delete this; return;
	}
}



void
ctundev::handle_write_event(rofl::cthread& thread, int fd)
{
	rofl::cpacket * pkt = (rofl::cpacket*)0;
	try {

		while (not pout_queue.empty()) {

			pkt = pout_queue.front();
			int rc = 0;
			if ((rc = write(fd, pkt->soframe(), pkt->length())) < 0) {
				fprintf(stderr, "[rofcore][ctundev][handle_wevent] error: %d (%s)\n", errno, strerror(errno));
				switch (errno) {
				case EAGAIN: 	throw eNetDevAgain("ctundev::handle_wevent() EAGAIN, retrying later");
				default:		throw eNetDevCritical("ctundev::handle_wevent() error occured");
				}
			}

			cpacketpool::get_instance().release_pkt(pkt);

			pout_queue.pop_front();
		}

		if (pout_queue.empty()) {
			thread.drop_write_fd(fd);
		}


	} catch (eNetDevAgain& e) {

		// keep fd in wfds
		rofcore::logging::debug << "[ctundev][handle_wevent] EAGAIN, retrying later" << std::endl;

	} catch (eNetDevCritical& e) {

		rofcore::logging::debug << "[ctundev][handle_wevent] error occured" << std::endl;

		cpacketpool::get_instance().release_pkt(pkt);
	}
}



void
ctundev::handle_timeout(rofl::cthread& thread, uint32_t timer_id,
		const std::list<unsigned int>& ttypes)
{
	switch (timer_id) {
	case CTUNDEV_TIMER_OPEN_PORT: {
		try {
			tun_open(devname);
		} catch (...) {
			thread.add_timer(CTUNDEV_TIMER_OPEN_PORT, rofl::ctimespec(1));
		}
	} break;
	}
}




