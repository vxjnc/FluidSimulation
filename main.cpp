#include <iostream>
#include <random>

#include <GLFW/glfw3.h>

#include "src/render/render.hpp"
#include "src/wgpu_context.hpp"

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    constexpr uint32_t W = 800, H = 600;
    GLFWwindow* window = glfwCreateWindow(W, H, "Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    WGPUContext& ctx = WGPUContext::instance();
    ctx.init(window, W, H);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) { WGPUContext::instance().resize(w, h); });

    Render render;
    wgpu::raii::Buffer fluid =
        ctx.createBuffer(W * H * sizeof(float) * 2, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, "Fluid");
    {
        std::mt19937 rng(42);
        std::vector<float> data(W * H * 2, 0);
        data[W * H + W] = 10;
        ctx.queue().writeBuffer(*fluid, 0, data.data(), data.size() * sizeof(float));
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ctx.processEvents();

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

        render.draw(targetView, fluid);
        ctx.present();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
