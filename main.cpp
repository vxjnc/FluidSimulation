#include <iostream>

#include <GLFW/glfw3.h>

#include "src/compute/fluid_sim.hpp"
#include "src/render/render.hpp"
#include "src/wgpu_context.hpp"

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    constexpr uint32_t W = 800, H = 600;
    GLFWwindow* window = glfwCreateWindow(W, H, "Fluid", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    WGPUContext& ctx = WGPUContext::instance();
    ctx.init(window, W, H);

    FluidSim sim;
    sim.init(W, H);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) { WGPUContext::instance().resize(w, h); });

    Render render;

    double prev = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ctx.processEvents();

        double now = glfwGetTime();
        float dt = static_cast<float>(now - prev);
        prev = now;

        sim.step(dt);

        wgpu::SurfaceTexture surfaceTex{};
        ctx.surface().getCurrentTexture(&surfaceTex);
        if (surfaceTex.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
            surfaceTex.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
            continue;
        }

        wgpu::Texture target(surfaceTex.texture);
        wgpu::TextureViewDescriptor viewDesc{};
        viewDesc.mipLevelCount = 1;
        viewDesc.arrayLayerCount = 1;
        auto targetView = target.createView(viewDesc);

        render.draw(targetView, sim.velocity);
        ctx.present();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
