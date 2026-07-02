#pragma once

#include <string_view>
#include <vector>

#include "src/compute/fluid_source.hpp"
#include "src/scripting/script_source.hpp"
#include "src/ui/script/plugin/script_panel.hpp"

class Application;

class ScriptingEngine {
public:
    virtual ~ScriptingEngine() = default;

    virtual bool is_available() { return false; }

    virtual void run_script(const ScriptSource&) {}
    virtual void stop_script(size_t) {}

    virtual void set_tick_callback(void*) {}
    virtual void tick() {}

    virtual void append_output(std::string_view) {}
    virtual const std::string& get_output(size_t) const {
        static std::string empty;
        return empty;
    }
    virtual void clear_output(size_t) {}

    virtual void set_panel(ScriptPanel) {}
    virtual void set_widget_value(const std::string& id, ExportValue value) {};

    virtual std::string_view python_path() { return ""; }

    virtual void set_current_context(size_t) {}
    virtual void for_each_panel(std::function<void(size_t id, ScriptPanel&)>) {}

    static ScriptingEngine* instance;
    std::vector<FluidSource>* sources = nullptr;
};

extern "C" ScriptingEngine* create_scripting_engine(std::vector<FluidSource>* sources,
                                                    std::string_view pythonPath);
extern "C" void destroy_scripting_engine(ScriptingEngine* engine);
