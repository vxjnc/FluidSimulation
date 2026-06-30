#include "script_console.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "src/ui/font_manager.hpp"

void ScriptConsole::render(float height, std::string_view output) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::BeginChild("##console", ImVec2(-1, height), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleColor();
    ImGui::PushFont(FontManager::mono());
    ImGui::TextUnformatted(output.data(), output.data() + output.size());
    ImGui::PopFont();
    ImGui::EndChild();
}
