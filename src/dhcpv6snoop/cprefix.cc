/*
 * cprefix.cc
 *
 *  Created on: 17.09.2013
 *      Author: andreas
 */

#include "cprefix.h"

using namespace dhcpv6snoop;


std::string	cprefix::prefix_up_script_path(DEFAULT_UP_SCRIPT_PATH);
std::string	cprefix::prefix_down_script_path(DEFAULT_DOWN_SCRIPT_PATH);



cprefix::cprefix() :
		preflen(0)
{

}



cprefix::~cprefix()
{

}



cprefix::cprefix(
			cprefix const& rt)
{
	*this = rt;
}



cprefix&
cprefix::operator= (
			cprefix const& rt)
{
	if (this == &rt)
		return *this;

	devname		= rt.devname;
	serverid	= rt.serverid;
	pref		= rt.pref;
	preflen		= rt.preflen;
	via			= rt.via;

	return *this;
}



cprefix::cprefix(
			std::string const& devname,
			std::string const& serverid,
			rofl::caddress const& pref,
			unsigned int preflen,
			rofl::caddress const& via) :
		devname(devname),
		serverid(serverid),
		pref(pref),
		preflen(preflen),
		via(via)
{

}



bool
cprefix::operator< (cprefix const& pr)
{
	return ((preflen < pr.preflen) && (pref < pr.pref));
}



void
cprefix::route_add()
{
	// TODO: set route into kernel
	std::cerr << "cprefix::route_add() " << *this << std::endl;

	std::vector<std::string> argv;
	std::vector<std::string> envp;

	std::stringstream s_prefix;
	s_prefix << "PREFIX=" << pref;
	envp.push_back(s_prefix.str());

	std::stringstream s_prefixlen;
	s_prefixlen << "PREFIXLEN=" << preflen;
	envp.push_back(s_prefixlen.str());

	std::stringstream s_nexthop;
	s_nexthop << "NEXTHOP=" << via;
	envp.push_back(s_nexthop.str());

	std::stringstream s_iface;
	s_iface << "IFACE=" << devname;
	envp.push_back(s_iface.str());

	cprefix::execute(cprefix::prefix_up_script_path, argv, envp);
}



void
cprefix::route_del()
{
	// TODO: remove route from kernel
	std::cerr << "cprefix::route_del() " << *this << std::endl;

	std::vector<std::string> argv;
	std::vector<std::string> envp;

	std::stringstream s_prefix;
	s_prefix << "PREFIX=" << pref;
	envp.push_back(s_prefix.str());

	std::stringstream s_prefixlen;
	s_prefixlen << "PREFIXLEN=" << preflen;
	envp.push_back(s_prefixlen.str());

	std::stringstream s_nexthop;
	s_nexthop << "NEXTHOP=" << via;
	envp.push_back(s_nexthop.str());

	std::stringstream s_iface;
	s_iface << "IFACE=" << devname;
	envp.push_back(s_iface.str());

	cprefix::execute(cprefix::prefix_down_script_path, argv, envp);
}





void
cprefix::execute(
		std::string const& executable,
		std::vector<std::string> argv,
		std::vector<std::string> envp)
{
	pid_t pid = 0;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "syscall error fork(): %d (%s)\n", errno, strerror(errno));
		return;
	}

	if (pid > 0) { // father process
		return;
	}


	// child process

	std::vector<const char*> vctargv;
	for (std::vector<std::string>::iterator
			it = argv.begin(); it != argv.end(); ++it) {
		vctargv.push_back((*it).c_str());
	}
	vctargv.push_back(NULL);


	std::vector<const char*> vctenvp;
	for (std::vector<std::string>::iterator
			it = envp.begin(); it != envp.end(); ++it) {
		vctenvp.push_back((*it).c_str());
	}
	vctenvp.push_back(NULL);


	execvpe(executable.c_str(), (char*const*)&vctargv[0], (char*const*)&vctenvp[0]);
}



