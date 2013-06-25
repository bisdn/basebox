/*
 * ctapdev.h
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#ifndef CTAPDEV_H_
#define CTAPDEV_H_ 1


#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#ifdef __cplusplus
}
#endif

#include <list>

#include <cnetdev.h>



namespace dptmap
{

class eTapDevBase 			: public eNetDevBase {};
class eTapDevSysCallFailed 	: public eTapDevBase {};
class eTapDevOpenFailed		: public eTapDevSysCallFailed {};
class eTapDevIoctlFailed	: public eTapDevSysCallFailed {};

class ctapdev : public cnetdev
{
	int 							fd; 			// tap device file descriptor
	std::list<rofl::cpacket*> 		pout_queue;		// queue of outgoing packets

public:

	ctapdev(cnetdev_owner *netdev_owner, std::string const& devname, uint32_t port_no);


	virtual ~ctapdev();


	/**
	 * @brief	Enqueues a single rofl::cpacket instance on cnetdev.
	 *
	 * rofl::cpacket instance must have been allocated on heap and must
	 * be removed
	 */
	virtual void enqueue(rofl::cpacket *pkt);

	virtual void enqueue(std::vector<rofl::cpacket*> pkts);


protected:

	/**
	 * @brief	open tapX device
	 */
	void
	tap_open(std::string const& devname);


	/**
	 * @brief	close tapX device
	 *
	 */
	void
	tap_close();


	/**
	 * @brief	handle read events on file descriptor
	 */
	virtual void
	handle_revent(int fd);


	/**
	 * @brief	handle write events on file descriptor
	 */
	virtual void
	handle_wevent(int fd);
};

}; // end of namespace vmcore

#endif /* CTAPDEV_H_ */
