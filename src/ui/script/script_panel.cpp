#include "script_panel.hpp"

#include <imgui.h>

#include "src/scripting/engine.hpp"

ScriptPanel::ScriptPanel() {
    scripting::set_output_handler([this](std::string_view text) { console_.append(text); });
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
        if (scripting::is_available()) {
            scripting::run_string(editor_.code());
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
    ImGui::TextDisabled("console");
    console_.render(consoleH);

    ImGui::End();
}
