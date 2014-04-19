/*
 * crofproxies.cpp
 *
 *  Created on: 19.04.2014
 *      Author: andreas
 */

#include "crofproxies.hpp"

using namespace rofcore;

/*static*/std::set<crofproxies*> crofproxies::rofproxies;

/*static*/void
crofproxies::crofproxies_sa_handler(
		int signum)
{
	for (std::set<crofproxies*>::iterator
			it = crofproxies::rofproxies.begin(); it != crofproxies::rofproxies.end(); ++it) {
		(*(*it)).signal_handler(signum);
	}
}


crofproxies::crofproxies(
		enum crofproxy::rofproxy_type_t proxy_type,
		rofl::openflow::cofhello_elem_versionbitmap const& versionbitmap) :
				rofl::crofbase(versionbitmap),
				proxy_type(proxy_type)
{
	crofproxies::rofproxies.insert(this);
}



crofproxies::~crofproxies()
{
	crofproxies::rofproxies.erase(this);
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
crofproxies::signal_handler(
		int signum)
{
	for (std::map<uint64_t, crofproxy*>::iterator
			it = proxies.begin(); it != proxies.end(); ++it) {
		it->second->signal_handler(signum);
	}
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





