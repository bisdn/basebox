/*
 * ccookies.cpp
 *
 *  Created on: 19.11.2014
 *      Author: andreas
 */

#include "ccookiebox.hpp"

using namespace roflibs::common::openflow;

/*static*/ccookiebox* ccookiebox::cookiebox = (ccookiebox*)0;

ccookie_owner::~ccookie_owner()
{
	ccookiebox::get_instance().deregister_cookie_owner(this);
}

uint64_t
ccookie_owner::acquire_cookie()
{
	return ccookiebox::get_instance().acquire_cookie(this);
}

void
ccookie_owner::release_cookie(uint64_t cookie)
{
	ccookiebox::get_instance().release_cookie(this, cookie);
}

