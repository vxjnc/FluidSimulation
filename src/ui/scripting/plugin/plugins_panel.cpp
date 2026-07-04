#include "plugins_panel.hpp"

#include <imgui.h>
#include <imgui_markdown.h>

#include "src/scripting/plugin_manager.hpp"
#include "src/utils/file_utils.hpp"

static ImGui::MarkdownConfig mdConfig{};

void PluginsPanel::render(bool& open, PluginManager& manager, PluginSettings& settings,
                          ScriptingEngine& engine) {
    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Plugins", &open)) {
        ImGui::End();
        return;
    }

    float buttonH = ImGui::GetFrameHeightWithSpacing();
    float leftW = 200.0f;

    ImGui::BeginChild("##left", ImVec2(leftW, 0), false);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    ImGui::BeginChild("##list", ImVec2(0, -buttonH), true);
    ImGui::PopStyleColor();
    for (auto& name : manager.plugins()) {
        bool enabled = settings.enabled[name];
        if (ImGui::Checkbox(("##" + name).c_str(), &enabled)) {
            settings.enabled[name] = enabled;
        }
        ImGui::SameLine();
        if (ImGui::Selectable(name.c_str(), selected_ == name)) {
            selected_ = name;
        }
    }
    ImGui::EndChild();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y * 0.5f);
    if (ImGui::Button("Rescan", ImVec2(-1, 0))) {
        manager.scan();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##right", ImVec2(0, 0), true);
    if (!selected_.empty()) {
        if (ImGui::BeginTabBar("##tabs")) {
            if (ImGui::BeginTabItem("Readme")) {
                if (readme_cache_.find(selected_) == readme_cache_.end()) {
                    readme_cache_[selected_] = FileUtils::read_file("plugins/" + selected_ + "/readme.md");
                }
                const std::string& text = readme_cache_[selected_];
                ImGui::Markdown(text.c_str(), text.size(), mdConfig);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Console")) {
                if (auto* source = manager.get_source(selected_)) {
                    auto& output = engine.get_output(source->id);
                    ImGui::TextUnformatted(output.data(), output.data() + output.size());
                }
                else {
                    ImGui::TextDisabled("Plugin not running");
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}
