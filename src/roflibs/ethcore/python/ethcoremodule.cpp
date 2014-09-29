#include "ethcoremodule.hpp"

using namespace roflibs::ethernet;

#ifdef __cplusplus
extern "C" {
#endif
static PyObject* ethcore_add_vlan(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_drop_vlan(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_has_vlan(PyObject *self, PyObject *args, PyObject* kw);
#ifdef __cplusplus
};
#endif

static PyMethodDef RofEthernetCoreMethods[] = {
		{"add_vlan",  (PyCFunction)ethcore_add_vlan,  METH_KEYWORDS, "Add VLAN."},
		{"drop_vlan", (PyCFunction)ethcore_drop_vlan, METH_KEYWORDS, "Drop VLAN."},
		{"has_vlan",  (PyCFunction)ethcore_has_vlan,  METH_KEYWORDS, "Check for VLAN."},
		{NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initethcore(void)
{
	if (not PyEval_ThreadsInitialized()) {
		PyEval_InitThreads();
	}
    (void) Py_InitModule("ethcore", RofEthernetCoreMethods);
}


static PyObject*
ethcore_add_vlan(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"vid",
				NULL
		};

		uint64_t dpid			= 0;
		uint16_t vid			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "KH", kwlist,
				&dpid, &vid)) {
			return NULL;
		}

		roflibs::ethernet::cethcore::set_eth_core(rofl::cdpid(dpid)).add_vlan(vid);

		return Py_None;
	} catch (eEthCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (eVlanNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "vid not found");
	} catch (rofl::eSysCall& e) {
		PyErr_SetString(PyExc_RuntimeError, "invalid address");
	}
	return NULL;
}


static PyObject*
ethcore_drop_vlan(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"vid",
				NULL
		};

		uint64_t dpid			= 0;
		uint16_t vid			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "KH", kwlist,
				&dpid, &vid)) {
			return NULL;
		}

		roflibs::ethernet::cethcore::set_eth_core(rofl::cdpid(dpid)).drop_vlan(vid);

		return Py_None;
	} catch (eEthCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (eVlanNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "vid not found");
	}
	return NULL;
}


static PyObject*
ethcore_has_vlan(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"vid",
				NULL
		};

		uint64_t dpid			= 0;
		uint16_t vid			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "KH", kwlist,
				&dpid, &vid)) {
			return NULL;
		}

		return Py_BuildValue("i", roflibs::ethernet::cethcore::set_eth_core(rofl::cdpid(dpid)).has_vlan(vid));
	} catch (eEthCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (eVlanNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "vid not found");
	} catch (rofl::eSysCall& e) {
		PyErr_SetString(PyExc_RuntimeError, "invalid address");
	}
	return NULL;
}



