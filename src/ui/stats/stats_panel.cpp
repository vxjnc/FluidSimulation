#include "stats_panel.hpp"

#include <imgui.h>

#include "src/compute/fluid_sim.hpp"
#include "src/ui/widgets/common.hpp"

void StatsPanel::render(Observable<bool>& open, const FluidSim& sim, const Render& render,
                        const GpuProfiler<>& uiProfiler) {
    Widgets::Begin("Stats", open);

    statsWidget.render(sim, render, uiProfiler);

    ImGui::End();
}
