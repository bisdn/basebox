/*
 * cdhclient.cc
 *
 *  Created on: 17.09.2013
 *      Author: andreas
 */

#include "cdhclient.h"

using namespace dhcpv6snoop;

std::map<std::string, cdhclient*> cdhclient::cdhclients;



cdhclient&
cdhclient::get_client(
			std::string const& clientid)
{
	if (cdhclient::cdhclients.find(clientid) == cdhclient::cdhclients.end()) {
		throw eDHClientNotFound();
	}
	return *(cdhclient::cdhclients[clientid]);
}



cdhclient::cdhclient(
		std::string const& clientid) :
				clientid(clientid)
{
	if (cdhclient::cdhclients.find(clientid) != cdhclient::cdhclients.end()) {
		throw eDHClientExists();
	}

	cdhclient::cdhclients[clientid] = this;
}



cdhclient::~cdhclient()
{
	cdhclient::cdhclients.erase(clientid);

	for (std::set<cprefix*>::iterator
			it = prefixes.begin(); it != prefixes.end(); ++it) {
		delete (*it);
	}
	prefixes.clear(); // not really necessary :P
}



void
cdhclient::prefix_add(
		cprefix const& prefix)
{
	std::cerr << "cdhclient::prefix_add() [1] " << *this << std::endl;
	if (find_if(prefixes.begin(), prefixes.end(),
			cprefix::cprefix_find_by_pref_and_preflen(prefix.get_pref(), prefix.get_pref_len()))
					!= prefixes.end()) {
		return;
	}
	cprefix *p = new cprefix(prefix);
	prefixes.insert(p);
	p->route_add();
	std::cerr << "cdhclient::prefix_add() [2] " << *this << std::endl;
}



void
cdhclient::prefix_del(
		std::string const& serverid)
{
    std::cerr << "cdhclient::prefix_del() [1] " << *this << std::endl;
	std::set<cprefix*>::iterator it;
	while ((it = find_if(prefixes.begin(), prefixes.end(),
					cprefix::cprefix_find_by_serverid(serverid)))
							!= prefixes.end()) {
		cprefix* p = (*it);
		p->route_del();
		prefixes.erase(it);
	}
	std::cerr << "cdhclient::prefix_del() [2] " << *this << std::endl;
}



