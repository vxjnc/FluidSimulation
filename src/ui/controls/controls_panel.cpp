#include "controls_panel.hpp"

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/ui/fluid_viewport.hpp"

void ControlsPanel::render(bool& open, FluidViewport& viewport, FluidSim& sim, AppSettings& settings,
                           std::vector<FluidSource>& sources) {
    ImGui::Begin("Controls", &open);

    simulationWidget.render(settings, sim, viewport);

    ImGui::Separator();

    renderWidget.render(settings.renderSettings);

    ImGui::Checkbox("Show Source Overlay", &settings.ui.showSourceOverlay);

    ImGui::Separator();

    ImGui::Text("Brush");
    brushWidget.render(settings);
    ImGui::Separator();

    ImGui::Text("Sources");
    for (size_t i = 0; i < sources.size();) {
        if (sourceWidget.render(sources[i], i, settings)) {
            ++i;
        }
        else {
            sources.erase(sources.begin() + i);
        }
    }
    if (ImGui::Button("Add Source")) {
        sources.emplace_back(0.5f, 0.5f, 0, 1.f, 0.05f, std::array{1.f, 1.f, 1.f});
    }

    ImGui::End();
}
