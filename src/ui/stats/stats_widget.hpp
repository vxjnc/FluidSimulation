#pragma once

#include "src/compute/gpu_profiler.hpp"

class FluidSim;
class Render;

class StatsWidget {
public:
    void render(const FluidSim& sim, const Render& render, const GpuProfiler<>& uiProfiler);
};
