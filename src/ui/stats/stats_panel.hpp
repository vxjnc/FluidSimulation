#pragma once

#include "src/ui/stats/stats_widget.hpp"
#include "src/utils/observable.hpp"

class FluidSim;

class StatsPanel {
public:
    void render(Observable<bool>& open, const FluidSim& sim, const Render& render,
                const GpuProfiler<>& uiProfiler);

private:
    StatsWidget statsWidget;
};
