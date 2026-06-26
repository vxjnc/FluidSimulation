#ifdef SCRIPTING_AVAILABLE

#include "scripting_engine.hpp"

#include <string>

#include <Python.h>
#include <dlfcn.h>

extern "C" PyObject* PyInit_fluidsim();
extern "C" PyObject* PyInit__fluidsim_io();

ScriptingEngine* ScriptingEngine::instance = nullptr;
namespace {
    static std::string popen_result(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return {};
        }
        char buf[1024] = {};
        fgets(buf, sizeof(buf), pipe);
        pclose(pipe);
        std::string s(buf);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
            s.pop_back();
        }
        return s;
    }
}

class ScriptingEngineImpl : public ScriptingEngine {
public:
    ScriptingEngineImpl(std::vector<FluidSource>* sources_, std::string_view pythonPath) {
        instance = this;
        sources = sources_;

        pythonPath_ = pythonPath;

        std::string prefix = popen_result(pythonPath_ + " -c \"import sys; print(sys.base_prefix)\"");
        if (!prefix.empty()) {
            setenv("PYTHONHOME", prefix.c_str(), 1);
        }

        PyImport_AppendInittab("_fluidsim_io", PyInit__fluidsim_io);
        PyImport_AppendInittab("fluidsim", PyInit_fluidsim);
        Py_Initialize();

        PyRun_SimpleString(R"(
import sys, _fluidsim_io
class _Capture:
    def write(self, text): _fluidsim_io.output(text)
    def flush(self): pass
sys.stdout = sys.stderr = _Capture()
)");
    }

    ~ScriptingEngineImpl() override {
        for (auto& s : scripts_) {
            if (s.tick_callback) {
                Py_DECREF(static_cast<PyObject*>(s.tick_callback));
            }
            if (s.compiled) {
                Py_DECREF(static_cast<PyObject*>(s.compiled));
            }
            if (s.globals) {
                Py_DECREF(static_cast<PyObject*>(s.globals));
            }
        }
        Py_FinalizeEx();
        instance = nullptr;
    }

    bool is_available() override { return true; }
    std::string_view python_path() override { return pythonPath_; }
    std::vector<Script>& scripts() override { return scripts_; }

    size_t add_script() override {
        scripts_.emplace_back();
        return scripts_.size() - 1;
    }

    void remove_script(size_t idx) override {
        stop_script(idx);
        scripts_.erase(scripts_.begin() + idx);
    }

    void run_script(size_t idx) override {
        stop_script(idx);
        Script& s = scripts_[idx];
        s.clear_output();

        if (!s.globals) {
            s.globals = PyDict_New();
            PyObject* builtins = PyImport_ImportModule("builtins");
            PyDict_SetItemString(static_cast<PyObject*>(s.globals), "__builtins__", builtins);
            Py_DECREF(builtins);
        }

        current_script = &s;

        size_t new_hash = std::hash<std::string>{}(s.code);
        if (!s.compiled || s.code_hash != new_hash) {
            if (s.compiled) {
                Py_DECREF(static_cast<PyObject*>(s.compiled));
            }
            s.compiled = Py_CompileString(s.code.c_str(), "<script>", Py_file_input);
            s.code_hash = new_hash;
        }

        if (s.compiled) {
            PyObject* result =
                PyEval_EvalCode(static_cast<PyObject*>(s.compiled), static_cast<PyObject*>(s.globals),
                                static_cast<PyObject*>(s.globals));
            if (result) {
                Py_DECREF(result);
            }
            else {
                PyErr_Print();
                PyErr_Clear();
            }
        }
        else {
            PyErr_Print();
            PyErr_Clear();
        }

        current_script = nullptr;
    }

    void stop_script(size_t idx) override {
        auto& s = scripts_[idx];
        if (s.tick_callback) {
            Py_DECREF(static_cast<PyObject*>(s.tick_callback));
            s.tick_callback = nullptr;
        }
    }

    void set_tick_callback(void* cb) override {
        if (!current_script) {
            return;
        }
        if (current_script->tick_callback) {
            Py_DECREF(static_cast<PyObject*>(current_script->tick_callback));
        }
        current_script->tick_callback = cb;
        if (cb) {
            Py_INCREF(static_cast<PyObject*>(cb));
        }
    }

    void tick() override {
        for (auto& s : scripts_) {
            if (!s.tick_callback) {
                continue;
            }
            current_script = &s;
            PyObject* res = PyObject_CallNoArgs(static_cast<PyObject*>(s.tick_callback));
            if (res) {
                Py_DECREF(res);
            }
            else {
                PyErr_Print();
                PyErr_Clear();
            }
            current_script = nullptr;
        }
    }

private:
    std::string pythonPath_;
    std::vector<Script> scripts_;
};

extern "C" ScriptingEngine* create_scripting_engine(std::vector<FluidSource>* sources,
                                                    std::string_view pythonPath) {
    return new ScriptingEngineImpl(sources, pythonPath);
}
extern "C" void destroy_scripting_engine(ScriptingEngine* engine) { delete engine; }

#endif
