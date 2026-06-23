#include "script_editor.hpp"

#include <imgui.h>

bool ScriptEditor::render(float height) {
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    float btn_h = ImGui::CalcTextSize("Run").y + padding.y * 2;
    float btn_w = ImGui::CalcTextSize("Run").x + padding.x * 2;

    ImGui::InputTextMultiline(
        "##code", code_.data(), code_.capacity(),
        ImVec2(-1, height - btn_h - ImGui::GetStyle().ItemSpacing.y * 2), ImGuiInputTextFlags_CallbackResize,
        [](ImGuiInputTextCallbackData* d) -> int {
            if (d->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                auto* s = static_cast<std::string*>(d->UserData);
                s->resize(static_cast<size_t>(d->BufTextLen));
                d->Buf = s->data();
            }
            return 0;
        },
        &code_);

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btn_w);
    bool run = ImGui::Button("Run");

    return run;
}
