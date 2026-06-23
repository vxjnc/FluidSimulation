#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <Python.h>

namespace py {
    extern int (*ArgParseTuple)(PyObject*, const char*, ...);       // PyArg_ParseTuple
    extern int (*Callable_Check)(PyObject*);                        // PyCallable_Check
    extern void (*Err_SetString)(PyObject*, const char*);           // PyErr_SetString
    extern PyObject* (*Module_Create2)(PyModuleDef*, int);          // PyModule_Create2
    extern void (*incref)(PyObject*);                               // Py_IncRef
    extern void (*decref)(PyObject*);                               // Py_DecRef
    extern void (*initialize)();                                    // Py_Initialize
    extern void (*finalize)();                                      // Py_Finalize
    extern int (*run_simple_string)(const char*, PyCompilerFlags*); // PyRun_SimpleStringFlags
    extern int (*append_inittab)(const char*, PyObject* (*)());     // PyImport_AppendInittab
    extern PyObject* (*call_no_args)(PyObject*);                    // PyObject_CallNoArgs
    extern PyObject* (*long_from_size_t)(size_t);                   // PyLong_FromSize_t
    extern PyObject* (*float_from_double)(double);                  // PyFloat_FromDouble

    extern PyObject* none;       // _Py_NoneStruct
    extern PyObject* type_error; // PyExc_TypeError

    bool resolve_all(void* lib);
}

#endif
