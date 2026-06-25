#ifdef SCRIPTING_AVAILABLE

#include "script_panel.hpp"

#include <algorithm>
#include <fstream>

#include <imgui.h>

#include "src/scripting/scripting_engine.hpp"
#include "src/utils/file_dialog.hpp"

void ScriptPanel::render(bool& open) {
    if (ScriptingEngine::instance->scripts().empty()) {
        active_idx_ = ScriptingEngine::instance->add_script();
        editor_.set_code("print('Hello, World!')");
    }

    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Script Editor", &open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open...")) {
                nfdu8filteritem_t filters[] = {{"Python Script", "py"}};
                FileDialog::Open(filters, [this](const char* path) {
                    std::ifstream f(path);
                    std::string code((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    editor_.set_code(code);
                    ScriptingEngine::instance->scripts()[active_idx_].code = code;
                });
            }
            if (ImGui::MenuItem("Save...")) {
                nfdu8filteritem_t filters[] = {{"Python Script", "py"}};
                FileDialog::Save("script.py", filters, [this](const char* path) {
                    std::ofstream f(path);
                    f << editor_.code();
                });
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::BeginTabBar("##scripts")) {
        auto& scripts = ScriptingEngine::instance->scripts();
        for (size_t i = 0; i < scripts.size(); i++) {
            bool tab_open = true;
            auto label = std::format("{}Script {}", scripts[i].tick_callback ? "• " : "", i + 1);
            if (ImGui::BeginTabItem(label.c_str(), &tab_open)) {
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
                active_idx_ = std::clamp(active_idx_, 0zu, scripts.size() - 1);
                editor_.set_code(ScriptingEngine::instance->scripts()[active_idx_].code);
            }
        }
        if (ImGui::TabItemButton("+")) {
            ScriptingEngine::instance->scripts()[active_idx_].code = editor_.code();
            active_idx_ = ScriptingEngine::instance->add_script();
            editor_.set_code("print('Hello, World!')");
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
        if (ScriptingEngine::instance->is_available()) {
            ScriptingEngine::instance->run_script(idx);
        }
        else {
            s.append_output("[error] Python not available");
        }
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

    console_.render(consoleH, s);
}

#endif
