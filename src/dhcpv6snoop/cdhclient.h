/*
 * cdhclient.h
 *
 *  Created on: 17.09.2013
 *      Author: andreas
 */

#ifndef CDHCLIENT_H_
#define CDHCLIENT_H_

#include <set>
#include <map>
#include <string>
#include <iostream>
#include <exception>
#include <algorithm>

#include "cprefix.h"

namespace dhcpv6snoop
{

class eDHClientBase 			: public std::exception {};
class eDHClientExists			: public eDHClientBase {};
class eDHClientNotFound			: public eDHClientBase {};

class cdhclient
{
	static std::map<std::string, cdhclient*> cdhclients; // key: clientid(string), value: pointer to cdhclient instance

public:

	static cdhclient&
	get_client(
			std::string const& clientid);

private:

	std::string			clientid;
	std::set<cprefix*>	prefixes;

public:

	cdhclient(
			std::string const& clientid);

	virtual
	~cdhclient();

	/**
	 * @brief 	Add a new prefix to this instance
	 *
	 * @param prefix
	 */
	void
	prefix_add(
			cprefix const& prefix);

	/**
	 * @brief	Delete all prefixes assigned by server-id
	 *
	 * @param serverid
	 */
	void
	prefix_del(
			std::string const& serverid);

public:

	friend std::ostream&
	operator<< (std::ostream& os, cdhclient const& dhc) {
		os << "<cdhclient: client-id: " << dhc.clientid << " ";
			os << "prefixes: ";
			for (std::set<cprefix*>::const_iterator
					it = dhc.prefixes.begin(); it != dhc.prefixes.end(); ++it) {
				os << *(*it) << " ";
			}
		os << " >";
		return os;
	};
};

}; // end of namespace



#endif /* CDHCLIENT_H_ */
