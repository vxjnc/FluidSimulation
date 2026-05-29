#pragma once

#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "generated/shaders/shader.wgsl.h"
#include "src/compute/fluid_state.hpp"
#include "src/wgpu_context.hpp"

namespace {
    wgpu::ShaderModule createShaderModule(std::string_view wgsl, std::string_view label) {
        WGPUShaderSourceWGSL wgslDesc{};
        wgslDesc.chain.sType = wgpu::SType::ShaderSourceWGSL;
        wgslDesc.code = wgpu::StringView(wgsl);

        wgpu::ShaderModuleDescriptor desc{};
        desc.label = wgpu::StringView(label);
        desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
        return WGPUContext::instance().device().createShaderModule(desc);
    }
}

class Render {
public:
    Render() { initPipeline(); }

    void draw(const wgpu::raii::TextureView& targetView, FluidState& fluid) {
        WGPUContext& ctx = WGPUContext::instance();

        uint32_t dims[2] = {fluid.width, fluid.height};
        ctx.queue().writeBuffer(*dimsBuffer_, 0, dims, sizeof(dims));

        wgpu::BindGroupEntry entries[3]{};
        entries[0].binding = 0;
        entries[0].buffer = *fluid.dye;
        entries[0].size = fluid.dye->getSize();
        entries[1].binding = 1;
        entries[1].buffer = *fluid.velocity;
        entries[1].size = fluid.velocity->getSize();
        entries[2].binding = 2;
        entries[2].buffer = *dimsBuffer_;
        entries[2].size = sizeof(dims);

        wgpu::BindGroupDescriptor bgDesc{};
        bgDesc.layout = *bindGroupLayout_;
        bgDesc.entryCount = sizeof(entries) / sizeof(entries[0]);
        bgDesc.entries = entries;
        bgDesc.label = wgpu::StringView("DrawBindGroup");
        wgpu::raii::BindGroup bindGroup = ctx.device().createBindGroup(bgDesc);

        // Encode
        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder();

        wgpu::RenderPassColorAttachment color{};
        color.view = *targetView;
        color.loadOp = wgpu::LoadOp::Clear;
        color.storeOp = wgpu::StoreOp::Store;
        color.clearValue = {0, 0, 0, 1};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("DrawPass");
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &color;

        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        pass->setPipeline(*pipeline_);
        pass->setBindGroup(0, *bindGroup, 0, nullptr);
        pass->draw(4, 1, 0, 0);
        pass->end();

        wgpu::raii::CommandBuffer cmd = enc->finish();
        ctx.queue().submit(1, &*cmd);
    }

private:
    void initPipeline() {
        WGPUContext& ctx = WGPUContext::instance();
        wgpu::raii::ShaderModule shader = createShaderModule(shader_wgsl, "DrawShader");

        // Bind group layout
        wgpu::BindGroupLayoutEntry entries[3]{};
        entries[0].binding = 0;
        entries[0].visibility = wgpu::ShaderStage::Fragment;
        entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        entries[1].binding = 1;
        entries[1].visibility = wgpu::ShaderStage::Fragment;
        entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
        entries[2].binding = 2;
        entries[2].visibility = wgpu::ShaderStage::Fragment;
        entries[2].buffer.type = wgpu::BufferBindingType::Uniform;
        entries[2].buffer.minBindingSize = 8;

        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.label = wgpu::StringView("DrawBindGroupLayout");
        bglDesc.entryCount = sizeof(entries) / sizeof(entries[0]);
        bglDesc.entries = entries;
        bindGroupLayout_ = ctx.device().createBindGroupLayout(bglDesc);

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.label = wgpu::StringView("DrawPipelineLayout");
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout*>(&bindGroupLayout_);
        wgpu::raii::PipelineLayout pipelineLayout = ctx.device().createPipelineLayout(plDesc);

        // Pipeline
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
        pipeline_ = ctx.device().createRenderPipeline(pipeDesc);

        dimsBuffer_ = ctx.createBuffer(sizeof(uint32_t) * 2, wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "Dims");
    }

    wgpu::raii::BindGroupLayout bindGroupLayout_;
    wgpu::raii::RenderPipeline pipeline_;
    wgpu::raii::Buffer dimsBuffer_;
};
