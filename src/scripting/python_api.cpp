#ifdef SCRIPTING_AVAILABLE

#include "python_api.hpp"

#include <dlfcn.h>

namespace py {
    int (*ArgParseTuple)(PyObject*, const char*, ...) = nullptr;
    int (*Callable_Check)(PyObject*) = nullptr;
    void (*Err_SetString)(PyObject*, const char*) = nullptr;
    PyObject* (*Module_Create2)(PyModuleDef*, int) = nullptr;
    void (*incref)(PyObject*) = nullptr;
    void (*decref)(PyObject*) = nullptr;
    PyObject* none = nullptr;
    PyObject* type_error = nullptr;
    void (*initialize)() = nullptr;
    void (*finalize)() = nullptr;
    int (*run_simple_string)(const char*, void*) = nullptr;
    int (*append_inittab)(const char*, PyObject* (*)()) = nullptr;
    PyObject* (*call_no_args)(PyObject*) = nullptr;
    PyObject* (*long_from_size_t)(size_t) = nullptr;

    bool resolve_all(void* lib) {
        auto r = [lib](const char* name, auto& fn) -> bool {
            fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(dlsym(lib, name));
            return fn != nullptr;
        };

        none = static_cast<PyObject*>(dlsym(lib, "_Py_NoneStruct"));
        type_error = *reinterpret_cast<PyObject**>(dlsym(lib, "PyExc_TypeError"));

        return none && type_error && r("PyArg_ParseTuple", ArgParseTuple) &&
               r("PyCallable_Check", Callable_Check) && r("PyErr_SetString", Err_SetString) &&
               r("PyModule_Create2", Module_Create2) && r("Py_IncRef", incref) && r("Py_DecRef", decref) &&
               r("Py_Initialize", initialize) && r("Py_Finalize", finalize) &&
               r("PyRun_SimpleStringFlags", run_simple_string) &&
               r("PyImport_AppendInittab", append_inittab) && r("PyObject_CallNoArgs", call_no_args) &&
               r("PyLong_FromSize_t", long_from_size_t);
    }
}

#endif
