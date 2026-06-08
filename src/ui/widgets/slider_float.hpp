#pragma once

#include <imgui.h>

#include "src/utils/observable.hpp"

namespace Widgets {
    inline bool SliderFloat(const char* label, Observable<float>& val, float min, float max) {
        if (ImGui::SliderFloat(label, val.ptr(), min, max)) {
            val.onChange(static_cast<float>(val));
            return true;
        }
        return false;
    }
}