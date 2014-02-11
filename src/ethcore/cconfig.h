/*
 * cconfig.h
 *
 *  Created on: 17.12.2013
 *      Author: andreas
 */

#ifndef CCONFIG_H_
#define CCONFIG_H_

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

namespace ethercore
{

class cconfig : public Config {
private:
	static std::string const CONFPATH;

	std::string		confpath;

private:

    static cconfig *scconfig;

    /**
     * @brief	Make copy constructor private for singleton
     */
    cconfig(cconfig const& conf) {};

	/**
	 *
	 */
	cconfig() {};

	/**
	 *
	 */
	virtual
	~cconfig() {};

public:

	/**
	 *
	 */
    static cconfig&
    get_instance();

    /**
     *
     */
    void
    open(std::string const& confpath = cconfig::CONFPATH);


public:
;
};

}; // end of namespace

#endif /* CCONFIG_H_ */
