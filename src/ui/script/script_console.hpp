#pragma once
#include <string>
#include <vector>

class ScriptConsole {
public:
    void append(std::string_view text);
    void clear();
    void render(float height);

private:
    std::vector<std::string> lines_;
    bool scrollToBottom_ = false;
};
