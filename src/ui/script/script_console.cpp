#include "script_console.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "src/scripting/scripting_engine.hpp"

void ScriptConsole::clear() { ScriptingEngine::instance->script().clear_output(); }

void ScriptConsole::render(float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::BeginChild("##console", ImVec2(-1, height), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleColor();

    ImGui::TextUnformatted(ScriptingEngine::instance->script().get_output().c_str());

    ImGui::EndChild();
}
