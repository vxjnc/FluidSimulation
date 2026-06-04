#pragma once

#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "generated/shaders/shader.wgsl.h"
#include "src/compute/fluid_state.hpp"
#include "src/render/render_settings.hpp"
#include "src/wgpu_context.hpp"

class Render {
public:
    void init() { initPipeline(); }

    void draw(const wgpu::raii::TextureView& targetView, FluidState& fluid, const RenderSettings& settings) {
        WGPUContext& ctx = WGPUContext::instance();

        uint32_t dims[4] = {fluid.width, fluid.height, static_cast<uint32_t>(settings.mode),
                            settings.showObstacles};
        ctx.queue().writeBuffer(*dimsBuffer, 0, dims, sizeof(dims));

        wgpu::BindGroupEntry entries[6]{};
        entries[0].binding = 0;
        entries[0].buffer = *fluid.dye;
        entries[0].size = fluid.dye->getSize();
        entries[1].binding = 1;
        entries[1].buffer = *fluid.velocity;
        entries[1].size = fluid.velocity->getSize();
        entries[2].binding = 2;
        entries[2].buffer = *fluid.pressure;
        entries[2].size = fluid.pressure->getSize();
        entries[3].binding = 3;
        entries[3].buffer = *fluid.divergence;
        entries[3].size = fluid.divergence->getSize();
        entries[4].binding = 4;
        entries[4].buffer = *fluid.obstacles;
        entries[4].size = fluid.obstacles->getSize();
        entries[5].binding = 5;
        entries[5].buffer = *dimsBuffer;
        entries[5].size = sizeof(dims);

        wgpu::BindGroupDescriptor bgDesc{};
        bgDesc.layout = *bindGroupLayout;
        bgDesc.entryCount = sizeof(entries) / sizeof(entries[0]);
        bgDesc.entries = entries;
        bgDesc.label = wgpu::StringView("DrawBindGroup");
        wgpu::raii::BindGroup bindGroup = ctx.device().createBindGroup(bgDesc);

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder();

        wgpu::RenderPassColorAttachment color{};
        color.view = *targetView;
        color.loadOp = wgpu::LoadOp::Clear;
        color.storeOp = wgpu::StoreOp::Store;
        color.clearValue = {0, 0, 0, 1};
        color.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("DrawPass");
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &color;

        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        pass->setPipeline(*pipeline);
        pass->setBindGroup(0, *bindGroup, 0, nullptr);
        pass->draw(4, 1, 0, 0);
        pass->end();

        wgpu::raii::CommandBuffer cmd = enc->finish();
        ctx.queue().submit(1, &*cmd);
    }

private:
    void initPipeline() {
        WGPUContext& ctx = WGPUContext::instance();
        wgpu::raii::ShaderModule shader =
            WGPUHelper::makeShaderModule(ctx.device(), shader_wgsl, "DrawShader");

        wgpu::BindGroupLayoutEntry entries[6]{};
        entries[0].binding = 0;
        entries[0].visibility = wgpu::ShaderStage::Fragment;
        entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        entries[1].binding = 1;
        entries[1].visibility = wgpu::ShaderStage::Fragment;
        entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        entries[2].binding = 2;
        entries[2].visibility = wgpu::ShaderStage::Fragment;
        entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        entries[3].binding = 3;
        entries[3].visibility = wgpu::ShaderStage::Fragment;
        entries[3].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        entries[4].binding = 4;
        entries[4].visibility = wgpu::ShaderStage::Fragment;
        entries[4].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        entries[5].binding = 5;
        entries[5].visibility = wgpu::ShaderStage::Fragment;
        entries[5].buffer.type = wgpu::BufferBindingType::Uniform;
        entries[5].buffer.minBindingSize = 16;

        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.label = wgpu::StringView("DrawBindGroupLayout");
        bglDesc.entryCount = sizeof(entries) / sizeof(entries[0]);
        bglDesc.entries = entries;
        bindGroupLayout = ctx.device().createBindGroupLayout(bglDesc);

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.label = wgpu::StringView("DrawPipelineLayout");
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout*>(&bindGroupLayout);
        wgpu::raii::PipelineLayout pipelineLayout = ctx.device().createPipelineLayout(plDesc);

        wgpu::ColorTargetState colorTarget{};
        colorTarget.format = ctx.surfaceFormat();
        colorTarget.writeMask = wgpu::ColorWriteMask::All;

        wgpu::FragmentState frag{};
        frag.module = *shader;
        frag.entryPoint = wgpu::StringView("fs_main");
        frag.targetCount = 1;
        frag.targets = &colorTarget;

        wgpu::RenderPipelineDescriptor pipeDesc{};
        pipeDesc.layout = *pipelineLayout;
        pipeDesc.vertex.module = *shader;
        pipeDesc.vertex.entryPoint = wgpu::StringView("vs_main");
        pipeDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
        pipeDesc.multisample.count = 1;
        pipeDesc.multisample.mask = 0xFFFFFFFF;
        pipeDesc.fragment = &frag;
        pipeline = ctx.device().createRenderPipeline(pipeDesc);

        dimsBuffer = WGPUHelper::makeBuffer(ctx.device(), sizeof(uint32_t) * 4,
                                            wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "Dims");
    }

    wgpu::raii::BindGroupLayout bindGroupLayout;
    wgpu::raii::RenderPipeline pipeline;
    wgpu::raii::Buffer dimsBuffer;
};
