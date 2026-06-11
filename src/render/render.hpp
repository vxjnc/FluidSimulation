#pragma once

#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu.hpp>

#include "generated/shaders/shader.wgsl.h"
#include "src/compute/fluid_sim.hpp"
#include "src/compute/gpu_profiler.hpp"
#include "src/compute/wgpu_helper.hpp"
#include "src/render/render_settings.hpp"
#include "src/wgpu_context.hpp"

class Render {
    struct RenderParams {
        uint32_t width, height;
        uint32_t dye_width, dye_height;
        uint32_t mode, show_obstacles;
    };

public:
    void init(wgpu::Device device) {
        initPipeline();

        profiler.init(device);
    }

    void draw(const wgpu::raii::TextureView& targetView, FluidSim& fluid_sim,
              const RenderSettings& settings) {
        WGPUContext& ctx = WGPUContext::instance();

        wgpu::CommandEncoderDescriptor cmdDesc{};
        cmdDesc.label = wgpu::StringView("DrawEncoder");
        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder(cmdDesc);

        draw(enc, targetView, fluid_sim, settings);

        wgpu::raii::CommandBuffer cmd = enc->finish();
        ctx.queue().submit(1, &*cmd);
    }

    void draw(wgpu::raii::CommandEncoder& enc, const wgpu::raii::TextureView& targetView, FluidSim& fluid_sim,
              const RenderSettings& settings) {
        WGPUContext& ctx = WGPUContext::instance();

        FluidState& fluid = fluid_sim.state;

        RenderParams p{
            fluid.width,
            fluid.height,
            fluid.dye_width,
            fluid.dye_height,
            static_cast<uint32_t>(settings.mode),
            settings.showObstacles,
        };
        ctx.queue().writeBuffer(*paramsBuffer, 0, &p, sizeof(p));

        wgpu::raii::BindGroup bindGroup = WGPUHelper::makeBindGroup(ctx.device(), pipeline,
                                                                    {
                                                                        *paramsBuffer,
                                                                        *fluid_sim.getCurrentDye(),
                                                                        *fluid.velocity,
                                                                        *fluid.pressure,
                                                                        *fluid.divergence,
                                                                        *fluid.obstacles,
                                                                        *fluid.curl,
                                                                    },
                                                                    "DrawBindGroup");

        wgpu::RenderPassColorAttachment color{};
        color.view = *targetView;
        color.loadOp = wgpu::LoadOp::Load;
        color.storeOp = wgpu::StoreOp::Store;
        color.clearValue = {0, 0, 0, 1};
        color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("DrawPass");
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &color;

        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        profiler.writeBegin(pass);

        pass->setPipeline(*pipeline);
        pass->setBindGroup(0, *bindGroup, 0, nullptr);
        pass->draw(4, 1, 0, 0);

        profiler.writeEnd(pass);
        pass->end();
        profiler.resolve(enc);
    }

    GpuProfiler<> profiler;

private:
    void initPipeline() {
        WGPUContext& ctx = WGPUContext::instance();
        wgpu::raii::ShaderModule shader =
            WGPUHelper::makeShaderModule(ctx.device(), shader_wgsl, "DrawShader");

        wgpu::ColorTargetState colorTarget{};
        colorTarget.format = ctx.surfaceFormat();
        colorTarget.writeMask = wgpu::ColorWriteMask::All;

        wgpu::FragmentState frag{};
        frag.module = *shader;
        frag.entryPoint = wgpu::StringView("fs_main");
        frag.targetCount = 1;
        frag.targets = &colorTarget;

        wgpu::RenderPipelineDescriptor pipeDesc{};
        pipeDesc.layout = nullptr;
        pipeDesc.vertex.module = *shader;
        pipeDesc.vertex.entryPoint = wgpu::StringView("vs_main");
        pipeDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
        pipeDesc.multisample.count = 1;
        pipeDesc.multisample.mask = 0xFFFFFFFF;
        pipeDesc.fragment = &frag;
        pipeline = ctx.device().createRenderPipeline(pipeDesc);

        paramsBuffer =
            WGPUHelper::makeBuffer(ctx.device(), sizeof(RenderParams),
                                   wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "Dims");
    }

    wgpu::raii::BindGroupLayout bindGroupLayout;
    wgpu::raii::RenderPipeline pipeline;
    wgpu::raii::Buffer paramsBuffer;
};
