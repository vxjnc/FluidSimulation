#pragma once
#include <imgui.h>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/ui/controls/brush_widget.hpp"
#include "src/ui/controls/render_widget.hpp"
#include "src/ui/controls/simulation_widget.hpp"
#include "src/ui/controls/source_widget.hpp"
#include "src/ui/fluid_viewport.hpp"

class ControlsPanel {
public:
    void render(bool& open, FluidViewport& viewport, FluidSim& sim, AppSettings& settings,
                std::vector<FluidSource>& sources) {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::Begin("Controls", &open);

        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("Sim size: %ux%u = %u", sim.state.width, sim.state.height,
                    sim.state.width * sim.state.height);
        ImGui::Text("Dye size: %ux%u = %u", sim.state.dye_width, sim.state.dye_height,
                    sim.state.dye_width * sim.state.dye_height);
        ImGui::Separator();

        simulationWidget.render(settings, sim, viewport);
        ImGui::Separator();

        renderWidget.render(settings.renderSettings);
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
            sources.emplace_back(0.5f, 0.5f, 0, 100, 10 * settings.simScale, std::array{1.f, 1.f, 1.f});
        }

        ImGui::End();
    }

private:
    SimulationWidget simulationWidget;
    RenderWidget renderWidget;
    BrushWidget brushWidget;
    SourceWidget sourceWidget;
};
