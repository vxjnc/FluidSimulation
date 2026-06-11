#pragma once
#include <imgui.h>

#include "src/compute/fluid_sim.hpp"
#include "src/ui/stats/stats_widget.hpp"

class StatsPanel {
public:
    void render(bool& open, const FluidSim& sim, const Render& render, const GpuProfiler& uiProfiler) {
        ImGui::Begin("Stats", &open);

        statsWidget.render(sim, render, uiProfiler);

        ImGui::End();
    }

private:
    StatsWidget statsWidget;
};
