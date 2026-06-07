#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include <zpp_bits.h>

#include "src/compute/fluid_source.hpp"

struct FsimFluidSource {
    static constexpr uint32_t kVersion = 1;
    uint32_t version = kVersion;

    std::array<float, 3> color{1.f, 1.f, 1.f};
    float x = 0.f, y = 0.f;
    float vx = 0.f, vy = 0.f;
    float radius = 4.f;
    bool active = true;
    int mode_mask = 0;
    uint8_t form = 0;

    constexpr static zpp::bits::errc serialize(auto& archive, auto& self) {
        if (auto err = archive(self.version); zpp::bits::failure(err.code)) {
            return err;
        }

        // v1
        if (auto err = archive(self.color, self.x, self.y, self.vx, self.vy, self.radius, self.active,
                               self.mode_mask, self.form);
            zpp::bits::failure(err.code)) {
            return err;
        }

        // v2: if (self.version >= 2) { self.alpha ... }

        return zpp::bits::errc{};
    }

    static FsimFluidSource fromFluidSource(const FluidSource& s) {
        FsimFluidSource r;
        r.color = s.color;
        r.x = s.x;
        r.y = s.y;
        r.vx = s.vx;
        r.vy = s.vy;
        r.radius = s.radius;
        r.active = s.active;
        r.mode_mask = s.mode_mask;
        r.form = static_cast<uint8_t>(s.form);
        return r;
    }

    FluidSource toFluidSource() const {
        FluidSource s;
        s.color = color;
        s.x = x;
        s.y = y;
        s.vx = vx;
        s.vy = vy;
        s.radius = radius;
        s.active = active;
        s.mode_mask = mode_mask;
        s.form = static_cast<FluidSource::Form>(form);
        return s;
    }
};

struct FsimFile {
    static constexpr uint32_t kVersion = 1;

    uint32_t version = kVersion;

    uint32_t width = 0, height = 0;
    uint32_t dyeWidth = 0, dyeHeight = 0;

    float dt = 0.01f;
    float velDissipation = 0.2f;
    float dyeDissipation = 0.25f;
    float curlStrength = 30.f;
    float simScale = 0.8f;
    float dyeScale = 1.0f;

    std::vector<FsimFluidSource> sources;

    std::vector<float> velocity;     // width*height*2
    std::vector<float> dye;          // dyeWidth*dyeHeight*4
    std::vector<uint32_t> obstacles; // width*height

    constexpr static zpp::bits::errc serialize(auto& archive, auto& self) {
        if (auto err = archive(self.version); zpp::bits::failure(err.code)) {
            return err;
        }

        // v1
        if (auto err = archive(self.width, self.height, self.dyeWidth, self.dyeHeight, self.dt,
                               self.velDissipation, self.dyeDissipation, self.curlStrength, self.simScale,
                               self.dyeScale, self.sources, self.velocity, self.dye, self.obstacles);
            zpp::bits::failure(err.code)) {
            return err;
        }

        // v2: if (self.version >= 2) { ... }

        return zpp::bits::errc{};
    }
};
