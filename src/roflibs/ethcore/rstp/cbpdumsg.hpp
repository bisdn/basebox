/*
 * cbpdumsg.hpp
 *
 *  Created on: 03.01.2015
 *      Author: andreas
 */

#ifndef CBPDUMSG_HPP_
#define CBPDUMSG_HPP_

#include <ostream>
#include <exception>

#include <rofl/common/cmemory.h>
#include <rofl/common/caddress.h>

#include "crstp_p.h"

namespace roflibs {
namespace eth {
namespace rstp {

namespace exception {

/**
 * @brief Base exception for class cbpdumsg
 */
class eBpduBase : public std::runtime_error {
public:
	eBpduBase(const std::string& __arg) : std::runtime_error(__arg) {};
};

/**
 * @brief Invalid parameter
 */
class eBpduInval : public std::runtime_error {
public:
	eBpduInval(const std::string& __arg) : std::runtime_error(__arg) {};
};

/**
 * @brief Invalid BPDU type
 */
class eBpduType : public std::runtime_error {
public:
	eBpduType(const std::string& __arg) : std::runtime_error(__arg) {};
};

}; // end of namespace exception

/**
 * @class cbpdumsg
 * @brief Base class for bridge PDUs
 */
class cbpdumsg {
public:

	/**
	 * @brief cbpdumsg desstructor
	 */
	virtual
	~cbpdumsg()
	{};

	/**
	 * @brief cbpdumsg default constructor
	 */
	cbpdumsg() :
		llc_ssap(0),
		llc_dsap(0),
		llc_control(0),
		bpdu_protid(0),
		bpdu_version(0),
		bpdu_type(0)
	{};

	/**
	 * @brief copy constructor
	 */
	cbpdumsg(
			const cbpdumsg& msg)
	{ *this = msg; };

public:

	/**
	 * @brief Assignment operator
	 */
	cbpdumsg&
	operator= (
			const cbpdumsg& bpdu) {
		if (this == &bpdu)
			return *this;
		eth_src = bpdu.eth_src;
		eth_dst = bpdu.eth_dst;
		llc_ssap = bpdu.llc_ssap;
		llc_dsap = bpdu.llc_dsap;
		llc_control = bpdu.llc_control;
		bpdu_protid = bpdu.bpdu_protid;
		bpdu_version = bpdu.bpdu_version;
		bpdu_type = bpdu.bpdu_type;
		return *this;
	};

public:

	/**
	 * @brief Returns buffer length required for packing this instance.
	 */
	virtual size_t
	length() const
	{ return sizeof(bpdu_common_hdr_t); };

	/**
	 * @brief Packs this instance' internal state into the given buffer.
	 */
	virtual void
	pack(
			uint8_t* buf,
			size_t buflen);

	/**
	 * @brief Unpacks a given buffer and stores its content in this instance.
	 */
	virtual void
	unpack(
			uint8_t* buf,
			size_t buflen);

public:

	/**
	 * @name Methods granting access to internal parameters
	 */

	/**@{*/

	/**
	 * @brief Return the Ethernet source address for this bridge PDU.
	 */
	const rofl::caddress_ll&
	get_eth_src() const
	{ return eth_src; };

	/**
	 * @brief Set the Ethernet source address for this bridge PDU.
	 */
	void
	set_eth_src(
			const rofl::caddress_ll& eth_src)
	{ this->eth_src = eth_src; };

	/**
	 * @brief Return the Ethernet destination address for this bridge PDU.
	 */
	const rofl::caddress_ll&
	get_eth_dst() const
	{ return eth_dst; };

	/**
	 * @brief Set the Ethernet destination address for this bridge PDU.
	 */
	void
	set_eth_dst(
			const rofl::caddress_ll& eth_dst)
	{ this->eth_dst = eth_dst; };

	/**
	 * @brief Return LLC SSAP.
	 */
	uint8_t
	get_llc_ssap() const
	{ return llc_ssap; };

	/**
	 * @brief Set LLC SSAP.
	 */
	void
	set_llc_ssap(
			uint8_t llc_ssap)
	{ this->llc_ssap = llc_ssap; };

	/**
	 * @brief Return LLC DSAP.
	 */
	uint8_t
	get_llc_dsap() const
	{ return llc_dsap; };

	/**
	 * @brief Set LLC DSAP.
	 */
	void
	set_llc_dsap(
			uint8_t llc_dsap)
	{ this->llc_dsap = llc_dsap; };

	/**
	 * @brief Return LLC control field.
	 */
	uint8_t
	get_llc_control() const
	{ return llc_control; };

	/**
	 * @brief Set LLC control field.
	 */
	void
	set_llc_control(
			uint8_t llc_control)
	{ this->llc_control = llc_control; };

	/**
	 * @brief Return bridge PDU protocol identifier.
	 */
	uint16_t
	get_bpdu_protid() const
	{ return bpdu_protid; };

	/**
	 * @brief Set bridge PDU protocol identifier.
	 */
	void
	set_bpdu_protid(
			uint16_t bpdu_protid)
	{ this->bpdu_protid = bpdu_protid; };

	/**
	 * @brief Return bridge PDU version.
	 */
	uint8_t
	get_bpdu_version() const
	{ return bpdu_version; };

	/**
	 * @brief Set bridge PDU version.
	 */
	void
	set_bpdu_version(
			uint8_t bpdu_version)
	{ this->bpdu_version = bpdu_version; };

	/**
	 * @brief Return bridge PDU type.
	 */
	uint8_t
	get_bpdu_type() const
	{ return bpdu_type; };

	/**
	 * @brief Set bridge PDU type.
	 */
	void
	set_bpdu_type(
			uint8_t bpdu_type)
	{ this->bpdu_type = bpdu_type; };

	/**@}*/

public:

	friend std::ostream&
	operator<< (std::ostream& os, const cbpdumsg& msg) {

		return os;
	};

private:

	rofl::caddress_ll   eth_src;
	rofl::caddress_ll   eth_dst;
	uint8_t             llc_ssap;
	uint8_t             llc_dsap;
	uint8_t             llc_control;
	uint16_t            bpdu_protid; // protocol identifier
	uint8_t             bpdu_version; // protocol version
	uint8_t             bpdu_type; // bridge PDU type
};

}; // end of namespace rstp
}; // end of namespace ethernet
}; // end of namespace roflibs

#endif /* CBPDUMSG_HPP_ */
