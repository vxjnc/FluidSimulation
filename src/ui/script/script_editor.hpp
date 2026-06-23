#pragma once
#include "TextEditor.h"

#include <string>

class ScriptEditor {
public:
    ScriptEditor();
    bool render(float height);
    std::string code() const { return editor_.GetText(); };

private:
    TextEditor editor_;
};
