#include "script_console.hpp"

#include <imgui.h>

void ScriptConsole::append(std::string_view text) {
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        lines_.emplace_back(text.substr(start, end - start));
        start = end + 1;
    }
    scrollToBottom_ = true;
}

void ScriptConsole::clear() { lines_.clear(); }

void ScriptConsole::render(float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::BeginChild("##console", ImVec2(-1, height), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& line : lines_) {
        ImGui::TextUnformatted(line.c_str());
    }

    if (scrollToBottom_) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom_ = false;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
