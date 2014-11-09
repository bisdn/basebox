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

#include <roflibs/netlink/clogging.hpp>

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
class ePythonFailed : public ePythonBase {
public:
	ePythonFailed(const std::string& __arg) : ePythonBase(__arg) {};
};

class cpython {

	static cpython* instance;

	/**
	 *
	 */
	cpython() :
		tid(0),
		program_name(DEFAULT_PROGRAM_NAME),
		script(DEFAULT_SCRIPT)
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
	run(const std::string& python_script = DEFAULT_SCRIPT,
			const std::string& program_name = DEFAULT_PROGRAM_NAME);

	/**
	 *
	 */
	void
	stop();

	/**
	 *
	 */
	const std::string&
	get_program_name() const {
		rofl::RwLock rwlock(thread_rwlock, rofl::RwLock::RWLOCK_READ);
		return program_name;
	};

	/**
	 *
	 */
	const std::string&
	get_script() const {
		rofl::RwLock rwlock(thread_rwlock, rofl::RwLock::RWLOCK_READ);
		return script;
	};

private:

	/**
	 *
	 */
	static
	void* run_script(void* arg);

private:

	static std::string				DEFAULT_PROGRAM_NAME;
	std::string						program_name;
	static std::string				DEFAULT_SCRIPT;
	std::string						script;
	pthread_t						tid;
	mutable rofl::PthreadRwLock		thread_rwlock;
};

}; // end of namespace python
}; // end of namespace roflibs

#endif /* CPYTHON_HPP_ */
