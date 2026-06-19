#include "render_widget.hpp"

#include <imgui.h>

#include "src/render/render_settings.hpp"

void RenderWidget::render(RenderSettings& renderSettings) {
    ImGui::Text("Render Mode");
    static const char* modes[] = {"Dye", "Velocity", "Pressure", "Divergence", "Curl"};
    int current = static_cast<int>(renderSettings.mode);
    if (ImGui::Combo("##render_mode", &current, modes, 5)) {
        renderSettings.mode = static_cast<RenderMode>(current);
    }

    ImGui::Checkbox("Show Obstacles", &renderSettings.showObstacles);
}
