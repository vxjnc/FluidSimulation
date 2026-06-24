#include "scripting_engine.hpp"

#include "src/scripting/python_binding.hpp"

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
    static auto methods = py_util::make_table({
        {"output",
         [](const char* text) {
             if (text && g_output_handler) {
                 g_output_handler(text);
             }
         }},
    });
    static PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        "_fluidsim_io",
        nullptr,
        -1,
        methods.data(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
    };
    return py::Module_Create2(&def, PYTHON_API_VERSION);
}

void ScriptingEngine::init(Application* app_, std::string_view pythonPath) {
    instance = this;
    app = app_;

    pythonPath_ = pythonPath.empty() ? find_python_exe() : std::string(pythonPath);
    if (pythonPath_.empty()) {
        return;
    }

    std::string libpath = find_libpython(pythonPath_);
    if (libpath.empty()) {
        return;
    }

    std::string prefix = popen_result(pythonPath_ + " -c \"import sys; print(sys.base_prefix)\"");
    if (!prefix.empty()) {
        prefix.erase(prefix.find_last_not_of("\n\r") + 1);
    }

    if (!prefix.empty()) {
        setenv("PYTHONHOME", prefix.c_str(), 1);
    }

    g_lib = dlopen(libpath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!g_lib) {
        return;
    }

    if (!py::resolve_all(g_lib)) {
        dlclose(g_lib);
        g_lib = nullptr;
        return;
    }

    py::append_inittab("fluidsim", PyInit_fluidsim);
    py::append_inittab("_fluidsim_io", init_fluidsim_io);

    py::initialize();

    py::run_simple_string(R"(
import sys, _fluidsim_io
class _Capture:
    def write(self, text): _fluidsim_io.output(text)
    def flush(self): pass
sys.stdout = sys.stderr = _Capture()
)",
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

bool ScriptingEngine::run_string(const std::string& code) {
    if (!available) {
        return false;
    }
    return py::run_simple_string(code.c_str(), nullptr) == 0;
}
void ScriptingEngine::stop_current_script() { set_tick_callback(nullptr); }

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
