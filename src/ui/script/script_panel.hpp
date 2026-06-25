#pragma once
#ifdef SCRIPTING_AVAILABLE

#include "src/ui/script/script_console.hpp"
#include "src/ui/script/script_editor.hpp"

class ScriptPanel {
public:
    void render(bool& open);
    void renderTab(size_t index);

private:
    size_t active_idx_ = 0;
    ScriptEditor editor_;
    ScriptConsole console_;
};

#endif
