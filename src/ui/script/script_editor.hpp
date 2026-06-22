#pragma once
#include <string>

class ScriptConsole;

class ScriptEditor {
public:
    bool render(float height);

    const std::string& code() const { return code_; }

private:
    std::string code_ = "print('hello')";
};
