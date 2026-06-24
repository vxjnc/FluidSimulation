#pragma once

#include <string>
#include <string_view>

#include "src/scripting/script.hpp"

class Application;

class ScriptingEngine {
public:
    void init(Application* app, std::string_view pythonPath = "");
    ~ScriptingEngine();

    bool is_available() { return available; }
    bool run_string(const std::string& code);
    void stop_current_script();

    void set_tick_callback(void* cb);
    void tick();

    std::string_view python_path() { return pythonPath_; }
    Script& script() { return script_; }

    static ScriptingEngine* instance;
    Application* app;

private:
    std::string pythonPath_;
    bool available = false;
    Script script_;
};
