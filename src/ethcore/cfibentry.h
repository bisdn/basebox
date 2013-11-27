/*
 * cfibentry.h
 *
 *  Created on: 15.07.2013
 *      Author: andreas
 */

#ifndef CFIBENTRY_H_
#define CFIBENTRY_H_ 1

#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/ciosrv.h>
#include <rofl/common/cmacaddr.h>
#include <rofl/common/crofbase.h>
#include <rofl/common/logging.h>
#include <rofl/common/openflow/cofdpt.h>

namespace ethercore
{

class cfibentry; // forward declaration, see below

class cfibtable
{
public:
	virtual ~cfibtable() {};
	virtual void fib_timer_expired(cfibentry *entry) = 0;
	virtual rofl::crofbase* get_rofbase() = 0;
	virtual uint32_t get_flood_group_id(uint16_t vid) = 0;
};

class cfibentry :
		public rofl::ciosrv
{
#define CFIBENTRY_DEFAULT_TIMEOUT		60

private:

	enum cfibentry_timer_t {
		CFIBENTRY_ENTRY_EXPIRED = 0x99ae,
	};

	cfibtable					*fib;
	uint64_t					dpid;
	uint16_t					vid;
	uint32_t					out_port_no;
	bool						tagged;
	rofl::cmacaddr				dst;
	uint8_t						table_id;
	int							entry_timeout;

public:

	/**
	 *
	 */
	cfibentry(
			cfibtable *fib,
			uint8_t table_id,
			rofl::cmacaddr dst,
			uint64_t dpid,
			uint16_t vid,
			uint32_t out_port_no,
			bool tagged = true);

	/**
	 *
	 */
	virtual
	~cfibentry();

	/**
	 *
	 */
	uint32_t
	get_out_port_no() const { return out_port_no; };

	/**
	 *
	 */
	void
	set_out_port_no(uint32_t out_port_no);

	/**
	 *
	 */
	rofl::cmacaddr const&
	get_lladdr() const { return dst; };

	/**
	 *
	 */
	void
	flow_mod_add();

	/**
	 *
	 */
	void
	flow_mod_delete();

	/**
	 *
	 */
	void
	flow_mod_modify();

private:

	/**
	 *
	 */
	virtual void
	handle_timeout(int opaque);

public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, cfibentry const& entry)
	{
		os << "<fibentry ";
			os << "dst: " << entry.dst << " ";
			os << "vid:" << entry.vid << " ";
			os << "out-port: " << entry.out_port_no << " ";
			os << "tagged:" << (entry.tagged ? "true":"false") << " ";
		os << ">";
		return os;
	};
};

}; // end of namespace

#endif /* CFIBENTRY_H_ */
