#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <string_view>
#include <vector>

#include "src/compute/fluid_source.hpp"

class Application;
class ScriptingEngine;

class ScriptingLoader {
public:
    ScriptingLoader() = default;
    void init(std::vector<FluidSource>* sources, std::string_view pythonPath = "");
    ~ScriptingLoader();

    ScriptingLoader(const ScriptingLoader&) = delete;
    ScriptingLoader& operator=(const ScriptingLoader&) = delete;

    ScriptingEngine& engine() { return *engine_; }

private:
    void* libpython_ = nullptr;
    void* libscripting_ = nullptr;
    ScriptingEngine* engine_ = nullptr;
};

#endif
