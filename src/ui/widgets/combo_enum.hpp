#pragma once
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

namespace Widgets {
    template <typename E> bool ComboEnum(const char* label, E& value) {
        static constexpr auto names = magic_enum::enum_names<E>();
        static constexpr auto items = [] {
            std::array<const char*, names.size()> arr{};
            for (size_t i = 0; i < names.size(); ++i) {
                arr[i] = names[i].data();
            }
            return arr;
        }();

        int current = static_cast<int>(value);
        if (ImGui::Combo(label, &current, items.data(), static_cast<int>(items.size()))) {
            value = static_cast<E>(current);
            return true;
        }
        return false;
    }
}
