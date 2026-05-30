#pragma once
#include <cstdint>

#include <GLFW/glfw3.h>

#include "compute/fluid_sim.hpp"
#include "render/render.hpp"
#include "src/app_settings.hpp"
#include "ui/fluid_viewport.hpp"
#include "ui/imgui_manager.hpp"

class Application {
public:
    Application(uint32_t width, uint32_t height, std::string_view title) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to init GLFW");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.data(), nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        WGPUContext& ctx = WGPUContext::instance();
        ctx.init(window, width, height);

        imguiManager.init(window, ctx.device(), ctx.surfaceFormat());

        glfwSetFramebufferSizeCallback(
            window, [](GLFWwindow*, int w, int h) { WGPUContext::instance().resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h)); });

        uint32_t sim_w = width - static_cast<uint32_t>(imguiManager.panelWidth());
        renderer.init();
        simulation.init(ctx.device(), ctx.queue(), sim_w, height);
        viewport.init(ctx.device(), sim_w, height, ctx.surfaceFormat());

        prevTime = glfwGetTime();
    };

    ~Application() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run() {
        WGPUContext& ctx = WGPUContext::instance();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ctx.processEvents();

            double now = glfwGetTime();
            float dt = static_cast<float>(now - prevTime);
            prevTime = now;

            processInput();

            update(dt);

            imguiManager.beginFrame();
            imguiManager.renderUI(viewport, mouse, simulation, appSettings_);

            render();
        }
    }

private:
    void processInput() {
        if (appSettings_.brushMode == BrushMode::Inject) {
            if (mouse.leftPressed) {
                simulation.inject(static_cast<float>(mouse.x), static_cast<float>(viewport.h) - static_cast<float>(mouse.y),
                                  static_cast<float>(mouse.dx) * appSettings_.brushStrength,
                                  -static_cast<float>(mouse.dy) * appSettings_.brushStrength, appSettings_.brushRadius);
            }
        }
        else {
            if (mouse.leftPressed || mouse.rightPressed) {
                simulation.paintObstacle(static_cast<uint32_t>(mouse.x), static_cast<uint32_t>(viewport.h) - static_cast<uint32_t>(mouse.y),
                                         static_cast<uint32_t>(appSettings_.brushRadius), mouse.rightPressed);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            appSettings_.paused = !appSettings_.paused;
        }
    }

    void update(float dt) {
        if (!appSettings_.paused) {
            simulation.step(dt);
        }
    }

    void render() {
        WGPUContext& ctx = WGPUContext::instance();

        renderer.draw(viewport.view, simulation.state, appSettings_.renderSettings);

        wgpu::SurfaceTexture surfaceTex{};
        ctx.surface().getCurrentTexture(&surfaceTex);
        wgpu::raii::Texture target(surfaceTex.texture);
        wgpu::raii::TextureView targetView = target->createView();

        wgpu::RenderPassColorAttachment att{};
        att.view = *targetView;
        att.loadOp = wgpu::LoadOp::Clear;
        att.storeOp = WGPUStoreOp_Store;
        att.clearValue = {0.0, 0.0, 0.0, 1.0};
        att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &att;

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder({});
        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        imguiManager.endFrame(*pass);
        pass->end();
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);

        ctx.present();
    }

    GLFWwindow* window = nullptr;

    AppSettings appSettings_;

    FluidSim simulation;
    Render renderer;
    FluidViewport viewport;
    ImGuiManager imguiManager;
    MouseState mouse;
    double prevTime = 0.0;
};
