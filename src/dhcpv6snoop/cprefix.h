/*
 * cprefix.h
 *
 *  Created on: 17.09.2013
 *      Author: andreas
 */

#ifndef CPREFIX_H_
#define CPREFIX_H_

#include <string>
#include <iostream>

#include <rofl/common/caddress.h>

namespace dhcpv6snoop
{

class cprefix
{
	std::string			devname;
	std::string 		serverid;
	rofl::caddress		pref;
	unsigned int		preflen;
	rofl::caddress		via;

public:

	cprefix();

	virtual
	~cprefix();

	cprefix(
			cprefix const& rt);

	cprefix&
	operator= (
			cprefix const& rt);

	cprefix(
			std::string const& devname,
			std::string const& serverid,
			rofl::caddress const& pref,
			unsigned int preflen,
			rofl::caddress const& via);


	bool
	operator< (cprefix const& rt);

public:

	void
	route_add();

	void
	route_del();

	std::string
	get_devname() const 	{ return devname; };

	std::string
	get_server_id() const 	{ return serverid; };

	rofl::caddress
	get_pref() const 		{ return pref; };

	unsigned int
	get_pref_len() const 	{ return preflen; };

	rofl::caddress
	get_via() const  		{ return via; };

private:



public:

	friend std::ostream&
	operator<< (std::ostream& os, cprefix const& rt) {

		return os;
	};

	class cprefix_find_by_pref_and_preflen {
		rofl::caddress 	pref;
		unsigned int 	preflen;
	public:
		cprefix_find_by_pref_and_preflen(rofl::caddress const& pref, unsigned int preflen) :
			pref(pref), preflen(preflen) {};
		bool operator() (cprefix const* p) {
			return ((p->pref == pref) && (p->preflen == preflen));
		};
	};

	class cprefix_find_by_serverid {
		std::string 	serverid;
	public:
		cprefix_find_by_serverid(std::string const& serverid) :
			serverid(serverid) {};
		bool operator() (cprefix const* p) {
			return (p->serverid == serverid);
		};
	};
};

}; // end of namespace

#endif /* CPREFIX_H_ */
