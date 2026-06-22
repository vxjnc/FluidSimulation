#include "script_editor.hpp"

#include <imgui.h>

bool ScriptEditor::render(float height) {
    ImGui::InputTextMultiline(
        "##code", code_.data(), code_.capacity(), ImVec2(-1, height), ImGuiInputTextFlags_CallbackResize,
        [](ImGuiInputTextCallbackData* d) -> int {
            if (d->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                auto* s = static_cast<std::string*>(d->UserData);
                s->resize(static_cast<size_t>(d->BufTextLen));
                d->Buf = s->data();
            }
            return 0;
        },
        &code_);

    return ImGui::Button("Run");
}
