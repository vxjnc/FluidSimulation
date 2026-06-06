#pragma once
#include <imgui.h>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/ui/fluid_viewport.hpp"

class SimulationWidget {
public:
    void render(AppSettings& settings, FluidSim& sim, const FluidViewport& viewport) {
        ImGui::Text("Simulation settings");

        float prevScale = settings.simScale;
        if (ImGui::SliderFloat("Sim Scale", &settings.simScale, 0.1f, 1.0f)) {
            float ratio = settings.simScale / prevScale;
            for (auto& s : sim.sources) {
                s.x *= ratio;
                s.y *= ratio;
                s.vx *= ratio;
                s.vy *= ratio;
                s.radius *= ratio;
            }
            sim.resizeWithResample(static_cast<uint32_t>(static_cast<float>(viewport.w) * settings.simScale),
                                   static_cast<uint32_t>(static_cast<float>(viewport.h) * settings.simScale));
        }

        if (ImGui::SliderFloat("Dye Scale", &settings.dyeScale, settings.simScale, 1.0f)) {
            sim.resizeWithResample(sim.state.width, sim.state.height,
                                   static_cast<uint32_t>(static_cast<float>(viewport.w) * settings.dyeScale),
                                   static_cast<uint32_t>(static_cast<float>(viewport.h) * settings.dyeScale));
        }

        ImGui::SliderFloat("Sim dt", &settings.dt, 0.001f, 0.1f);
        ImGui::SliderFloat("Velocity Dissipation", &settings.velDissipation, 0.0f, 4.0f);
        ImGui::SliderFloat("Dye Dissipation", &settings.dyeDissipation, 0.0f, 4.0f);
        ImGui::SliderFloat("Curl Strength", &settings.curlStrength, 0.0f, 50.0f);

        if (ImGui::Button("Clear")) {
            sim.state.clear();
        }
    }
};
