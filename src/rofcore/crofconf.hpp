/*
 * crofconf.h
 *
 *  Created on: 17.12.2013
 *      Author: andreas
 */

#ifndef CROFCONF_H_
#define CROFCONF_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#ifdef __cplusplus
}
#endif


#include <string>
#include <iostream>

#include <libconfig.h++>

using namespace libconfig;

namespace rofcore
{

class crofconf : public Config {
private:
	static std::string const CONFPATH;

	std::string		confpath;

private:

    static crofconf *scrofconf;

    /**
     * @brief	Make copy constructor private for singleton
     */
    crofconf(crofconf const& conf) {};

	/**
	 *
	 */
	crofconf() {};

	/**
	 *
	 */
	virtual
	~crofconf() {};

public:

	/**
	 *
	 */
    static crofconf&
    get_instance();

    /**
     *
     */
    void
    open(std::string const& confpath = crofconf::CONFPATH);


public:
;
};

}; // end of namespace

#endif /* CROFCONF_H_ */
