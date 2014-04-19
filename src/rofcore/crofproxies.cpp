/*
 * crofproxies.cpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#include "crofproxies.hpp"

using namespace rofcore;

crofproxies::crofproxies(
		enum crofproxy::rofproxy_type_t proxy_type) :
				proxy_type(proxy_type)
{

}



crofproxies::~crofproxies()
{
	clear();
}



crofproxies::crofproxies(
		crofproxies const& rofproxies)
{
	*this = rofproxies;
}



crofproxies&
crofproxies::operator= (
		crofproxies const& rofproxies)
{
	if (this == &rofproxies)
		return *this;

	clear();
	// TODO

	return *this;
}



void
crofproxies::clear()
{
	for (std::map<uint64_t, crofproxy*>::iterator
			it = proxies.begin(); it != proxies.end(); ++it) {
		delete it->second;
	}
	proxies.clear();
}



crofproxy&
crofproxies::add_proxy(
		uint64_t dpid)
{
	if (proxies.find(dpid) != proxies.end()) {
		delete proxies[dpid];
	}
	proxies[dpid] = crofproxy::crofproxy_factory(proxy_type);
	return dynamic_cast<crofproxy&>( *(proxies[dpid]) );
}



crofproxy&
crofproxies::set_proxy(
		uint64_t dpid)
{
	if (proxies.find(dpid) == proxies.end()) {
		proxies[dpid] = crofproxy::crofproxy_factory(proxy_type);
	}
	return dynamic_cast<crofproxy&>( *(proxies[dpid]) );
}



crofproxy const&
crofproxies::get_proxy(
		uint64_t dpid) const
{
	if (proxies.find(dpid) == proxies.end()) {
		throw eRofProxyNotFound();
	}
	return dynamic_cast<crofproxy const&>( *(proxies.at(dpid)) );
}



void
crofproxies::drop_proxy(
		uint64_t dpid)
{
	if (proxies.find(dpid) == proxies.end()) {
		return;
	}
	delete proxies[dpid];
	proxies.erase(dpid);
}



bool
crofproxies::has_proxy(
		uint64_t dpid) const
{
	return (not (proxies.find(dpid) == proxies.end()));
}





