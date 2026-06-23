#include "script_console.hpp"

#include <imgui.h>

void ScriptConsole::append(std::string_view text) {
    text_ += text;
    if (text_.size() > 10 * 1024) {
        text_.erase(0, text_.size() - 10 * 1024);
    }
    scrollToBottom_ = true;
}

void ScriptConsole::clear() { text_.clear(); }

void ScriptConsole::render(float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::BeginChild("##console", ImVec2(-1, height), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::InputTextMultiline("##console", text_.data(), text_.size(), ImVec2(-1, height),
                              ImGuiInputTextFlags_ReadOnly);

    if (scrollToBottom_) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom_ = false;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
