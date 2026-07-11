#pragma once

#include <string_view>

#include "src/utils/dynlib.hpp"

class Application;
class ScriptingEngine;
class NotificationManager;

class ScriptingLoader {
public:
    ScriptingLoader() = default;
    void init(Application* app, std::string_view pythonPath = "");
    ~ScriptingLoader();

    ScriptingLoader(const ScriptingLoader&) = delete;
    ScriptingLoader& operator=(const ScriptingLoader&) = delete;

    ScriptingEngine& engine() { return *engine_; }

private:
    DynLib libpython_;
    DynLib libscripting_;
    ScriptingEngine* engine_ = nullptr;
};
