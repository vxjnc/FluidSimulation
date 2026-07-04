#pragma once

#include <string_view>
#include <vector>

#include "src/compute/fluid_source.hpp"
#include "src/utils/dynlib.hpp"

class Application;
class ScriptingEngine;
class NotificationManager;

class ScriptingLoader {
public:
    ScriptingLoader() = default;
    void init(std::vector<FluidSource>* sources, NotificationManager* notifications,
              std::string_view pythonPath = "");
    ~ScriptingLoader();

    ScriptingLoader(const ScriptingLoader&) = delete;
    ScriptingLoader& operator=(const ScriptingLoader&) = delete;

    ScriptingEngine& engine() { return *engine_; }

private:
    DynLib libpython_;
    DynLib libscripting_;
    ScriptingEngine* engine_ = nullptr;
};
