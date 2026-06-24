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

static FILE* cross_popen(const char* command, const char* mode) {
#ifdef _WIN32
    return _popen(command, mode);
#else
    return popen(command, mode);
#endif
}

static int cross_pclose(FILE* stream) {
#ifdef _WIN32
    return _pclose(stream);
#endif
    return pclose(stream);
}

static std::string popen_result(const std::string& cmd) {
    FILE* pipe = cross_popen(cmd.c_str(), "r");
    if (!pipe) {
        return {};
    }
    char buf[1024] = {};
    fgets(buf, sizeof(buf), pipe);
    cross_pclose(pipe);
    std::string s(buf);

    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
    return s;
}

static std::string find_python_exe() {
#ifdef _WIN32
    for (const char* c : {"python.exe", "python3.exe"}) {
        std::string r = popen_result(std::format("where {} 2>nul", c));
        if (!r.empty()) {
            return r;
        }
    }
#else
    for (const char* c : {"python3", "python3.12", "python3.11", "python3.10"}) {
        std::string r = popen_result(std::format("which {} 2>/dev/null", c));
        if (!r.empty()) {
            return r;
        }
    }
#endif
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
             if (text && ScriptingEngine::instance->current_script) {
                 ScriptingEngine::instance->current_script->append_output(text);
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

    pythonPath_ = pythonPath.empty() ? find_python_exe() : pythonPath;
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

size_t ScriptingEngine::add_script() {
    scripts_.emplace_back();
    return scripts_.size() - 1;
}

void ScriptingEngine::remove_script(size_t idx) {
    stop_script(idx);
    scripts_.erase(scripts_.begin() + idx);
}

void ScriptingEngine::run_script(size_t idx) {
    if (!available) {
        return;
    }
    stop_script(idx);
    Script& s = scripts_[idx];
    s.clear_output();

    if (!s.globals) {
        s.globals = py::dict_new();
        PyObject* builtins = py::import_import_module("builtins");
        py::dict_set_item_string(static_cast<PyObject*>(s.globals), "__builtins__", builtins);
        py::decref(builtins);
    }
    current_script = &s;
    py::run_string(s.code.c_str(), Py_file_input, static_cast<PyObject*>(s.globals),
                   static_cast<PyObject*>(s.globals));
    current_script = nullptr;
}

void ScriptingEngine::stop_script(size_t idx) {
    auto& s = scripts_[idx];
    if (s.tick_callback) {
        py::decref(static_cast<PyObject*>(s.tick_callback));
        s.tick_callback = nullptr;
    }
}

void ScriptingEngine::set_tick_callback(void* cb) {
    if (!current_script) {
        return;
    }
    if (current_script->tick_callback) {
        py::decref(static_cast<PyObject*>(current_script->tick_callback));
    }
    current_script->tick_callback = cb;
    if (cb) {
        py::incref(static_cast<PyObject*>(cb));
    }
}

void ScriptingEngine::tick() {
    if (!available) {
        return;
    }
    for (auto& s : scripts_) {
        if (!s.tick_callback) {
            continue;
        }
        current_script = &s;
        PyObject* res = py::call_no_args(static_cast<PyObject*>(s.tick_callback));
        if (res) {
            py::decref(res);
        }
        current_script = nullptr;
    }
}

#else

ScriptingEngine* ScriptingEngine::instance = nullptr;

bool ScriptingEngine::init(Application*) { return false; }
void ScriptingEngine::shutdown() {}
bool ScriptingEngine::is_available() { return false; }
int ScriptingEngine::add_script() { return -1; }
void ScriptingEngine::remove_script(int) {}
void ScriptingEngine::run_script(int) {}
void ScriptingEngine::stop_script(int) {}
void ScriptingEngine::set_tick_callback(void*) {}
void ScriptingEngine::tick() {}
void ScriptingEngine::set_output_handler(std::function<void(std::string_view)>) {}

#endif
