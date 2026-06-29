#ifdef SCRIPTING_AVAILABLE

#include "plugin_panel.hpp"

#include <imgui.h>

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::map<std::string, ExportValue> PluginPanel::collect_state() const {
    std::map<std::string, ExportValue> state;
    for (const auto& w : widgets) {
        std::visit(overloaded{
                       [&](const Button&) {},
                       [&](const SliderF& s) { state[s.id] = s.val; },
                       [&](const DragInt& i) { state[i.id] = i.val; },
                       [&](const DragF2& f) { state[f.id] = f.val; },
                       [&](const Checkbox& c) { state[c.id] = c.val; },
                   },
                   w);
    }
    return state;
}

void PluginPanel::draw() {
    for (auto& w : widgets) {
        std::visit(overloaded{
                       [&](Button& b) {
                           if (ImGui::Button(b.label.c_str()) && b.on_click) {
                               b.on_click(collect_state());
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

#endif
