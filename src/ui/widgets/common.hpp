#pragma once
#include <imgui.h>

#include "src/utils/observable.hpp"

namespace Widgets {
    inline bool Begin(const char* name, Observable<bool>& open) {
        bool val = static_cast<bool>(open);
        bool visible = ImGui::Begin(name, &val);
        open = val;
        return visible;
    }

    inline bool MenuItem(const char* label, const char* shortcut, Observable<bool>& selected) {
        bool val = static_cast<bool>(selected);
        bool clicked = ImGui::MenuItem(label, shortcut, &val);
        selected = val;
        return clicked;
    }

    inline bool SliderFloat(const char* label, Observable<float>& val, float min, float max) {
        if (ImGui::SliderFloat(label, val.ptr(), min, max)) {
            val.onChange(static_cast<float>(val));
            return true;
        }
        return false;
    }
}
