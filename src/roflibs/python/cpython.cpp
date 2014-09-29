/*
 * cpython.cpp
 *
 *  Created on: 25.09.2014
 *      Author: andreas
 */

#include "cpython.hpp"

using namespace roflibs::python;

cpython* cpython::instance = (cpython*)0;

int
cpython::run(const std::string& python_script)
{
	FILE* fp = fopen(python_script.c_str(), "r");

	if (NULL == fp) {
		return -1;
	}

	Py_SetProgramName(const_cast<char*>( python_script.c_str() ));  /* optional but recommended */
	Py_Initialize();
	PyRun_SimpleFile(fp, python_script.c_str());
	Py_Finalize();

	fclose(fp);

	return 0;
}


pthread_t
cpython::run_as_thread(const std::string& python_script)
{
	pthread_t tid = 0;
	int rc = 0;

	rofl::RwLock rwlock(threads_rwlock, rofl::RwLock::RWLOCK_WRITE);

	if ((rc = pthread_create(&tid, NULL, cpython::run_script, NULL)) < 0) {
		return 0;
	}

	threads[tid] = python_script;

	return tid;
}


int
cpython::stop_thread(pthread_t tid)
{
	rofl::RwLock rwlock(threads_rwlock, rofl::RwLock::RWLOCK_WRITE);

	if (threads.find(tid) == threads.end()) {
		throw ePythonNotFound("cpython::stop_thread()");
	}
	pthread_cancel(tid);
	int* retval = (int*)NULL;
	pthread_join(tid, (void**)&retval);
	threads.erase(tid);
	return 0;
}


const std::string&
cpython::get_thread_arg(pthread_t tid) const
{
	rofl::RwLock rwlock(threads_rwlock, rofl::RwLock::RWLOCK_READ);

	if (threads.find(tid) == threads.end()) {
		throw ePythonNotFound("cpython::get_thread_arg()");
	}
	return threads.at(tid);
}


bool
cpython::has_thread_arg(pthread_t tid) const
{
	rofl::RwLock rwlock(threads_rwlock, rofl::RwLock::RWLOCK_READ);
	return (not (threads.find(tid) == threads.end()));
}


/*static*/
void*
cpython::run_script(void* arg)
{
	while (not cpython::get_instance().has_thread_arg(pthread_self())) {
		sleep(1);
	}
	cpython::get_instance().run(cpython::get_instance().get_thread_arg(pthread_self()));

	return NULL;
}



