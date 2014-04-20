/*
 * crofmodel.cpp
 *
 *  Created on: 20.04.2014
 *      Author: andreas
 */

#include "crofmodel.hpp"

using namespace rofcore;

crofmodel::crofmodel(
		uint8_t version) :
				version(version),
				n_buffers(0),
				n_tables(0),
				auxiliary_id(0),
				capabilities(0),
				flags(0),
				miss_send_len(0),
				ports(version),
				tables(version)
{

}



crofmodel::~crofmodel()
{

}



crofmodel::crofmodel(
		crofmodel const& rofmodel)
{
	*this = rofmodel;
}



crofmodel&
crofmodel::operator= (
		crofmodel const& rofmodel)
{
	if (this == &rofmodel)
		return *this;

	version			= rofmodel.version;
	n_buffers		= rofmodel.n_buffers;
	n_tables		= rofmodel.n_tables;
	auxiliary_id	= rofmodel.auxiliary_id;
	capabilities	= rofmodel.capabilities;
	flags			= rofmodel.flags;
	miss_send_len	= rofmodel.miss_send_len;
	ports			= rofmodel.ports;
	tables			= rofmodel.tables;
	desc_stats		= rofmodel.desc_stats;

	return *this;
}



void
crofmodel::clear()
{
	ports.clear();
	tables.clear();
	desc_stats.clear();
}


