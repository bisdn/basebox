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
cpython::run(std::string python_script)
{
	Py_SetProgramName((char*)"/usr/bin/python2.7");  /* optional but recommended */
	Py_Initialize();
	int argc = 1;
	char* argv[] = {
			const_cast<char*>( python_script.c_str() ),
	};
	PySys_SetArgv(argc, argv);

#if 0
	PyObject* argv_list = PyList_New(0);
	PyList_Append(argv_list, Py_BuildValue("s", ""));
#endif
	// Get a reference to the main module.
	PyObject* main_module = PyImport_AddModule("__main__");
	//PyModule_AddObject(main_module, "argv", argv_list);

	PyObject * sys_module = PyImport_ImportModule("sys");
	PyModule_AddObject(main_module, "sys", sys_module);
	PyObject * grecore_module = PyImport_ImportModule("grecore");
	PyModule_AddObject(main_module, "grecore", grecore_module);
	PyObject * ethcore_module = PyImport_ImportModule("ethcore");
	PyModule_AddObject(main_module, "ethcore", ethcore_module);

	// Get the main module's dictionary
	// and make a copy of it.
	PyObject* main_dict = PyModule_GetDict(main_module);

	FILE* fp = fopen(python_script.c_str(), "r");
	if (NULL == fp) {
		return -1;
	}
	PyRun_File(fp, python_script.c_str(), Py_file_input, main_dict, main_dict);
	fclose(fp);

	Py_Finalize();

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



