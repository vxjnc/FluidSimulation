#pragma once

template <typename T> struct Range {
    Range(T min, T max) : min(min), max(max) {}
    T min;
    T max;
};

struct SplatSettings {
    Range<int> countRange{40u, 50u};

    Range<float> radiusRange{50.f, 70.f};

    bool applyVelocity = true;

    Range<float> vxRange{-100.f, 100.f};
    Range<float> vyRange{-100.f, 100.f};

    bool applyColor = true;
};
