/*
 * crtable.h
 *
 *  Created on: 02.07.2013
 *      Author: andreas
 */

#ifndef DPTROUTE_H_
#define DPTROUTE_H_ 1

#include <ostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
#ifdef __cplusplus
}
#endif

#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/cofdpt.h>
#include <rofl/common/openflow/cflowentry.h>
#include <cnetlink.h>
#include <crtroute.h>

namespace dptmap
{

class dptroute :
		public cnetlink_subscriber
{
private:


	rofl::crofbase			*rofbase;
	rofl::cofdpt			*dpt;
	uint8_t					 table_id;
	unsigned int			 rtindex;
	rofl::cflowentry		 flowentry;


public:


	/**
	 *
	 */
	dptroute(
			rofl::crofbase* rofbase,
			rofl::cofdpt* dpt,
			uint8_t table_id,
			unsigned int rtindex);


	/**
	 *
	 * @param table_id
	 * @param rtindex
	 */
	virtual
	~dptroute();


	/**
	 *
	 * @param rtindex
	 */
	virtual void
	route_created(
			uint8_t table_id,
			unsigned int rtindex);


	/**
	 *
	 * @param rtindex
	 */
	virtual void
	route_updated(
			uint8_t table_id,
			unsigned int rtindex);


	/**
	 *
	 * @param rtindex
	 */
	virtual void
	route_deleted(
			uint8_t table_id,
			unsigned int rtindex);

public:

	/**
	 *
	 */
	friend std::ostream&
	operator<< (std::ostream& os, dptroute const& route)
	{
		// FIXME: write cflowentry::operator<<()
		rofl::cflowentry fe(route.flowentry);
		char s_buf[1024];
		memset(s_buf, 0, sizeof(s_buf));
		snprintf(s_buf, sizeof(s_buf)-1, "%s", fe.c_str());
		os << "dptroute{";
		os << "table_id=" << (unsigned int)route.table_id << " ";
		os << "rtindex=" << route.rtindex << " ";
		os << "flowentry=" << s_buf << " ";
		os << "}";
		return os;
	};
};

};

#endif /* CRTABLE_H_ */
