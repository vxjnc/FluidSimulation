#include "engine.hpp"

#ifdef SCRIPTING_AVAILABLE

#include <string>

#include <Python.h>
#include <dlfcn.h>

extern "C" PyObject* PyInit_fluidsim();

namespace scripting {
    static bool g_available = false;
    static void* g_lib = nullptr;
    static std::function<void(const std::string&)> g_output_handler;

    static void (*s_Py_Initialize)() = nullptr;
    static void (*s_Py_Finalize)() = nullptr;
    static int (*s_PyRun_SimpleString)(const char*) = nullptr;
    static const char* (*s_Py_GetVersion)() = nullptr;
    static int (*s_PyImport_AppendInittab)(const char*, PyObject* (*)()) = nullptr;
    static int (*s_PyArg_ParseTuple)(PyObject*, const char*, ...) = nullptr;
    static PyObject* (*s_PyModule_Create2)(PyModuleDef*, int) = nullptr;
    static PyObject* s_Py_None = nullptr;
    static PyObject* g_tick_callback = nullptr;
    static PyObject* (*s_PyObject_CallNoArgs)(PyObject*) = nullptr;

    static std::string popen_result(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return {};
        }
        char buf[512] = {};
        fgets(buf, sizeof(buf), pipe);
        pclose(pipe);
        std::string s(buf);
        if (!s.empty() && s.back() == '\n') {
            s.pop_back();
        }
        return s;
    }

    static std::string find_python_exe() {
        for (const char* c : {"python3", "python3.12", "python3.11", "python3.10"}) {
            std::string r = popen_result(std::string("which ") + c + " 2>/dev/null");
            if (!r.empty()) {
                return r;
            }
        }
        return {};
    }

    static std::string find_libpython(const std::string& python_exe) {
        return popen_result(python_exe + " -c \"import sysconfig; print("
                                         "sysconfig.get_config_var('LIBDIR') + '/libpython'"
                                         " + sysconfig.get_config_var('LDVERSION') + '.so.1.0')\"");
    }

    template <typename T> static bool resolve(void* lib, const char* name, T& fn) {
        fn = reinterpret_cast<T>(dlsym(lib, name));
        if (!fn) {
            return false;
        }
        return true;
    }

    void set_output_handler(std::function<void(std::string_view)> handler) {
        g_output_handler = std::move(handler);
    }

    void set_tick_callback(void* cb) {
        Py_XDECREF(g_tick_callback);
        g_tick_callback = static_cast<PyObject*>(cb);
        Py_XINCREF(g_tick_callback);
    }

    static PyObject* init_fluidsim_io() {
        static PyMethodDef methods[] = {{"output",
                                         [](PyObject*, PyObject* args) -> PyObject* {
                                             const char* text = nullptr;
                                             if (s_PyArg_ParseTuple(args, "s", &text) && text &&
                                                 g_output_handler) {
                                                 g_output_handler(text);
                                             }
                                             s_Py_None->ob_refcnt++;
                                             return s_Py_None;
                                         },
                                         METH_VARARGS, nullptr},
                                        {nullptr, nullptr, 0, nullptr}};
        static PyModuleDef def = {
            PyModuleDef_HEAD_INIT, "_fluidsim_io", nullptr, -1, methods, nullptr, nullptr, nullptr, nullptr};
        return s_PyModule_Create2(&def, PYTHON_API_VERSION);
    }
    bool init() {
        std::string python_exe = find_python_exe();
        if (python_exe.empty()) {
            return false;
        }

        std::string libpath = find_libpython(python_exe);
        if (libpath.empty()) {
            return false;
        }

        std::string prefix = popen_result(python_exe + " -c \"import sys; print(sys.prefix)\"");

        g_lib = dlopen(libpath.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!g_lib) {
            return false;
        }

        if (!resolve(g_lib, "Py_Initialize", s_Py_Initialize) ||
            !resolve(g_lib, "Py_Finalize", s_Py_Finalize) ||
            !resolve(g_lib, "PyRun_SimpleString", s_PyRun_SimpleString) ||
            !resolve(g_lib, "Py_GetVersion", s_Py_GetVersion) ||
            !resolve(g_lib, "PyImport_AppendInittab", s_PyImport_AppendInittab) ||
            !resolve(g_lib, "PyArg_ParseTuple", s_PyArg_ParseTuple) ||
            !resolve(g_lib, "PyModule_Create2", s_PyModule_Create2) ||
            !resolve(g_lib, "_Py_NoneStruct", s_Py_None) ||
            !resolve(g_lib, "PyObject_CallNoArgs", s_PyObject_CallNoArgs)) {
            dlclose(g_lib);
            g_lib = nullptr;
            return false;
        }

        if (!prefix.empty()) {
            setenv("PYTHONHOME", prefix.c_str(), 1);
        }

        s_PyImport_AppendInittab("fluidsim", PyInit_fluidsim);
        s_PyImport_AppendInittab("_fluidsim_io", init_fluidsim_io);

        s_Py_Initialize();

        s_PyRun_SimpleString("import sys, _fluidsim_io\n"
                             "class _Capture:\n"
                             "    def write(self, text): _fluidsim_io.output(text)\n"
                             "    def flush(self): pass\n"
                             "sys.stdout = sys.stderr = _Capture()\n");
        g_available = true;
        return true;
    }

    void shutdown() {
        if (g_available) {
            s_Py_Finalize();
        }
        g_available = false;
        if (g_lib) {
            dlclose(g_lib);
            g_lib = nullptr;
        }
    }

    bool is_available() { return g_available; }

    bool run_string(const std::string& code) {
        if (!g_available) {
            return false;
        }
        return s_PyRun_SimpleString(code.c_str()) == 0;
    }

    void run_tick() {
        if (!g_available || !g_tick_callback) {
            return;
        }
        PyObject* res = s_PyObject_CallNoArgs(g_tick_callback);
        Py_XDECREF(res);
    }
}

#else

#include <functional>
#include <string>

namespace scripting {
    void set_tick_callback(void* cb) {}
    void set_output_handler(std::function<void(std::string_view)>) {}
    bool init() { return false; }
    void run_tick();
    void shutdown() {}
    bool is_available() { return false; }
    bool run_string(const std::string&) { return false; }
}

#endif
