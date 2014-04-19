/*
 * crofproxy.cpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#include "crofproxy.hpp"

using namespace rofcore;

crofproxy::crofproxy()
{

}



crofproxy::~crofproxy()
{

}



crofproxy::crofproxy(
			crofproxy const& rofproxy)
{
	*this = rofproxy;
}



crofproxy&
crofproxy::operator= (
			crofproxy const& rofproxy)
{
	if (this == &rofproxy)
		return *this;

	// TODO

	return *this;
}



crofproxy*
crofproxy::crofproxy_factory(
		enum rofproxy_type_t proxy_type)
{
	switch (proxy_type) {
	case PROXY_TYPE_ETHCORE: {
		//return new cethcore();
	} break;
	case PROXY_TYPE_IPCORE: {
		// return new cipcore();
	} break;
	}
}






