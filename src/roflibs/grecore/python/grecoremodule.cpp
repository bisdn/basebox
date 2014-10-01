#include "grecoremodule.hpp"

using namespace roflibs::gre;

#ifdef __cplusplus
extern "C" {
#endif
static PyObject* grecore_get_gre_terms_in4(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* grecore_add_gre_term_in4(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* grecore_drop_gre_term_in4(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* grecore_get_gre_terms_in6(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* grecore_add_gre_term_in6(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* grecore_drop_gre_term_in6(PyObject *self, PyObject *args, PyObject* kw);

#ifdef __cplusplus
};
#endif

static PyMethodDef RofGREcoreMethods[] = {
		{"get_gre_terms_in4", (PyCFunction)grecore_get_gre_terms_in4, METH_KEYWORDS, "Get list of all GREv4 tunnels."},
		{"add_gre_term_in4",  (PyCFunction)grecore_add_gre_term_in4,  METH_KEYWORDS, "Add GREv4 tunnel."},
		{"drop_gre_term_in4", (PyCFunction)grecore_drop_gre_term_in4, METH_KEYWORDS, "Drop GREv4 tunnel."},
		{"get_gre_terms_in6", (PyCFunction)grecore_get_gre_terms_in6, METH_KEYWORDS, "Get list of all GREv6 tunnels."},
		{"add_gre_term_in6",  (PyCFunction)grecore_add_gre_term_in6,  METH_KEYWORDS, "Add GREv6 tunnel."},
		{"drop_gre_term_in6", (PyCFunction)grecore_drop_gre_term_in6, METH_KEYWORDS, "Drop GREv6 tunnel."},
		{NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initgrecore(void)
{
	if (not PyEval_ThreadsInitialized()) {
		PyEval_InitThreads();
	}
    (void) Py_InitModule("grecore", RofGREcoreMethods);
}


static PyObject*
grecore_get_gre_terms_in4(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				NULL
		};

		uint64_t dpid				= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "K", kwlist, &dpid)) {
			return NULL;
		}

		PyObject* py_termids = PyList_New(0);

		std::vector<uint32_t> term_ids =
				roflibs::gre::cgrecore::get_gre_core(rofl::cdpid(dpid)).get_gre_terms_in4();

		for (std::vector<uint32_t>::const_iterator
				it = term_ids.begin(); it != term_ids.end(); ++it) {
			PyList_Append(py_termids, Py_BuildValue("k", *it));
		}

		return py_termids;
	} catch (eGreCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (rofl::eSysCall& e) {
		PyErr_SetString(PyExc_RuntimeError, "invalid address");
	}
	return NULL;
}


static PyObject*
grecore_add_gre_term_in4(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"term_id",
				"gre_portno",
				"local",
				"remote",
				"gre_key",
				NULL
		};

		uint64_t dpid				= 0;
		uint32_t term_id			= 0;
		uint32_t gre_portno			= 0;
		const char* local			= (const char*)0;
		const char* remote			= (const char*)0;
		uint32_t gre_key			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "Kkkssk", kwlist,
				&dpid, &term_id, &gre_portno, &local, &remote, &gre_key)) {
			return NULL;
		}

		roflibs::gre::cgrecore::set_gre_core(rofl::cdpid(dpid)).
				set_gre_term_in4(term_id, gre_portno, rofl::caddress_in4(local), rofl::caddress_in4(remote), gre_key);

		return Py_None;
	} catch (eGreCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (rofl::eSysCall& e) {
		PyErr_SetString(PyExc_RuntimeError, "invalid address");
	}
	return NULL;
}


static PyObject*
grecore_drop_gre_term_in4(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"term_id",
				NULL
		};

		uint32_t dpid				= 0;
		uint32_t term_id			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "Kk", kwlist,
				&dpid, &term_id)) {
			return NULL;
		}

		roflibs::gre::cgrecore::set_gre_core(rofl::cdpid(dpid)).
				drop_gre_term_in4(term_id);

		return Py_None;
	} catch (eGreCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	}
	return NULL;
}


static PyObject*
grecore_get_gre_terms_in6(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				NULL
		};

		uint64_t dpid				= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "K", kwlist, &dpid)) {
			return NULL;
		}

		PyObject* py_termids = PyList_New(0);

		std::vector<uint32_t> term_ids =
				roflibs::gre::cgrecore::get_gre_core(rofl::cdpid(dpid)).get_gre_terms_in6();

		for (std::vector<uint32_t>::const_iterator
				it = term_ids.begin(); it != term_ids.end(); ++it) {
			PyList_Append(py_termids, Py_BuildValue("k", *it));
		}

		return py_termids;
	} catch (eGreCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (rofl::eSysCall& e) {
		PyErr_SetString(PyExc_RuntimeError, "invalid address");
	}
	return NULL;
}


static PyObject*
grecore_add_gre_term_in6(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"term_id",
				"gre_portno",
				"local",
				"remote",
				"gre_key",
				NULL
		};

		uint64_t dpid				= 0;
		uint32_t term_id			= 0;
		uint32_t gre_portno			= 0;
		const char* local			= (const char*)0;
		const char* remote			= (const char*)0;
		uint32_t gre_key			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "Kkkssk", kwlist,
				&dpid, &term_id, &gre_portno, &local, &remote, &gre_key)) {
			return NULL;
		}

		roflibs::gre::cgrecore::set_gre_core(rofl::cdpid(dpid)).
				set_gre_term_in6(term_id, gre_portno, rofl::caddress_in6(local), rofl::caddress_in6(remote), gre_key);

		return Py_None;
	} catch (eGreCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (rofl::eSysCall& e) {
		PyErr_SetString(PyExc_RuntimeError, "invalid address");
	}
	return NULL;
}


static PyObject*
grecore_drop_gre_term_in6(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"term_id",
				NULL
		};

		uint64_t dpid				= 0;
		uint32_t term_id			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "Kk", kwlist,
				&dpid, &term_id)) {
			return NULL;
		}

		roflibs::gre::cgrecore::set_gre_core(rofl::cdpid(dpid)).
				drop_gre_term_in6(term_id);

		return Py_None;
	} catch (eGreCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	}
	return NULL;
}



