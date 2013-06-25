/*
 * clinkcache.h
 *
 *  Created on: 25.06.2013
 *      Author: andreas
 */

#ifndef CLINKCACHE_H_
#define CLINKCACHE_H_ 1

#include <rofl/common/ciosrv.h>

namespace dptmap
{

class clinkcache :
		public rofl::ciosrv
{

	struct nl_cache_mngr *mngr;
	struct nl_cache *linkcache;


public:

	/**
	 *
	 */
	clinkcache();


	/**
	 *
	 */
	virtual
	~clinkcache();


};

}; // end of namespace dptmap

#endif /* CLINKCACHE_H_ */
