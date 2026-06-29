#ifdef SCRIPTING_AVAILABLE

#include "scripting_engine.hpp"

#include <string>

#include <Python.h>
#include <nanobind/nanobind.h>

#include "src/utils/python_find.hpp"

extern "C" PyObject* PyInit_fluidsim();
extern "C" PyObject* PyInit__fluidsim_io();

namespace nb = nanobind;

ScriptingEngine* ScriptingEngine::instance = nullptr;

class ScriptingEngineImpl : public ScriptingEngine {
public:
    ScriptingEngineImpl(std::vector<FluidSource>* sources_, std::string_view pythonPath) {
        instance = this;
        sources = sources_;

        pythonPath_ = pythonPath;

        std::string prefix = python_find::find_prefix(pythonPath_);
        if (!prefix.empty()) {
            setenv("PYTHONHOME", prefix.c_str(), 1);
        }

        PyImport_AppendInittab("_fluidsim_io", PyInit__fluidsim_io);
        PyImport_AppendInittab("fluidsim", PyInit_fluidsim);
        Py_Initialize();

#ifdef NDEBUG
        PyObject* mod = PyImport_ImportModule("fluidsim");
        if (mod) {
            Py_DECREF(mod);
        }
        nb::set_leak_warnings(false);
#endif

        PyObject* err = PyErr_Occurred();
        if (err) {
            PyErr_Print();
            PyErr_Clear();
        }

        PyRun_SimpleString(R"(
import sys, _fluidsim_io
class _Capture:
    def write(self, text): _fluidsim_io.output(text)
    def flush(self): pass
sys.stdout = sys.stderr = _Capture()
)");
    }

    ~ScriptingEngineImpl() override {
        for (size_t i = 0; i < scripts_.size(); i++) {
            stop_script(i);
        }
        scripts_.clear();
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

        if (s.globals) {
            Py_DECREF(static_cast<PyObject*>(s.globals));
            s.globals = nullptr;
        }
        nb::dict globals;
        nb::object builtins = nb::module_::import_("builtins");
        globals["__builtins__"] = builtins;
        s.globals = globals.release().ptr();

        current_script = &s;

        PyObject* compiled = Py_CompileString(s.code.c_str(), "<script>", Py_file_input);
        if (compiled) {
            PyObject* result = PyEval_EvalCode(compiled, static_cast<PyObject*>(s.globals),
                                               static_cast<PyObject*>(s.globals));
            Py_DECREF(compiled);
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
        if (s.globals) {
            Py_DECREF(static_cast<PyObject*>(s.globals));
            s.globals = nullptr;
        }
        if (s.panel) {
            for (auto& w : s.panel->widgets) {
                if (auto* b = std::get_if<Button>(&w)) {
                    b->on_click = nullptr;
                }
            }
            s.panel.reset();
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
