#pragma once

#include "TextEditor.h"

#include <string>

class ScriptEditor {
public:
    ScriptEditor();
    bool render(float height);
    std::string code() const { return editor_.GetText(); };
    void set_code(std::string_view text) { editor_.SetText(text); }

private:
    TextEditor editor_;
};
