#pragma once
#include <string>
#include <string_view>

class Script {
public:
    std::string code;
    void* tick_callback = nullptr;

    void append_output(std::string_view text) {
        output += text;
        if (output.size() > 10 * 1024) {
            output.erase(0, output.size() - 10 * 1024);
        }
    }
    const std::string& get_output() { return output; }
    void clear_output() { output.clear(); }

private:
    std::string output;
};
