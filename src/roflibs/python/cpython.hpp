/*
 * cpython.hpp
 *
 *  Created on: 25.09.2014
 *      Author: andreas
 */

#ifndef CPYTHON_HPP_
#define CPYTHON_HPP_

#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#ifdef __cplusplus
}
#endif

#include <Python.h>
#include <exception>
#include <string>
#include <map>
#include <rofl/common/thread_helper.h>

namespace roflibs {
namespace python {

class ePythonBase : public std::runtime_error {
public:
	ePythonBase(const std::string& __arg) : std::runtime_error(__arg) {};
};
class ePythonNotFound : public ePythonBase {
public:
	ePythonNotFound(const std::string& __arg) : ePythonBase(__arg) {};
};

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
	int
	run(const std::string& python_script);

	/**
	 *
	 */
	pthread_t
	run_as_thread(const std::string& python_script);

	/**
	 *
	 */
	int
	stop_thread(pthread_t tid);

	/**
	 *
	 */
	static
	void* run_script(void* arg);

	/**
	 *
	 */
	const std::string&
	get_thread_arg(pthread_t tid) const;

	/**
	 *
	 */
	bool
	has_thread_arg(pthread_t tid) const;

private:

	std::map<pthread_t, std::string>		threads; // key: tid, value: python_script
	mutable rofl::PthreadRwLock				threads_rwlock;
};

}; // end of namespace python
}; // end of namespace roflibs

#endif /* CPYTHON_HPP_ */
