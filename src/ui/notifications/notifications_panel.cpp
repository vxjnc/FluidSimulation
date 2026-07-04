#include "notifications_panel.hpp"

#include <format>

#include <imgui.h>

#include "src/notification_manager.hpp"

void NotificationsPanel::render(NotificationManager& notifications) {
    ImVec4 color;
    ImVec2 vp = ImGui::GetMainViewport()->Size;
    float y = vp.y - 20.0f;

    for (auto& n : notifications.active_toasts()) {
        switch (n.level) {
        case NotifyLevel::Warning:
            color = ImVec4(0.9f, 0.7f, 0.1f, 1.0f);
            break;
        case NotifyLevel::Error:
            color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            break;
        default:
            color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            break;
        }

        ImGui::SetNextWindowPos(ImVec2(vp.x - 20.0f, y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::Begin(std::format("##toast{}", reinterpret_cast<uintptr_t>(&n)).c_str(), nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextColored(color, "%s", n.message.c_str());
        y -= ImGui::GetWindowHeight() + 8.0f;
        ImGui::End();
    }
}
