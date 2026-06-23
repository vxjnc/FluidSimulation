#include "script_editor.hpp"

#include <imgui.h>

ScriptEditor::ScriptEditor() {
    editor_.SetLanguage(TextEditor::Language::Python());
    editor_.SetText("print('hello')");
}

bool ScriptEditor::render(float height) {
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    float btn_h = ImGui::CalcTextSize("Run").y + padding.y * 2;
    float btn_w = ImGui::CalcTextSize("Run").x + padding.x * 2;

    editor_.Render("##code", ImVec2(-1, height - btn_h - ImGui::GetStyle().ItemSpacing.y * 2));

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btn_w);
    bool run = ImGui::Button("Run");

    return run;
}
