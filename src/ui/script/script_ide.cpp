
#include "script_ide.hpp"

#include <fstream>

#include <imgui.h>

#include "src/scripting/scripting_engine.hpp"
#include "src/utils/file_dialog.hpp"
#include "src/utils/file_utils.hpp"

namespace {
    void draw_script_panel(ScriptPanel& panel) {
        for (auto& w : panel.widgets) {
            std::visit(ScriptPanel::overloaded{
                           [&](SameLine&) { ImGui::SameLine(); },
                           [&](Button& b) {
                               if (ImGui::Button(b.label.c_str()) && b.on_click) {
                                   if (auto result = b.on_click(panel.collect_state())) {
                                       for (auto& [k, v] : *result) {
                                           panel.set_value(k, v);
                                       }
                                   }
                               }
                           },
                           [&](SliderF& s) { ImGui::SliderFloat(s.label.c_str(), &s.val, s.min, s.max); },
                           [&](DragInt& i) { ImGui::DragInt(i.label.c_str(), &i.val); },
                           [&](DragF2& f) { ImGui::DragFloat2(f.label.c_str(), f.val.data()); },
                           [&](Checkbox& c) { ImGui::Checkbox(c.label.c_str(), &c.val); },
                       },
                       w);
        }
    }
}

void ScriptIDE::render(bool& open, ScriptingEngine& engine, std::vector<ScriptSource>& scripts) {
    if (!engine.is_available()) {
        ImGui::Begin("Script Editor", &open);
        ImGui::TextDisabled("Python not available");
        ImGui::End();
        return;
    }

    engine.for_each_panel([&engine](size_t id, ScriptPanel& panel) {
        engine.set_current_context(id);
        auto title = std::format("Script {} Panel", id);
        ImGui::Begin(title.c_str());
        draw_script_panel(panel);
        ImGui::End();
    });
    engine.set_current_context(ScriptSource::INVALID_ID);

    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Script Editor", &open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open...")) {
                nfdu8filteritem_t filters[] = {{"Python Script", "py"}};
                FileDialog::Open(filters, [this, &scripts](const char* path) {
                    std::string code = FileUtils::read_file(path);
                    addScript(std::move(code));
                    active_idx_ = scripts.size() - 1;
                    editor_.set_code(scripts[active_idx_].code);
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
        for (size_t i = 0; i < scripts.size(); i++) {
            bool tab_open = true;
            auto& s = scripts[i];
            auto label = std::format("Script {}", i + 1);
            if (ImGui::BeginTabItem(label.c_str(), &tab_open)) {
                if (active_idx_ != i) {
                    scripts[active_idx_].code = editor_.code();
                    active_idx_ = i;
                    editor_.set_code(s.code);
                }
                renderTab(s, engine);
                ImGui::EndTabItem();
            }
            if (!tab_open) {
                removeScript(s.id);
            }
        }
        if (ImGui::TabItemButton("+")) {
            if (!scripts.empty()) {
                scripts[active_idx_].code = editor_.code();
            }
            addScript("print('Hello, World!')");
            active_idx_ = scripts.size() - 1;
            editor_.set_code(scripts[active_idx_].code);
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void ScriptIDE::renderTab(ScriptSource& s, ScriptingEngine& engine) {
    float totalH = ImGui::GetContentRegionAvail().y;
    float buttonsH = ImGui::GetFrameHeightWithSpacing() * 2;
    float editorH = totalH * 0.6f;
    float consoleH = totalH * 0.4f - buttonsH;

    if (editor_.render(editorH)) {
        s.code = editor_.code();
        engine.run_script(s);
    }

    ImGui::Separator();
    if (ImGui::SmallButton("Clear")) {
        engine.clear_output(s.id);
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    if (ImGui::SmallButton("Stop")) {
        engine.stop_script(s.id);
    }
    ImGui::PopStyleColor();

    console_.render(consoleH, engine.get_output(s.id));
}
