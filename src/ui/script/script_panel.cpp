#ifdef SCRIPTING_AVAILABLE

#include "script_panel.hpp"

#include <algorithm>
#include <fstream>

#include <imgui.h>

#include "src/scripting/scripting_engine.hpp"
#include "src/utils/file_dialog.hpp"

namespace {
    void draw_plugin_panel(PluginPanel& panel) {
        for (auto& w : panel.widgets) {
            std::visit(PluginPanel::overloaded{
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

void ScriptPanel::render(bool& open, ScriptingEngine& engine) {
    if (engine.scripts().empty()) {
        active_idx_ = engine.add_script();
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
                FileDialog::Open(filters, [this, &engine](const char* path) {
                    std::ifstream f(path);
                    std::string code((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                    editor_.set_code(code);
                    engine.scripts()[active_idx_].code = code;
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
        auto& scripts = engine.scripts();
        for (size_t i = 0; i < scripts.size(); i++) {
            bool tab_open = true;
            auto label = std::format("{}Script {}", scripts[i].tick_callback ? "• " : "", i + 1);
            if (ImGui::BeginTabItem(label.c_str(), &tab_open)) {
                if (active_idx_ != i) {
                    engine.scripts()[active_idx_].code = editor_.code();
                    active_idx_ = i;
                    editor_.set_code(scripts[i].code);
                }
                renderTab(i, engine);
                ImGui::EndTabItem();
            }
            if (!tab_open) {
                engine.remove_script(i);
                active_idx_ = std::clamp(active_idx_, 0zu, scripts.size() - 1);
                editor_.set_code(engine.scripts()[active_idx_].code);
            }
        }
        if (ImGui::TabItemButton("+")) {
            engine.scripts()[active_idx_].code = editor_.code();
            active_idx_ = engine.add_script();
            editor_.set_code("print('Hello, World!')");
        }
        ImGui::EndTabBar();
    }

    for (size_t i = 0; i < engine.scripts().size(); i++) {
        auto& s = engine.scripts()[i];
        if (s.panel) {
            auto title = std::format("Script {} Panel", i + 1);
            ImGui::Begin(title.c_str());
            engine.current_script = &s;
            draw_plugin_panel(*s.panel);
            engine.current_script = nullptr;
            ImGui::End();
        }
    }

    ImGui::End();
}

void ScriptPanel::renderTab(size_t idx, ScriptingEngine& engine) {
    Script& s = engine.scripts()[idx];

    float totalH = ImGui::GetContentRegionAvail().y;
    float buttonsH = ImGui::GetFrameHeightWithSpacing() * 2;
    float editorH = totalH * 0.6f;
    float consoleH = totalH * 0.4f - buttonsH;

    if (editor_.render(editorH)) {
        s.code = editor_.code();
        if (engine.is_available()) {
            engine.run_script(idx);
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
        engine.stop_script(idx);
    }
    ImGui::PopStyleColor();

    console_.render(consoleH, s);
}

#endif
