#pragma once
#include <string>

class ScriptConsole {
public:
    void append(std::string_view text);
    void clear();
    void render(float height);

private:
    std::string text_;
    bool scrollToBottom_ = false;
};
