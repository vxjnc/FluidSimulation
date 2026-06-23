#ifdef SCRIPTING_AVAILABLE

#include "engine.hpp"
#include "python_api.hpp"

#include <Python.h>

#include "src/app.hpp"

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

static PyObject* fluidsim_add_source(PyObject*, PyObject* args) {
    float x, y, vx, vy, radius, r, g, b;
    if (!py::ArgParseTuple(args, "ffffffff", &x, &y, &vx, &vy, &radius, &r, &g, &b)) {
        return nullptr;
    }
    auto& sources = scripting::app->getSources();
    sources.push_back(FluidSource(x, y, vx, vy, radius, std::array<float, 3>{r, g, b}));
    return py::long_from_size_t(sources.size() - 1);
}

static PyObject* fluidsim_remove_source(PyObject*, PyObject* args) {
    int idx;
    if (!py::ArgParseTuple(args, "i", &idx)) {
        return nullptr;
    }
    auto& sources = scripting::app->getSources();
    if (idx < 0 || idx >= (int)sources.size()) {
        py::Err_SetString(py::type_error, "index out of range");
        return nullptr;
    }
    sources.erase(sources.begin() + idx);
    py::incref(py::none);
    return py::none;
}

static PyObject* fluidsim_set_source(PyObject*, PyObject* args) {
    int idx;
    float x, y, vx, vy, radius, r, g, b;
    if (!py::ArgParseTuple(args, "iffffffff", &idx, &x, &y, &vx, &vy, &radius, &r, &g, &b)) {
        return nullptr;
    }
    auto& sources = scripting::app->getSources();
    if (idx < 0 || idx >= (int)sources.size()) {
        py::Err_SetString(py::type_error, "index out of range");
        return nullptr;
    }
    sources[idx] = FluidSource(x, y, vx, vy, radius, std::array<float, 3>{r, g, b});
    py::incref(py::none);
    return py::none;
}

static PyMethodDef FluidSimMethods[] = {
    {"on_tick", (PyCFunction)fluidsim_on_tick, METH_VARARGS, nullptr},
    {"add_source", (PyCFunction)fluidsim_add_source, METH_VARARGS, nullptr},
    {"remove_source", (PyCFunction)fluidsim_remove_source, METH_VARARGS, nullptr},
    {"set_source", (PyCFunction)fluidsim_set_source, METH_VARARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}};

static PyModuleDef fluidsim_module = {
    PyModuleDef_HEAD_INIT, "fluidsim", nullptr, -1, FluidSimMethods, nullptr, nullptr, nullptr, nullptr};

PyMODINIT_FUNC PyInit_fluidsim() { return py::Module_Create2(&fluidsim_module, PYTHON_API_VERSION); }

#endif
