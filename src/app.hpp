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
        viewport.init(ctx.device(), sim_w, height, ctx.surfaceFormat());

        simulation.init(ctx.device(), ctx.queue(), sim_w * settings.simScale, height * settings.simScale);
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

            processInput();

            update(settings.dt);

            imguiManager.beginFrame();
            imguiManager.renderUI(viewport, mouse, simulation, settings);

            render();
        }
    }

private:
    void processInput() {
        float sx = mouse.x * settings.simScale;
        float sy = (viewport.h - mouse.y) * settings.simScale;
        float sdx = mouse.dx * settings.simScale;
        float sdy = mouse.dy * settings.simScale;
        float sr = settings.brushRadius * settings.simScale;

        if (settings.brushMode == BrushMode::Inject) {
            if (mouse.leftPressed) {
                simulation.inject(sx, sy, sdx * settings.brushStrength, -sdy * settings.brushStrength, sr);
            }
        }
        else {
            if (mouse.leftPressed || mouse.rightPressed) {
                simulation.paintObstacle(sx, sy, sr, mouse.rightPressed);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            settings.paused = !settings.paused;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_D)) {
            settings.renderSettings.mode = RenderMode::Dye;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_V)) {
            settings.renderSettings.mode = RenderMode::Velocity;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_P)) {
            settings.renderSettings.mode = RenderMode::Pressure;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_G)) {
            settings.renderSettings.mode = RenderMode::Divergence;
        }
    }

    void update(float dt) {
        if (!settings.paused) {
            simulation.step(dt);
        }
    }

    void render() {
        WGPUContext& ctx = WGPUContext::instance();

        renderer.draw(viewport.view, simulation.state, settings.renderSettings);

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

        wgpu::CommandEncoderDescriptor cmdDesc{};
        cmdDesc.label = wgpu::StringView("CommandEncoder");

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder(cmdDesc);
        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        imguiManager.endFrame(*pass);
        pass->end();
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);

        ctx.present();
    }

    GLFWwindow* window = nullptr;

    AppSettings settings;

    FluidSim simulation;
    Render renderer;
    FluidViewport viewport;
    ImGuiManager imguiManager;
    MouseState mouse;
};
