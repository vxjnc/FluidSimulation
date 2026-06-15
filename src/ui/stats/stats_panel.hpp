#pragma once
#include <imgui.h>

#include "src/compute/fluid_sim.hpp"
#include "src/ui/stats/stats_widget.hpp"
#include "src/ui/widgets/common.hpp"
#include "src/utils/observable.hpp"

class StatsPanel {
public:
    void render(Observable<bool>& open, const FluidSim& sim, const Render& render,
                const GpuProfiler<>& uiProfiler) {
        Widgets::Begin("Stats", open);

        statsWidget.render(sim, render, uiProfiler);

        ImGui::End();
    }

private:
    StatsWidget statsWidget;
};
