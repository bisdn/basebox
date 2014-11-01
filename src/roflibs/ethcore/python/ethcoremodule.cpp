#include "ethcoremodule.hpp"

using namespace roflibs::eth;

#ifdef __cplusplus
extern "C" {
#endif
static PyObject* ethcore_get_switches(PyObject *self, PyObject *args);
static PyObject* ethcore_get_vlans(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_add_vlan(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_drop_vlan(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_has_vlan(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_add_port(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_drop_port(PyObject *self, PyObject *args, PyObject* kw);
static PyObject* ethcore_has_port(PyObject *self, PyObject *args, PyObject* kw);
#ifdef __cplusplus
};
#endif

static PyMethodDef RofEthernetCoreMethods[] = {
		{"get_switches", (PyCFunction)ethcore_get_switches, METH_VARARGS, "Get list of all switches."},
		{"get_vlans", (PyCFunction)ethcore_get_vlans, METH_KEYWORDS, "Get list of all vids."},
		{"add_vlan",  (PyCFunction)ethcore_add_vlan,  METH_KEYWORDS, "Add VLAN."},
		{"drop_vlan", (PyCFunction)ethcore_drop_vlan, METH_KEYWORDS, "Drop VLAN."},
		{"has_vlan",  (PyCFunction)ethcore_has_vlan,  METH_KEYWORDS, "Check for VLAN."},
		{"add_port",  (PyCFunction)ethcore_add_port,  METH_KEYWORDS, "Add port to a VLAN."},
		{"drop_port", (PyCFunction)ethcore_drop_port, METH_KEYWORDS, "Drop port from a VLAN."},
		{"has_port",  (PyCFunction)ethcore_has_port,  METH_KEYWORDS, "Check for port membership on VLAN."},
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
ethcore_get_switches(PyObject *self, PyObject *args)
{
	try {
		PyObject* py_switches = PyList_New(0);

		std::cerr << "UUUUUU [1]" << std::endl;

		std::set<uint64_t> dpids =
				roflibs::eth::cethcore::get_eth_cores();

		std::cerr << "UUUUUU [2] size:" << dpids.size() << std::endl;

		for (std::set<uint64_t>::const_iterator
				it = dpids.begin(); it != dpids.end(); ++it) {
			PyList_Append(py_switches, Py_BuildValue("K", *it));
		}

		std::cerr << "UUUUUU [3]" << std::endl;

		return py_switches;
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
ethcore_get_vlans(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				NULL
		};

		uint64_t dpid			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "K", kwlist, &dpid)) {
			return NULL;
		}

		PyObject* py_vids = PyList_New(0);

		std::vector<uint16_t> vids =
				roflibs::eth::cethcore::get_eth_core(rofl::cdpid(dpid)).get_vlans();

		for (std::vector<uint16_t>::const_iterator
				it = vids.begin(); it != vids.end(); ++it) {
			PyList_Append(py_vids, Py_BuildValue("i", *it));
		}

		return py_vids;
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

		roflibs::eth::cethcore::set_eth_core(rofl::cdpid(dpid)).add_vlan(vid);

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

		roflibs::eth::cethcore::set_eth_core(rofl::cdpid(dpid)).drop_vlan(vid);

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

		return Py_BuildValue("i", roflibs::eth::cethcore::set_eth_core(rofl::cdpid(dpid)).has_vlan(vid));
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
ethcore_add_port(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"vid",
				"portno",
				"tagged",
				NULL
		};

		uint64_t dpid			= 0;
		uint16_t vid			= 0;
		uint32_t portno			= 0;
		int	tagged				= 1;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "KHIi", kwlist,
				&dpid, &vid, &portno, &tagged)) {
			return NULL;
		}

		roflibs::eth::cethcore::set_eth_core(rofl::cdpid(dpid)).set_vlan(vid).add_phy_port(portno, tagged);

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
ethcore_drop_port(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"vid",
				"portno",
				NULL
		};

		uint64_t dpid			= 0;
		uint16_t vid			= 0;
		uint32_t portno			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "KHI", kwlist,
				&dpid, &vid, &portno)) {
			return NULL;
		}

		roflibs::eth::cethcore::set_eth_core(rofl::cdpid(dpid)).set_vlan(vid).drop_phy_port(portno);

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
ethcore_has_port(PyObject *self, PyObject *args, PyObject* kw)
{
	try {
		char* kwlist [] = {
				"dpid",
				"vid",
				"portno",
				NULL
		};

		uint64_t dpid			= 0;
		uint16_t vid			= 0;
		uint32_t portno			= 0;

		if (!PyArg_ParseTupleAndKeywords(args, kw, "KHI", kwlist,
				&dpid, &vid, &portno)) {
			return NULL;
		}

		return Py_BuildValue("i", roflibs::eth::cethcore::set_eth_core(rofl::cdpid(dpid)).get_vlan(vid).has_phy_port(portno));
	} catch (eEthCoreNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "dpid not found");
	} catch (eVlanNotFound& e) {
		PyErr_SetString(PyExc_RuntimeError, "vid not found");
	} catch (rofl::eSysCall& e) {
		PyErr_SetString(PyExc_RuntimeError, "invalid address");
	}
	return NULL;
}



