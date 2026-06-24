#pragma once

#include "src/ui/script/script_console.hpp"
#include "src/ui/script/script_editor.hpp"

class ScriptPanel {
public:
    void render(bool& open);

private:
    ScriptEditor editor_;
    ScriptConsole console_;
};
