#pragma once
#include <array>
#include <cmath>
#include <numbers>
#include <random>

namespace ColorUtils {
    constexpr inline std::byte srgbToLinear(std::byte b) {
        float x = static_cast<float>(b) / 255.f;
        float l = x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
        return static_cast<std::byte>(l * 255.f + 0.5f);
    };

    constexpr inline std::array<float, 3> hsvToRgb(float h, float s, float v) {
        if (s == 0.f) {
            return {v, v, v};
        }

        float h6 = h * 6.f;
        size_t i = static_cast<size_t>(std::floor(h6)) % 6;
        float f = h6 - std::floor(h6);

        float p = v * (1.f - s);
        float q = v * (1.f - s * f);
        float t = v * (1.f - s * (1.f - f));

        return std::array{std::array{v, t, p}, std::array{q, v, p}, std::array{p, v, t},
                          std::array{p, q, v}, std::array{t, p, v}, std::array{v, p, q}}[i];
    }

    namespace Generator {
        inline std::array<float, 3> randomVibrant(std::mt19937& mt) {
            std::uniform_real_distribution<float> dist(0.f, 1.f);
            float hue = dist(mt);
            float saturation = 0.85f;
            float value = 0.95f;
            return hsvToRgb(hue, saturation, value);
        }

        inline std::array<float, 3> randomRgb(std::mt19937& mt) {
            std::uniform_real_distribution<float> dist(0.f, 1.f);
            return {dist(mt), dist(mt), dist(mt)};
        }

        inline std::array<float, 3> randomPastel(std::mt19937& mt) {
            std::uniform_real_distribution<float> dist(0.f, 1.f);
            float hue = dist(mt);
            float saturation = 0.4f;
            float value = 0.9f;
            return hsvToRgb(hue, saturation, value);
        }

        inline std::array<float, 3> nextGoldenRatioColor(float& currentHue) {
            currentHue = std::fmod(currentHue + std::numbers::phi_v<float>, 1.0f);
            return hsvToRgb(currentHue, 1.0f, 1.0f);
        }
    };
}
