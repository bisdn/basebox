/*
 * cpython.hpp
 *
 *  Created on: 25.09.2014
 *      Author: andreas
 */

#ifndef CPYTHON_HPP_
#define CPYTHON_HPP_

#include <Python.h>

namespace roflibs {
namespace python {

class cpython {

	static cpython* instance;

	/**
	 *
	 */
	cpython()
	{};

	/**
	 *
	 */
	~cpython()
	{};

public:

	/**
	 *
	 */
	static cpython&
	get_instance() {
		if ((cpython*)0 == cpython::instance) {
			cpython::instance = new cpython();
		}
		return *(cpython::instance);
	};

	/**
	 *
	 */
	void
	run();

	/**
	 *
	 */
	void
	stop();

private:


};

}; // end of namespace python
}; // end of namespace roflibs

#endif /* CPYTHON_HPP_ */
