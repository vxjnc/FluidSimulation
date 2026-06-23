#pragma once

#include <functional>
#include <string>

class Application;

class ScriptingEngine {
public:
    ScriptingEngine(Application* app);
    ~ScriptingEngine();

    bool is_available() { return available; }
    bool run_string(const std::string& code);

    void set_tick_callback(void* cb);
    void tick();

    void set_output_handler(std::function<void(std::string_view)> handler);

    static ScriptingEngine* instance;
    Application* app;

private:
    bool available = false;
};
