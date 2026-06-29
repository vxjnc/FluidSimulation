#pragma once

#include <array>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using ExportValue = std::variant<float, int, bool, std::array<float, 2>>;

class Script;

struct Button {
    std::string id;
    std::string label;
    std::function<std::optional<std::map<std::string, ExportValue>>(
        const std::map<std::string, ExportValue>&)>
        on_click;
};

struct SliderF {
    std::string id;
    std::string label;
    float val;
    float min;
    float max;
};

struct DragInt {
    std::string id;
    std::string label;
    int val;
};

struct DragF2 {
    std::string id;
    std::string label;
    std::array<float, 2> val;
};

struct Checkbox {
    std::string id;
    std::string label;
    bool val;
};

struct SameLine {};

using Widget = std::variant<SameLine, Button, SliderF, DragInt, DragF2, Checkbox>;

class PluginPanel {
public:
    std::vector<Widget> widgets;

    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    void set_value(const std::string& id, ExportValue value) {
        for (auto& w : widgets) {
            std::visit(overloaded{
                           [&](SameLine&) {},
                           [&](Button&) {},
                           [&](SliderF& s) {
                               if (s.id == id) {
                                   s.val = std::get<float>(value);
                               }
                           },
                           [&](DragInt& i) {
                               if (i.id == id) {
                                   i.val = std::get<int>(value);
                               }
                           },
                           [&](DragF2& f) {
                               if (f.id == id) {
                                   f.val = std::get<std::array<float, 2>>(value);
                               }
                           },
                           [&](Checkbox& c) {
                               if (c.id == id) {
                                   c.val = std::get<bool>(value);
                               }
                           },
                       },
                       w);
        }
    }

    std::map<std::string, ExportValue> collect_state() const {
        std::map<std::string, ExportValue> state;
        for (const auto& w : widgets) {
            std::visit(overloaded{
                           [&](const SameLine&) {},
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
};
