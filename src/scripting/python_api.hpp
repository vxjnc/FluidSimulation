#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <Python.h>

namespace py {
    extern int (*ArgParseTuple)(PyObject*, const char*, ...);
    extern int (*Callable_Check)(PyObject*);
    extern void (*Err_SetString)(PyObject*, const char*);
    extern PyObject* (*Module_Create2)(PyModuleDef*, int);
    extern void (*incref)(PyObject*);
    extern void (*decref)(PyObject*);
    extern PyObject* none;
    extern PyObject* type_error;
    extern void (*initialize)();
    extern void (*finalize)();
    extern int (*run_simple_string)(const char*, void*);
    extern int (*append_inittab)(const char*, PyObject* (*)());
    extern PyObject* (*call_no_args)(PyObject*);

    bool resolve_all(void* lib);
}

#endif
