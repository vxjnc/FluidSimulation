#ifdef SCRIPTING_AVAILABLE

#include "scripting_engine.hpp"

#include <chrono>
#include <cstdlib>
#include <format>
#include <string>

#include <Python.h>
#include <nanobind/eval.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "src/scripting/script_runtime.hpp"
#include "src/utils/python_find.hpp"

extern "C" PyObject* PyInit_fluidsim();
extern "C" PyObject* PyInit__fluidsim_io();

namespace nb = nanobind;

ScriptingEngine* ScriptingEngine::instance = nullptr;

class ScriptingEngineImpl : public ScriptingEngine {
public:
    ScriptingEngineImpl(ScriptHost* host_, std::string_view pythonPath) {
        instance = this;
        host = host_;
        pythonPath_ = pythonPath;

        std::string base_prefix = python_find::find_base_prefix(pythonPath_);
        if (!base_prefix.empty()) {
#ifdef _WIN32
            _putenv_s("PYTHONHOME", base_prefix.c_str());
#else
            setenv("PYTHONHOME", base_prefix.c_str(), 1);
#endif
        }

        PyImport_AppendInittab("_fluidsim_io", PyInit__fluidsim_io);
        PyImport_AppendInittab("fluidsim", PyInit_fluidsim);
        Py_Initialize();

        std::string site_packages = python_find::find_site_packages(pythonPath_);
        if (!site_packages.empty()) {
            nb::module_::import_("sys").attr("path").attr("insert")(0, site_packages);
        }

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

        nb::dict globals = nb::module_::import_("__main__").attr("__dict__");
        nb::exec(R"(
import sys, _fluidsim_io
class _Capture:
    def write(self, text): _fluidsim_io.output(text)
    def flush(self): pass
sys.stdout = sys.stderr = _Capture()
)",
                 globals);
    }

    ~ScriptingEngineImpl() override {
        runtimes_.clear();
        Py_FinalizeEx();
        instance = nullptr;
    }

    bool is_available() override { return true; }
    std::string_view python_path() override { return pythonPath_; }

    void run_script(const ScriptSource& source) override {
        clear_output(source.id);
        stop_script(source.id);

        ScriptRuntime rt;
        rt.globals["__builtins__"] = nb::module_::import_("builtins");

        current_id_ = source.id;
        current_script_ = &rt;

        auto compile_start = std::chrono::steady_clock::now();
        PyObject* compiled = Py_CompileString(source.code.c_str(),
                                              std::format("<script {}>", source.id).c_str(), Py_file_input);
        auto compile_elapsed = std::chrono::steady_clock::now() - compile_start;
        if (compiled) {
            auto start = std::chrono::steady_clock::now();
            PyObject* result = PyEval_EvalCode(compiled, rt.globals.ptr(), rt.globals.ptr());
            auto elapsed = std::chrono::steady_clock::now() - start;
            Py_DECREF(compiled);
            if (result) {
                Py_DECREF(result);
                auto compile_ms = std::chrono::duration<double, std::milli>(compile_elapsed).count();
                auto ms = std::chrono::duration<double, std::milli>(elapsed).count();
                append_output(std::format("Compiled in {:.2f}ms\nCompleted in {:.2f}ms\n", compile_ms, ms));
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

        current_script_ = nullptr;
        current_id_ = ScriptSource::INVALID_ID;

        if (rt.is_alive()) {
            runtimes_.emplace(source.id, std::move(rt));
        }
    }

    void stop_script(size_t id) override { runtimes_.erase(id); }

    void set_tick_callback(std::function<void()> cb) override {
        if (current_script_) {
            current_script_->tick_callback = std::move(cb);
        }
    }

    void tick() override {
        for (auto& [id, rt] : runtimes_) {
            if (!rt.tick_callback) {
                continue;
            }
            current_script_ = &rt;
            current_id_ = id;
            try {
                rt.tick_callback();
            }
            catch (nb::python_error& e) {
                host->notifications().error(e.what());
                e.restore();
                PyErr_Print();
                PyErr_Clear();
            }
            current_script_ = nullptr;
            current_id_ = ScriptSource::INVALID_ID;
        }
    }

    void append_output(std::string_view text) override {
        if (!current_script_) {
            return;
        }
        auto& out = outputs_[current_id_];
        out += text;
        constexpr size_t max_size = 10 * 1024;
        if (out.size() > max_size) {
            out.erase(0, out.size() - max_size);
        }
    }

    const std::string& get_output(size_t id) const override {
        static std::string empty;
        auto it = outputs_.find(id);
        return it != outputs_.end() ? it->second : empty;
    }

    void clear_output(size_t id) override { outputs_.erase(id); }

    void set_panel(ScriptPanel panel) override {
        if (current_script_) {
            current_script_->panel = std::move(panel);
        }
    }
    void set_widget_value(const std::string& id, ExportValue value) override {
        if (current_script_) {
            current_script_->panel->set_value(id, value);
        }
    }
    void set_widget_label(const std::string& id, std::string label) override {
        if (current_script_) {
            current_script_->panel->set_label(id, std::move(label));
        }
    }

    void set_current_context(size_t id) override {
        if (id == ScriptSource::INVALID_ID) {
            current_script_ = nullptr;
            current_id_ = ScriptSource::INVALID_ID;
            return;
        }
        auto it = runtimes_.find(id);
        current_script_ = it != runtimes_.end() ? &it->second : nullptr;
        current_id_ = id;
    }

    void for_each_panel(std::function<void(size_t id, ScriptPanel&)> fn) override {
        for (auto& [id, rt] : runtimes_) {
            if (rt.panel) {
                fn(id, *rt.panel);
            }
        }
    }

private:
    std::string pythonPath_;
    std::map<size_t, ScriptRuntime> runtimes_;
    std::map<size_t, std::string> outputs_;
    ScriptRuntime* current_script_ = nullptr;
    size_t current_id_ = ScriptSource::INVALID_ID;
};

extern "C" ScriptingEngine* create_scripting_engine(ScriptHost* host, std::string_view pythonPath) {
    return new ScriptingEngineImpl(host, pythonPath);
}
extern "C" void destroy_scripting_engine(ScriptingEngine* engine) { delete engine; }

#endif
