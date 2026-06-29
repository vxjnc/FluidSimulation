#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <array>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

using ExportValue = std::variant<float, int, bool, std::array<float, 2>>;

struct Button {
    std::string id;
    std::string label;
    std::function<void(const std::map<std::string, ExportValue>&)> on_click;
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

using Widget = std::variant<Button, SliderF, DragInt, DragF2, Checkbox>;

class PluginPanel {
public:
    std::vector<Widget> widgets;

    std::map<std::string, ExportValue> collect_state() const;
    void draw();
};

#endif
