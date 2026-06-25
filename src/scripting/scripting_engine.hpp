#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <string>
#include <string_view>
#include <vector>

#include "src/scripting/script.hpp"

class Application;

class ScriptingEngine {
public:
    void init(Application* app, std::string_view pythonPath = "");
    ~ScriptingEngine();

    bool is_available() { return available; }
    size_t add_script();
    void remove_script(size_t idx);
    void run_script(size_t idx);
    void stop_script(size_t idx);

    void set_tick_callback(void* cb);
    void tick();

    std::string_view python_path() { return pythonPath_; }
    std::vector<Script>& scripts() { return scripts_; }

    static ScriptingEngine* instance;
    Application* app;
    Script* current_script = nullptr;

private:
    std::string pythonPath_;
    bool available = false;
    std::vector<Script> scripts_;
};

#endif
