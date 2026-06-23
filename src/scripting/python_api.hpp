#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <Python.h>

namespace py {
    extern int (*ArgParseTuple)(PyObject*, const char*, ...);              // PyArg_ParseTuple
    extern int (*Callable_Check)(PyObject*);                               // PyCallable_Check
    extern void (*Err_SetString)(PyObject*, const char*);                  // PyErr_SetString
    extern PyObject* (*Module_Create2)(PyModuleDef*, int);                 // PyModule_Create2
    extern void (*incref)(PyObject*);                                      // Py_IncRef
    extern void (*decref)(PyObject*);                                      // Py_DecRef
    extern void (*initialize)();                                           // Py_Initialize
    extern void (*finalize)();                                             // Py_Finalize
    extern int (*run_simple_string)(const char*, PyCompilerFlags*);        // PyRun_SimpleStringFlags
    extern int (*append_inittab)(const char*, PyObject* (*)());            // PyImport_AppendInittab
    extern PyObject* (*call_no_args)(PyObject*);                           // PyObject_CallNoArgs
    extern PyObject* (*long_from_size_t)(size_t);                          // PyLong_FromSize_t
    extern PyObject* (*float_from_double)(double);                         // PyFloat_FromDouble
    extern PyObject* (*list_new)(Py_ssize_t);                              // PyList_New
    extern int (*list_set_item)(PyObject*, Py_ssize_t, PyObject*);         // PyList_SetItem
    extern PyObject* (*dict_new)();                                        // PyDict_New
    extern int (*dict_set_item_string)(PyObject*, const char*, PyObject*); // PyDict_SetItemString
    extern PyObject* (*tuple_new)(Py_ssize_t);                             // PyTuple_New
    extern int (*tuple_set_item)(PyObject*, Py_ssize_t, PyObject*);        // PyTuple_SetItem
    extern PyObject* (*tuple_pack)(Py_ssize_t, ...);                       // PyTuple_Pack
    extern PyObject* (*bool_from_long)(long);                              // PyBool_FromLong
    extern PyObject* (*unicode_from_string)(const char*);                  // PyUnicode_FromString
    extern PyObject* (*import_import_module)(const char*);                 // PyImport_ImportModule
    extern PyObject* (*object_get_attr_string)(PyObject*, const char*);    // PyObject_GetAttrString
    extern PyObject* (*object_call)(PyObject*, PyObject*, PyObject*);      // PyObject_Call
    extern int (*module_add_object)(PyObject*, const char*, PyObject*);    // PyModule_AddObject

    extern PyObject* none;       // _Py_NoneStruct
    extern PyObject* type_error; // PyExc_TypeError

    bool resolve_all(void* lib);
}

#endif
