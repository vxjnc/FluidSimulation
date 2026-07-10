#include "render_widget.hpp"

#include <imgui.h>

#include "src/render/render_settings.hpp"
#include "src/ui/widgets/combo_enum.hpp"

void RenderWidget::render(RenderSettings& renderSettings) {
    ImGui::Text("Render Mode");
    Widgets::ComboEnum("##render_mode", renderSettings.mode);
    ImGui::Checkbox("Show Obstacles", &renderSettings.showObstacles);
}
