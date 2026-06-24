#pragma once

#include <functional>
#include <string>
#include <string_view>

class Application;

class ScriptingEngine {
public:
    void init(Application* app, std::string_view pythonPath = "");
    ~ScriptingEngine();

    bool is_available() { return available; }
    bool run_string(const std::string& code);

    void set_tick_callback(void* cb);
    void tick();

    void set_output_handler(std::function<void(std::string_view)> handler);

    std::string_view python_path() { return pythonPath_; }

    static ScriptingEngine* instance;
    Application* app;

private:
    std::string pythonPath_;
    bool available = false;
};
