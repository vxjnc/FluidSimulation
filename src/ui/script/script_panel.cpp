#include "script_panel.hpp"

#include <imgui.h>

#include "src/scripting/scripting_engine.hpp"

void ScriptPanel::render(bool& open) {
    if (ScriptingEngine::instance->scripts().empty()) {
        active_idx_ = ScriptingEngine::instance->add_script();
        editor_.set_code("print('Hello, World!')");
    }

    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Script Editor", &open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("##scripts")) {
        auto& scripts = ScriptingEngine::instance->scripts();
        for (size_t i = 0; i < scripts.size(); i++) {
            bool tab_open = true;
            char label[32];
            std::snprintf(label, sizeof(label), "Script %zu", i + 1);

            if (ImGui::BeginTabItem(label, &tab_open)) {
                if (active_idx_ != i) {
                    ScriptingEngine::instance->scripts()[active_idx_].code = editor_.code();
                    active_idx_ = i;
                    editor_.set_code(scripts[i].code);
                }
                renderTab(i);
                ImGui::EndTabItem();
            }
            if (!tab_open) {
                ScriptingEngine::instance->remove_script(i);
                if (active_idx_ >= scripts.size()) {
                    active_idx_ = scripts.size() - 1;
                }
                editor_.set_code(ScriptingEngine::instance->scripts()[active_idx_].code);
            }
        }
        if (ImGui::TabItemButton("+")) {
            ScriptingEngine::instance->scripts()[active_idx_].code = editor_.code();
            active_idx_ = ScriptingEngine::instance->add_script();
            editor_.set_code("");
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void ScriptPanel::renderTab(size_t idx) {
    Script& s = ScriptingEngine::instance->scripts()[idx];

    float totalH = ImGui::GetContentRegionAvail().y;
    float buttonsH = ImGui::GetFrameHeightWithSpacing() * 2;
    float editorH = totalH * 0.6f;
    float consoleH = totalH * 0.4f - buttonsH;

    if (editor_.render(editorH)) {
        s.code = editor_.code();
#ifdef SCRIPTING_AVAILABLE
        if (ScriptingEngine::instance->is_available()) {
            ScriptingEngine::instance->run_script(idx);
        }
        else {
            s.append_output("[error] Python not available");
        }
#else
        s.append_output("[error] Scripting not compiled");
#endif
    }

    ImGui::Separator();
    if (ImGui::SmallButton("Clear")) {
        s.clear_output();
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    if (ImGui::SmallButton("Stop")) {
        ScriptingEngine::instance->stop_script(idx);
    }
    ImGui::PopStyleColor();

    ImGui::TextDisabled("console");
    console_.render(consoleH, s);
}
