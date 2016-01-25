/*
 * cnetdevice.h
 *
 *  Created on: 24.06.2013
 *      Author: andreas
 */

#ifndef CNETDEVICE_H_
#define CNETDEVICE_H_ 1

#include <exception>
#include <map>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#ifdef __cplusplus
}
#endif

#include <rofl/common/cpacket.h>
#include <rofl/common/caddress.h>

#include <roflibs/netlink/cpacketpool.hpp>

namespace rofcore {

class cnetdev; // forward declaration, see below

class eNetDevBase 			: public std::runtime_error {
public:
	eNetDevBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class eNetDevInval 			: public eNetDevBase {
public:
	eNetDevInval(const std::string& __arg) : eNetDevBase(__arg) {};
};
class eNetDevInvalOwner 	: public eNetDevInval {
public:
	eNetDevInvalOwner(const std::string& __arg) : eNetDevInval(__arg) {};
};
class eNetDevInvalNetDev	: public eNetDevInval {
public:
	eNetDevInvalNetDev(const std::string& __arg) : eNetDevInval(__arg) {};
};
class eNetDevExists			: public eNetDevInval {
public:
	eNetDevExists(const std::string& __arg) : eNetDevInval(__arg) {};
};
class eNetDevSysCallFailed	: public eNetDevBase {
	std::string s_error;
public:
	eNetDevSysCallFailed(const std::string& __arg) : eNetDevBase(__arg) {
		std::stringstream ss;
		ss << __arg << " syscall failed (";
		ss << "errno:" << errno << " ";
		ss << strerror(errno)<< ")";
		s_error = ss.str();
	};
	virtual
	~eNetDevSysCallFailed() throw() {};
	virtual const char* what() const throw() {
		return s_error.c_str();
	};
};
class eNetDevSocket			: public eNetDevSysCallFailed {
public:
	eNetDevSocket(const std::string& __arg) : eNetDevSysCallFailed(__arg) {};
};
class eNetDevIoctl			: public eNetDevSysCallFailed {
public:
	eNetDevIoctl(const std::string& __arg) : eNetDevSysCallFailed(__arg) {};
};
class eNetDevAgain			: public eNetDevSysCallFailed {
public:
	eNetDevAgain(const std::string& __arg) : eNetDevSysCallFailed(__arg) {};
};
class eNetDevCritical		: public eNetDevBase {
public:
	eNetDevCritical(const std::string& __arg) : eNetDevBase(__arg) {};
};



class cnetdev_owner
{
protected:

	friend class cnetdev;

	std::map<std::string, cnetdev*> devs;

public:

	virtual ~cnetdev_owner() {};

	/**
	 * @brief	Enqeues a packet received on a cnetdevice to this cnetdevice_owner
	 *
	 * This method is used to enqueue new cpacket instances received on a cnetdevice
	 * on a cnetdevice_owner instance. Default behaviour of all ::enqueue() methods
	 * is deletion of all packets from heap. All ::enqueue() methods should be subscribed
	 * by a class deriving from cnetdevice_owner.
	 *
	 * @param pkt Pointer to packet allocated on heap. This pkt must be deleted by this method.
	 */
	virtual void enqueue(cnetdev *netdev, rofl::cpacket* pkt);


	/**
	 * @brief	Enqeues a vector of packets received on a cnetdevice to this cnetdevice_owner
	 *
	 * This method is used to enqueue new cpacket instances received on a cnetdevice
	 * on a cnetdevice_owner instance. Default behaviour of all ::enqueue() methods
	 * is deletion of all packets from heap. All ::enqueue() methods should be subscribed
	 * by a class deriving from cnetdevice_owner.
	 *
	 * @param pkts Vector of cpacket instances allocated on the heap. All pkts must be deleted by this method.
	 */
	virtual void enqueue(cnetdev *netdev, std::vector<rofl::cpacket*> pkts);


private:


	/**
	 * @brief	Called, once a new cnetdev instance has been created.
	 *
	 * @param netdev pointer to cnetdev instance created.
	 */
	void netdev_created(cnetdev* netdev);


	/**
	 * @brief	Called, once a cnetdev instance is removed.
	 *
	 * @param netdev pointer to cnetdev instance whose destructor was called.
	 */
	void netdev_removed(cnetdev* netdev);
};


class cnetdev
{
protected:
	std::string		 devname;
	cnetdev_owner	*netdev_owner;
	mutable unsigned int	 ifindex;
	rofl::cmacaddr	 hwaddr;

public:


	/**
	 * @brief	Constructor for class cnetdev.
	 *
	 * @param devname std::string containing name for this network device, e.g. eth0
	 * @param netdev_owner pointer to cnetdev_owner instance attached to this network device
	 */
	cnetdev(cnetdev_owner *netdev_owner, std::string const& devname, pthread_t tid = 0);

	virtual ~cnetdev();


	/**
	 * @brief	Returns std::string with network device name.
	 *
	 * @return const reference to std::string devname
	 */
	std::string const& get_devname() const;



	/**
	 * @brief	Enqueues a single rofl::cpacket instance on cnetdev.
	 *
	 * rofl::cpacket instance must have been allocated on heap and must
	 * be removed
	 */
	virtual void enqueue(rofl::cpacket *pkt);

	virtual void enqueue(std::vector<rofl::cpacket*> pkts);


	/**
	 * @brief	enable interface (set IFF_UP flag)
	 */
	virtual void enable_interface();


	/**
	 * @brief	disable interface (clear IFF_UP flag)
	 */
	virtual void disable_interface();


	/**
	 *
	 */
	virtual unsigned int get_ifindex() const;


	/**
	 *
	 */
	virtual rofl::cmacaddr get_hwaddr();


	/**
	 *
	 */
	virtual void set_hwaddr(rofl::cmacaddr const& hwaddr);
};

};

#endif /* CNETDEVICE_H_ */
