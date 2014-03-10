/*
 * flowmod.h
 *
 *  Created on: 03.07.2013
 *      Author: andreas
 */

#ifndef FLOWMOD_H_
#define FLOWMOD_H_ 1

namespace dptmap
{

class flowmod
{
public:

	/**
	 *
	 */
	virtual ~flowmod() {};


	/**
	 *
	 */
	virtual void flow_mod_add() = 0;


	/**
	 *
	 */
	virtual void flow_mod_modify() = 0;


	/**
	 *
	 */
	virtual void flow_mod_delete() = 0;
};

}; // end of namespace

#endif /* FLOWMOD_H_ */
