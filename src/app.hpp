#pragma once
#include <cstdint>
#include <vector>

#include <GLFW/glfw3.h>
#include <nfd.hpp>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "src/app_settings.hpp"
#include "src/capture/screenshot_capture.hpp"
#include "src/color_generator.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/render/render.hpp"
#include "src/save/save_manager.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/imgui_manager.hpp"

class Application {
public:
    Application(uint32_t width, uint32_t height, std::string_view title) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to init GLFW");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.data(), nullptr,
                                  nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        WGPUContext& ctx = WGPUContext::instance();
        ctx.init(window, width, height);

        imguiManager.init(window, ctx.device(), ctx.surfaceFormat());

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
            WGPUContext::instance().resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        });

        renderer.init();
        viewport.init(ctx.device(), width, height, ctx.surfaceFormat());

        simulation.init(ctx.device(), ctx.queue(),
                        static_cast<uint32_t>(static_cast<float>(width) * settings.simScale),
                        static_cast<uint32_t>(static_cast<float>(height) * settings.simScale),
                        static_cast<uint32_t>(static_cast<float>(width) * settings.dyeScale),
                        static_cast<uint32_t>(static_cast<float>(height) * settings.dyeScale));
    };

    ~Application() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run() {
        WGPUContext& ctx = WGPUContext::instance();
        std::vector<FluidSource> frameSources;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ctx.processEvents();
            imguiManager.beginFrame();

            wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder();

            frameSources = sources;

            if (auto splats = imguiManager.splatPanel.takeSplats()) {
                frameSources.insert(frameSources.end(), splats->begin(), splats->end());
            }

            processInput(enc, frameSources);
            update(enc, frameSources);

            imguiManager.renderUI(viewport, mouse, simulation, settings, sources);
            if (imguiManager.saveSimRequested) {
                std::string cwd = std::filesystem::current_path().string();

                imguiManager.saveSimRequested = false;
                NFD::UniquePath outPath;
                nfdu8filteritem_t filters[] = {{"Fluid Simulation", "fsim"}};
                if (NFD::SaveDialog(outPath, filters, 1, cwd.c_str(), "simulation.fsim") == NFD_OKAY) {
                    SaveManager::save(outPath.get(), simulation, sources, settings);
                }
            }
            if (imguiManager.screenshotRequested) {
                imguiManager.screenshotRequested = false;
                ScreenshotCapture::request(viewport, ScreenshotCapture::Mode::Clipboard);
            }
            if (imguiManager.saveScreenshotRequested) {
                std::string cwd = std::filesystem::current_path().string();

                imguiManager.saveScreenshotRequested = false;
                NFD::UniquePath outPath;
                nfdu8filteritem_t filters[] = {{"PNG Image", "png"}};
                if (NFD::SaveDialog(outPath, filters, sizeof(filters) / sizeof(filters[0]), cwd.c_str(),
                                    "screenshot.png") == NFD_OKAY) {
                    ScreenshotCapture::request(viewport, ScreenshotCapture::Mode::File, outPath.get());
                }
            }

            render(enc);

            wgpu::raii::CommandBuffer cmd = enc->finish({});
            ctx.queue().submit(1, &*cmd);

            if (imguiManager.dyeToApply) {
                ctx.queue().writeBuffer(*simulation.getCurrentDye(), 0, imguiManager.dyeToApply->data(),
                                        imguiManager.dyeToApply->size() * sizeof(float));
                imguiManager.dyeToApply.reset();
            }
            if (imguiManager.loadSimRequested) {
                std::string cwd = std::filesystem::current_path().string();

                imguiManager.loadSimRequested = false;
                NFD::UniquePath outPath;
                nfdu8filteritem_t filters[] = {{"Fluid Simulation", "fsim"}};
                if (NFD::OpenDialog(outPath, filters, 1, cwd.c_str()) == NFD_OKAY) {
                    SaveManager::load(outPath.get(), simulation, sources, settings);
                }
            }

            ctx.present();
        }
    }

private:
    void processInput(wgpu::raii::CommandEncoder& enc, std::vector<FluidSource>& frameSources) {
        float sx = mouse.x * settings.simScale;
        float sy = (static_cast<float>(viewport.h) - mouse.y) * settings.simScale;
        float sdx = mouse.dx * settings.simScale;
        float sdy = mouse.dy * settings.simScale;
        float sr = settings.brushRadius * settings.simScale;

        if (settings.brushMode == BrushMode::Inject) {
            if (mouse.leftPressed) {
                frameSources
                    .emplace_back(sx, sy, sdx * settings.brushStrength, -sdy * settings.brushStrength, sr,
                                  settings.brushColor)
                    .mode_mask = settings.brushModeMask;
            }
        }
        else if (settings.brushMode == BrushMode::PaintWall) {
            if (mouse.leftPressed || mouse.rightPressed) {
                simulation.paintObstacle(enc, static_cast<uint32_t>(sx), static_cast<uint32_t>(sy),
                                         static_cast<uint32_t>(sr), mouse.rightPressed);
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
        if (ImGui::IsKeyPressed(ImGuiKey_C)) {
            settings.renderSettings.mode = RenderMode::Curl;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftAlt) || ImGui::IsKeyPressed(ImGuiKey_RightAlt)) {
            imguiManager.menuBarVisible = !imguiManager.menuBarVisible;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F12)) {
            ScreenshotCapture::request(viewport, ScreenshotCapture::Mode::Clipboard);
        }

        if (settings.paused && ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            settings.paused = false;
            update(enc, frameSources);
            settings.paused = true;
        }

        if (mouse.leftJustPressed) {
            static float hue = static_cast<float>(time(nullptr));
            settings.brushColor = ColorUtils::Generator::nextGoldenRatioColor(hue);
        }
    }

    void update(wgpu::raii::CommandEncoder& enc, std::span<const FluidSource> frameSources) {
        static float lastDt = 0.0f;
        static float lastDyeDissipation = 0.0f;
        static float lastVelDissipation = 0.0f;
        static float lastCurlStrength = 0.0f;

        if (settings.dt != lastDt) {
            simulation.setDt(settings.dt);
            lastDt = settings.dt;
        }
        if (settings.dyeDissipation != lastDyeDissipation) {
            simulation.setDyeDissipation(settings.dyeDissipation);
            lastDyeDissipation = settings.dyeDissipation;
        }
        if (settings.velDissipation != lastVelDissipation) {
            simulation.setVelDissipation(settings.velDissipation);
            lastVelDissipation = settings.velDissipation;
        }
        if (settings.curlStrength != lastCurlStrength) {
            simulation.setCurlStrength(settings.curlStrength);
            lastCurlStrength = settings.curlStrength;
        }

        if (!frameSources.empty()) {
            simulation.inject(enc, frameSources);
        }
        if (!settings.paused) {
            simulation.step(enc);
        }
    }

    void render(wgpu::raii::CommandEncoder& enc) {
        WGPUContext& ctx = WGPUContext::instance();

        renderer.draw(enc, viewport.view, simulation, settings.renderSettings);

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

        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        imguiManager.endFrame(*pass);
        pass->end();
    }

    GLFWwindow* window = nullptr;
    NFD::Guard nfdGuard;

    AppSettings settings;

    FluidSim simulation;
    std::vector<FluidSource> sources;

    Render renderer;
    FluidViewport viewport;
    ImGuiManager imguiManager;
    MouseState mouse;
};
