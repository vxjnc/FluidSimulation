#pragma once
#include <cstring>
#include <deque>
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
        if (!supported_ || pending_) {
            return;
        }
        pass->writeTimestamp(*querySet_, 0);
    }

    template <typename PassEncoder> void writeEnd(PassEncoder& pass) {
        if (!supported_ || pending_) {
            return;
        }
        pass->writeTimestamp(*querySet_, 1);
        wantsResolve_ = true;
    }

    void resolve(wgpu::raii::CommandEncoder& enc) {
        if (!supported_ || !wantsResolve_) {
            return;
        }
        enc->resolveQuerySet(*querySet_, 0, 2, *resolveBuffer_, 0);
        wantsResolve_ = false;
        pending_ = true;
    }

    void requestReadback() {
        if (!supported_ || !pending_) {
            return;
        }

        GpuReadback::request(*resolveBuffer_, 2 * sizeof(size_t), [this](std::span<const std::byte> data) {
            const size_t* timestamps = reinterpret_cast<const size_t*>(data.data());
            size_t delta = timestamps[1] - timestamps[0];

            sum_ += delta;
            samples_.emplace_back(delta);
            if (samples_.size() > MAX_SAMPLES) {
                sum_ -= samples_.front();
                samples_.pop_front();
            }
            pending_ = false;
        });
    }

    std::optional<Stats> getStats() const {
        if (!supported_ || samples_.empty()) {
            return std::nullopt;
        }

        // TODO: use timestampPeriod scaling
        return Stats{static_cast<double>(sum_) / static_cast<double>(samples_.size())};
    }

    size_t readSync(wgpu::Device device) {
        if (!supported_ || !pending_) {
            return 0;
        }

        size_t result = 0;
        bool done = false;

        GpuReadback::request(*resolveBuffer_, 2 * sizeof(size_t), [&](std::span<const std::byte> data) {
            auto* timestamps = reinterpret_cast<const size_t*>(data.data());
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

        pending_ = false;
        return result;
    }

private:
    size_t sum_ = 0;

    bool supported_ = false;
    bool wantsResolve_ = false;
    bool pending_ = false;

    wgpu::raii::QuerySet querySet_;
    wgpu::raii::Buffer resolveBuffer_;
    std::deque<size_t> samples_;
};
