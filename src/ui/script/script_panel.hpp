#pragma once

#include "sigslot/signal.hpp"
#include "src/scripting/script_source.hpp"
#include "src/ui/script/script_console.hpp"
#include "src/ui/script/script_editor.hpp"

class ScriptingEngine;

class ScriptPanel {
public:
    sigslot::signal<> addScript;
    sigslot::signal<size_t> removeScript;

    void render(bool& open, ScriptingEngine& engine, std::span<ScriptSource> scripts);
    void renderTab(ScriptSource& s, ScriptingEngine& engine);

private:
    size_t active_idx_ = 0;
    ScriptEditor editor_;
    ScriptConsole console_;
};
