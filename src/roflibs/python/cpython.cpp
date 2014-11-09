/*
 * cpython.cpp
 *
 *  Created on: 25.09.2014
 *      Author: andreas
 */

#include "cpython.hpp"

using namespace roflibs::python;

cpython* cpython::instance = (cpython*)0;
std::string cpython::DEFAULT_PROGRAM_NAME = std::string("/usr/bin/python2.7");
std::string cpython::DEFAULT_SCRIPT = std::string("/usr/local/sbin/baseboxd.py");


void
cpython::run(const std::string& script, const std::string& program_name)
{
	if (tid != 0) {
		stop();
	}

	rofl::RwLock rwlock(thread_rwlock, rofl::RwLock::RWLOCK_WRITE);

	int rc = 0;
	this->script = script;
	this->program_name = program_name;

	rofcore::logging::debug << "THREAD STARTING" << std::endl;

	if ((rc = pthread_create(&tid, NULL, cpython::run_script, NULL)) < 0) {
		throw ePythonFailed("cpython::run() unable to create thread for python interpreter");
	}

	rofcore::logging::debug << "THREAD STARTED" << std::endl;
}


void
cpython::stop()
{
	rofl::RwLock rwlock(thread_rwlock, rofl::RwLock::RWLOCK_WRITE);

	if (tid == 0) {
		return;
	}

	rofcore::logging::debug << "THREAD STOPPING" << std::endl;

	pthread_cancel(tid);

	int* retval = (int*)NULL;
	pthread_join(tid, (void**)&retval);

	rofcore::logging::debug << "THREAD STOPPED" << std::endl;

	tid = 0;
}



/*static*/
void*
cpython::run_script(void* arg)
{
	cpython& p = cpython::get_instance();

	// set program name
	Py_SetProgramName((char*)const_cast<char*>(p.get_program_name().c_str()));  /* optional but recommended */

	// initialize interpreter
	Py_Initialize();

	// sys.argv
	int argc = 1;
	char* argv[] = {
			const_cast<char*>( p.get_script().c_str() ),
	};
	PySys_SetArgv(argc, argv);

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

	FILE* fp = fopen(p.get_script().c_str(), "r");
	if (NULL == fp) {
		return NULL;
	}
	PyRun_File(fp, p.get_script().c_str(), Py_file_input, main_dict, main_dict);
	fclose(fp);

	Py_Finalize();

	return NULL;
}



