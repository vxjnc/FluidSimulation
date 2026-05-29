#include <iostream>

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "src/compute/fluid_sim.hpp"
#include "src/render/render.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/imgui_manager.hpp"
#include "src/wgpu_context.hpp"

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    constexpr int W = 1280, H = 720;
    GLFWwindow* window = glfwCreateWindow(W, H, "Fluid", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    WGPUContext& ctx = WGPUContext::instance();
    ctx.init(window, W, H);

    ImGuiManager imGuiManager(window, ctx.device(), ctx.surfaceFormat());

    glfwSetFramebufferSizeCallback(
        window, [](GLFWwindow*, int w, int h) { WGPUContext::instance().resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h)); });

    constexpr int panel_w = 280;
    FluidSim sim;
    sim.init(ctx.device(), ctx.queue(), W - panel_w, H);

    FluidViewport viewport;
    viewport.init(ctx.device(), W - panel_w, H, WGPUContext::instance().surfaceFormat());

    Render render;
    MouseState mouse{};

    double prev = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ctx.processEvents();

        if (mouse.pressed) {
            sim.inject(static_cast<float>(mouse.x), static_cast<float>(viewport.h) - static_cast<float>(mouse.y),
                       static_cast<float>(mouse.dx) * 10.0f, -static_cast<float>(mouse.dy) * 10.0f);
        }

        double now = glfwGetTime();
        float dt = static_cast<float>(now - prev);
        prev = now;
        sim.step(dt);

        imGuiManager.beginFrame();
        imGuiManager.renderUI(viewport, mouse, sim);

        render.draw(viewport.view, sim.state);

        wgpu::SurfaceTexture surfaceTex{};
        ctx.surface().getCurrentTexture(&surfaceTex);
        wgpu::raii::Texture target(surfaceTex.texture);
        wgpu::raii::TextureView targetView = target->createView();

        wgpu::RenderPassColorAttachment att{};
        att.view = *targetView;
        att.loadOp = wgpu::LoadOp::Clear;
        att.storeOp = WGPUStoreOp_Store;
        att.clearValue = {0.1, 0.1, 0.1, 1.0};
        att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &att;

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder({});
        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        imGuiManager.endFrame(*pass);
        pass->end();
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);

        ctx.present();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
