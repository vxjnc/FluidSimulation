#include "notifications_area.hpp"

#include <format>
#include <ranges>

#include <imgui.h>

#include "src/notification_manager.hpp"

void NotificationsArea::render(NotificationManager& notifications) {
    ImVec4 color;
    ImVec2 vp = ImGui::GetMainViewport()->Size;
    float y = vp.y - 20.0f;

    for (auto [i, n] : notifications.active_toasts() | std::views::reverse | std::views::enumerate) {
        switch (n.level) {
        case NotifyLevel::Warning:
            color = ImVec4(0.9f, 0.7f, 0.1f, 1.0f);
            break;
        case NotifyLevel::Error:
            color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            break;
        case NotifyLevel::Info:
            color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
        }

        ImGui::SetNextWindowPos(ImVec2(vp.x - 20.0f, y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::Begin(std::format("##toast{}", i).c_str(), nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings);

        float radius = ImGui::GetTextLineHeight() * 0.3f;
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImVec2 center(cursor.x + radius, cursor.y + ImGui::GetTextLineHeight() * 0.5f);
        ImGui::GetWindowDrawList()->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(color));

        ImGui::Dummy(ImVec2(radius * 2.0f + 6.0f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextColored(color, "%s", n.message.c_str());

        y -= ImGui::GetWindowHeight() + 8.0f;
        ImGui::End();
    }
}
