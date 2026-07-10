#pragma once
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <optional>

#include <webgpu/webgpu-raii.hpp>

#include "src/capture/gpu_readback.hpp"
#include "src/compute/wgpu_helper.hpp"

template <size_t MAX_SAMPLES = 60> class GpuProfiler {
public:
    struct Stats {
        double avgNs;
    };

    void init(wgpu::Device device) {
        supported_ = device.hasFeature(wgpu::FeatureName::TimestampQuery);
        if (!supported_) {
            return;
        }

        wgpu::QuerySetDescriptor qsDesc{};
        qsDesc.type = wgpu::QueryType::Timestamp;
        qsDesc.count = 2;
        qsDesc.label = wgpu::StringView("ProfilerQuerySet");
        querySet_ = device.createQuerySet(qsDesc);

        resolveBuffer_ = WGPUHelper::makeBuffer(device, 2 * sizeof(size_t),
                                                wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc,
                                                "ProfilerResolveBuffer");
    }

    bool isSupported() const { return supported_; }

    template <typename PassEncoder> void writeBegin(PassEncoder& pass) {
        if (!supported_ || !enabled || state_->pending) {
            return;
        }
        pass->writeTimestamp(*querySet_, 0);
    }

    template <typename PassEncoder> void writeEnd(PassEncoder& pass) {
        if (!supported_ || !enabled || state_->pending) {
            return;
        }
        pass->writeTimestamp(*querySet_, 1);
        wantsResolve_ = true;
    }

    void resolve(wgpu::raii::CommandEncoder& enc) {
        if (!supported_ || !enabled || !wantsResolve_) {
            return;
        }
        enc->resolveQuerySet(*querySet_, 0, 2, *resolveBuffer_, 0);
        wantsResolve_ = false;
        state_->pending = true;
    }

    void recordReadback(wgpu::CommandEncoder& enc, wgpu::Device device) {
        if (!supported_ || !enabled || !state_->pending) {
            return;
        }

        auto state = state_;
        auto* rbState = GpuReadback::record<size_t>( //
            enc, device, *resolveBuffer_, 2 * sizeof(size_t), [state](std::span<const size_t> timestamps) {
                size_t delta = timestamps[1] - timestamps[0];

                state->sum += delta;
                state->samples.emplace_back(delta);
                if (state->samples.size() > MAX_SAMPLES) {
                    state->sum -= state->samples.front();
                    state->samples.pop_front();
                }
                state->pending = false;
            });

        pendingFinish_ = [rbState]() { GpuReadback::startMap<size_t>(rbState); };
    }

    void finishReadback() {
        if (pendingFinish_) {
            pendingFinish_();
            pendingFinish_ = nullptr;
        }
    }

    std::optional<Stats> getStats() const {
        if (!supported_ || state_->samples.empty()) {
            return std::nullopt;
        }

        // TODO: use timestampPeriod scaling
        return Stats{static_cast<double>(state_->sum) / static_cast<double>(state_->samples.size())};
    }

    size_t readSync(wgpu::Device device) {
        if (!supported_ || !enabled || !state_->pending) {
            return 0;
        }

        size_t result = 0;
        bool done = false;

        GpuReadback::request<size_t>(device, *resolveBuffer_, 2 * sizeof(size_t),
                                     [&](std::span<const size_t> timestamps) {
                                         result = timestamps[1] - timestamps[0];
                                         done = true;
                                     });

        while (!done) {
#ifdef WEBGPU_BACKEND_DAWN
            device.tick();
#endif
#ifdef WEBGPU_BACKEND_WGPU
            device.poll(false, nullptr);
#endif
        }

        state_->pending = false;
        return result;
    }

    bool enabled = true;

private:
    struct SharedState {
        size_t sum = 0;
        bool pending = false;
        std::deque<size_t> samples;
    };

    bool supported_ = false;
    bool wantsResolve_ = false;

    std::shared_ptr<SharedState> state_ = std::make_shared<SharedState>();
    std::function<void()> pendingFinish_;

    wgpu::raii::QuerySet querySet_;
    wgpu::raii::Buffer resolveBuffer_;
};
