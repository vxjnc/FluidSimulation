#include <GLFW/glfw3.h>
#include <benchmark/benchmark.h>
#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_sim.hpp"

class FluidSimBenchmark : public benchmark::Fixture {
public:
    GLFWwindow* window = nullptr;
    FluidSim simulation;

    void SetUp(const ::benchmark::State& state) override {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to init GLFW for benchmark");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        window = glfwCreateWindow(1280, 720, "Headless Benchmark", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create hidden window for benchmark");
        }

        WGPUContext& ctx = WGPUContext::instance();
        ctx.init(window, 1280, 720);

        uint32_t simWidth = static_cast<uint32_t>(state.range(0));
        uint32_t simHeight = static_cast<uint32_t>(state.range(0));

        simulation.init(ctx.device(), ctx.queue(), simWidth, simHeight, simWidth, simHeight);

        simulation.setDt(0.01f);
        simulation.setDyeDissipation(0.1f);
        simulation.setVelDissipation(0.1f);
        simulation.setCurlStrength(30.0f);

        for (int i = 0; i < 10; ++i) {
            wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder();
            simulation.step(enc);
            simulation.profiler.resolve(enc);

            wgpu::raii::CommandBuffer cmd = enc->finish({});
            ctx.queue().submit(1, &*cmd);

            simulation.profiler.readSync(ctx.device());
        }
    }

    void TearDown(const ::benchmark::State&) override {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

BENCHMARK_DEFINE_F(FluidSimBenchmark, ComputeStep)(benchmark::State& state) {
    WGPUContext& ctx = WGPUContext::instance();
    auto& profiler = simulation.profiler;

    if (!profiler.isSupported()) {
        state.SkipWithError("Timestamp queries not supported on this device");
        return;
    }

    for (auto _ : state) {
        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder();

        simulation.step(enc);
        profiler.resolve(enc);

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);

        size_t ns = profiler.readSync(ctx.device());
        state.SetIterationTime(static_cast<double>(ns) / 1e9);
    }

    state.counters["GridSize"] = static_cast<double>(state.range(0));
}

BENCHMARK_REGISTER_F(FluidSimBenchmark, ComputeStep)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024);

BENCHMARK_MAIN();
