#ifdef SCRIPTING_AVAILABLE

#include "engine.hpp"

#include <Python.h>
#include <dlfcn.h>

extern "C" void _Py_Dealloc(PyObject*) {}

static int (*local_PyArg_ParseTuple)(PyObject*, const char*, ...) = nullptr;
static int (*local_PyCallable_Check)(PyObject*) = nullptr;
static void (*local_PyErr_SetString)(PyObject*, const char*) = nullptr;
static PyObject* local_PyExc_TypeError = nullptr;
static PyObject* (*local_PyModule_Create2)(PyModuleDef*, int) = nullptr;
static PyObject* local_Py_None = nullptr;
static void (*local_Py_IncRef)(PyObject*) = nullptr;

static bool init_python_symbols() {
    if (local_PyArg_ParseTuple) {
        return true;
    }

    local_PyArg_ParseTuple = (int (*)(PyObject*, const char*, ...))dlsym(RTLD_DEFAULT, "PyArg_ParseTuple");
    local_PyCallable_Check = (int (*)(PyObject*))dlsym(RTLD_DEFAULT, "PyCallable_Check");
    local_PyErr_SetString = (void (*)(PyObject*, const char*))dlsym(RTLD_DEFAULT, "PyErr_SetString");

    void** type_error_ptr = (void**)dlsym(RTLD_DEFAULT, "PyExc_TypeError");
    if (type_error_ptr) {
        local_PyExc_TypeError = static_cast<PyObject*>(*type_error_ptr);
    }

    local_PyModule_Create2 = (PyObject * (*)(PyModuleDef*, int)) dlsym(RTLD_DEFAULT, "PyModule_Create2");
    local_Py_None = (PyObject*)dlsym(RTLD_DEFAULT, "_Py_NoneStruct");
    local_Py_IncRef = (void (*)(PyObject*))dlsym(RTLD_DEFAULT, "Py_IncRef");

    return local_PyArg_ParseTuple && local_PyCallable_Check && local_PyErr_SetString &&
           local_PyExc_TypeError && local_PyModule_Create2 && local_Py_None && local_Py_IncRef;
}

static PyObject* fluidsim_on_tick(PyObject* /*self*/, PyObject* args) {
    if (!init_python_symbols()) {
        return nullptr;
    }

    PyObject* callback_obj = nullptr;

    if (!local_PyArg_ParseTuple(args, "O", &callback_obj)) {
        return nullptr;
    }

    if (!local_PyCallable_Check(callback_obj)) {
        local_PyErr_SetString(local_PyExc_TypeError, "argument must be callable");
        return nullptr;
    }

    scripting::set_tick_callback(callback_obj);

    local_Py_IncRef(local_Py_None);
    return local_Py_None;
}

static PyMethodDef FluidSimMethods[] = {{"on_tick", (PyCFunction)fluidsim_on_tick, METH_VARARGS, nullptr},
                                        {nullptr, nullptr, 0, nullptr}};

static struct PyModuleDef fluidsim_module = {};

extern "C" PyMODINIT_FUNC PyInit_fluidsim(void) {
    if (!init_python_symbols()) {
        return nullptr;
    }

    fluidsim_module.m_base.ob_base.ob_refcnt = 1;
    fluidsim_module.m_name = "fluidsim";
    fluidsim_module.m_doc = "FluidSimulation scripting API";
    fluidsim_module.m_size = -1;
    fluidsim_module.m_methods = FluidSimMethods;

    return local_PyModule_Create2(&fluidsim_module, PYTHON_API_VERSION);
}

#endif
