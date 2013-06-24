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

#ifdef __cplusplus
}
#endif

#include <rofl/common/cpacket.h>
#include <rofl/common/ciosrv.h>

#include <cpacketpool.h>

namespace dptmap
{

class cnetdev; // forward declaration, see below

class eNetDevBase 			: public std::exception {};
class eNetDevInval 			: public eNetDevBase {};
class eNetDevInvalOwner 	: public eNetDevInval {};
class eNetDevInvalNetDev	: public eNetDevInval {};
class eNetDevExists			: public eNetDevInval {};
class eNetDevSysCallFailed	: public eNetDevBase {};
class eNetDevSocket			: public eNetDevSysCallFailed {};
class eNetDevIoctl			: public eNetDevSysCallFailed {};
class eNetDevAgain			: public eNetDevSysCallFailed {};
class eNetDevCritical		: public eNetDevBase {};



class cnetdev_owner
{
protected:

	friend class cnetdev;

	std::map<std::string, cnetdev*> devs;

public:

	virtual ~cnetdev_owner() {};

#if 0
	/**
	 * @brief	Signals a cnetdev open message to owner.
	 *
	 * @param netdev cnetdev instance that opened its associated system port.
	 */
	virtual void netdev_open(cnetdev *netdev) {};


	/**
	 * @brief	Signals a cnetdev close message to owner.
	 *
	 * @param netdev cnetdev instance that closed its associated system port.
	 */
	virtual void netdev_close(cnetdev *netdev) {};
#endif

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



class cnetdev : public rofl::ciosrv
{
protected:

	std::string		 devname;
	cnetdev_owner	*netdev_owner;

public:


	/**
	 * @brief	Constructor for class cnetdev.
	 *
	 * @param devname std::string containing name for this network device, e.g. eth0
	 * @param netdev_owner pointer to cnetdev_owner instance attached to this network device
	 */
	cnetdev(cnetdev_owner *netdev_owner, std::string const& devname);

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
};

};

#endif /* CNETDEVICE_H_ */
