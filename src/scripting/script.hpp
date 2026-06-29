#pragma once
#ifdef SCRIPTING_AVAILABLE
#include <optional>
#include <string>
#include <string_view>

#include "src/ui/script/plugin/plugin_panel.hpp"

class Script {
public:
    void append_output(std::string_view text) {
        output += text;
        if (output.size() > 10 * 1024) {
            output.erase(0, output.size() - 10 * 1024);
        }
    }
    const std::string& get_output() { return output; }
    void clear_output() { output.clear(); }

    std::string code;
    std::optional<PluginPanel> panel;

    void* tick_callback = nullptr; // PyObject*
    void* compiled = nullptr;      // PyObject*
    void* globals;                 // PyObject*
    size_t code_hash = 0;

private:
    std::string output;
};

#endif
