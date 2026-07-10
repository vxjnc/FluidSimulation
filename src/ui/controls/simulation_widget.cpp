#include "simulation_widget.hpp"

#include <imgui.h>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/widgets/common.hpp"

void SimulationWidget::render(AppSettings& settings, FluidSim& sim, const FluidViewport& viewport) {
    ImGui::Text("Simulation settings");

    if (ImGui::SliderFloat("Sim Scale", &settings.simScale, 0.1f, 1.0f)) {
        sim.resizeWithResample(static_cast<uint32_t>(static_cast<float>(viewport.w) * settings.simScale),
                               static_cast<uint32_t>(static_cast<float>(viewport.h) * settings.simScale));
    }

    if (ImGui::SliderFloat("Dye Scale", &settings.dyeScale, settings.simScale, 1.0f)) {
        sim.resizeWithResample(sim.state.width, sim.state.height,
                               static_cast<uint32_t>(static_cast<float>(viewport.w) * settings.dyeScale),
                               static_cast<uint32_t>(static_cast<float>(viewport.h) * settings.dyeScale));
    }

    Widgets::SliderFloat("Sim dt", settings.dt, 0.001f, 0.1f);
    Widgets::SliderFloat("Velocity Dissipation", settings.velDissipation, 0.0f, 4.0f);
    Widgets::SliderFloat("Dye Dissipation", settings.dyeDissipation, 0.0f, 4.0f);
    Widgets::SliderFloat("Curl Strength", settings.curlStrength, 0.0f, 50.0f);

    if (ImGui::Button("Clear")) {
        sim.state.clear();
    }
}
