#ifdef SCRIPTING_AVAILABLE

#include "engine.hpp"
#include "python_api.hpp"

#include <Python.h>

extern "C" void _Py_Dealloc(PyObject*) {}

static PyObject* fluidsim_on_tick(PyObject*, PyObject* args) {
    PyObject* cb = nullptr;
    if (!py::ArgParseTuple(args, "O", &cb)) {
        return nullptr;
    }
    if (!py::Callable_Check(cb)) {
        py::Err_SetString(py::type_error, "argument must be callable");
        return nullptr;
    }
    scripting::set_tick_callback(cb);
    py::incref(py::none);
    return py::none;
}

static PyMethodDef FluidSimMethods[] = {{"on_tick", (PyCFunction)fluidsim_on_tick, METH_VARARGS, nullptr},
                                        {nullptr, nullptr, 0, nullptr}};

static PyModuleDef fluidsim_module = {
    PyModuleDef_HEAD_INIT, "fluidsim", nullptr, -1, FluidSimMethods, nullptr, nullptr, nullptr, nullptr};

PyMODINIT_FUNC PyInit_fluidsim() { return py::Module_Create2(&fluidsim_module, PYTHON_API_VERSION); }

#endif
