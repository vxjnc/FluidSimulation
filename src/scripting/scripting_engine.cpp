#include "scripting_engine.hpp"

#ifdef SCRIPTING_AVAILABLE

#include "python_api.hpp"

#include <string>

#include <Python.h>
#include <dlfcn.h>

extern "C" PyObject* PyInit_fluidsim();

ScriptingEngine* ScriptingEngine::instance = nullptr;

static void* g_lib = nullptr;
static std::function<void(std::string_view)> g_output_handler;
static PyObject* g_tick_callback = nullptr;

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

static PyObject* init_fluidsim_io() {
    static PyMethodDef methods[] = {{"output",
                                     [](PyObject*, PyObject* args) -> PyObject* {
                                         const char* text = nullptr;
                                         if (py::ArgParseTuple(args, "s", &text) && text &&
                                             g_output_handler) {
                                             g_output_handler(text);
                                         }
                                         py::incref(py::none);
                                         return py::none;
                                     },
                                     METH_VARARGS, nullptr},
                                    {nullptr, nullptr, 0, nullptr}};
    static PyModuleDef def = {
        PyModuleDef_HEAD_INIT, "_fluidsim_io", nullptr, -1, methods, nullptr, nullptr, nullptr, nullptr};
    return py::Module_Create2(&def, PYTHON_API_VERSION);
}

ScriptingEngine::ScriptingEngine(Application* app_) {
    instance = this;
    app = app_;

    std::string python_exe = find_python_exe();
    if (python_exe.empty()) {
        return;
    }

    std::string libpath = find_libpython(python_exe);
    if (libpath.empty()) {
        return;
    }

    std::string prefix = popen_result(python_exe + " -c \"import sys; print(sys.prefix)\"");

    g_lib = dlopen(libpath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!g_lib) {
        return;
    }

    if (!py::resolve_all(g_lib)) {
        dlclose(g_lib);
        g_lib = nullptr;
        return;
    }

    if (!prefix.empty()) {
        setenv("PYTHONHOME", prefix.c_str(), 1);
    }

    py::append_inittab("fluidsim", PyInit_fluidsim);
    py::append_inittab("_fluidsim_io", init_fluidsim_io);

    py::initialize();

    py::run_simple_string("import sys, _fluidsim_io\n"
                          "class _Capture:\n"
                          "    def write(self, text): _fluidsim_io.output(text)\n"
                          "    def flush(self): pass\n"
                          "sys.stdout = sys.stderr = _Capture()\n",
                          nullptr);

    available = true;
}

ScriptingEngine::~ScriptingEngine() {
    if (available) {
        py::finalize();
    }
    available = false;
    if (g_lib) {
        dlclose(g_lib);
        g_lib = nullptr;
    }
}

bool ScriptingEngine::is_available() { return available; }

bool ScriptingEngine::run_string(const std::string& code) {
    if (!available) {
        return false;
    }
    return py::run_simple_string(code.c_str(), nullptr) == 0;
}

void ScriptingEngine::set_tick_callback(void* cb) {
    if (g_tick_callback) {
        py::decref(g_tick_callback);
    }
    g_tick_callback = static_cast<PyObject*>(cb);
    if (g_tick_callback) {
        py::incref(g_tick_callback);
    }
}

void ScriptingEngine::tick() {
    if (!available || !g_tick_callback) {
        return;
    }
    PyObject* res = py::call_no_args(g_tick_callback);
    if (res) {
        py::decref(res);
    }
}

void ScriptingEngine::set_output_handler(std::function<void(std::string_view)> handler) {
    g_output_handler = std::move(handler);
}

#else

ScriptingEngine* ScriptingEngine::instance = nullptr;

bool ScriptingEngine::init(Application*) { return false; }
void ScriptingEngine::shutdown() {}
bool ScriptingEngine::is_available() { return false; }
bool ScriptingEngine::run_string(const std::string&) { return false; }
void ScriptingEngine::set_tick_callback(void*) {}
void ScriptingEngine::tick() {}
void ScriptingEngine::set_output_handler(std::function<void(std::string_view)>) {}

#endif
