#include "script_panel.hpp"

#include <string_view>

#include <imgui.h>

#include "src/scripting/scripting_engine.hpp"

ScriptPanel::ScriptPanel() {
    ScriptingEngine::instance->set_output_handler([this](std::string_view text) { console_.append(text); });
}

void ScriptPanel::render(bool& open) {
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Script Editor", &open)) {
        ImGui::End();
        return;
    }

    float totalH = ImGui::GetContentRegionAvail().y;
    float editorH = totalH * 0.6f;
    float consoleH = totalH * 0.4f - ImGui::GetFrameHeightWithSpacing();

    if (editor_.render(editorH)) {
#ifdef SCRIPTING_AVAILABLE
        if (ScriptingEngine::instance->is_available()) {
            ScriptingEngine::instance->run_string(editor_.code());
        }
        else {
            console_.append("[error] Python not available");
        }
#else
        console_.append("[error] Scripting not compiled");
#endif
    }

    ImGui::Separator();
    if (ImGui::SmallButton("Clear")) {
        console_.clear();
    }
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    if (ImGui::SmallButton("Stop")) {
        ScriptingEngine::instance->stop_current_script();
    }
    ImGui::PopStyleColor();

    ImGui::TextDisabled("console");
    console_.render(consoleH);

    ImGui::End();
}
