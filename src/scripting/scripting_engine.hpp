#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <string_view>
#include <vector>

#include "src/compute/fluid_source.hpp"
#include "src/scripting/script.hpp"

class Application;

class ScriptingEngine {
public:
    virtual ~ScriptingEngine() = default;

    virtual bool is_available() { return false; }
    virtual size_t add_script() { return 0; }
    virtual void remove_script(size_t) {}
    virtual void run_script(size_t) {}
    virtual void stop_script(size_t) {}
    virtual void set_tick_callback(void*) {}
    virtual void tick() {}
    virtual std::string_view python_path() { return ""; }
    virtual std::vector<Script>& scripts() {
        static std::vector<Script> empty;
        return empty;
    }

    static ScriptingEngine* instance;

    std::vector<FluidSource>* sources = nullptr;
    Script* current_script = nullptr;
};

extern "C" ScriptingEngine* create_scripting_engine(std::vector<FluidSource>* sources,
                                                    std::string_view pythonPath);
extern "C" void destroy_scripting_engine(ScriptingEngine* engine);

#endif
