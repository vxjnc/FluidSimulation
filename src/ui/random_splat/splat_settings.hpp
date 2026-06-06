#pragma once

struct SplatSettings {
    int countMin = 40;
    int countMax = 50;

    float radiusMin = 50.f;
    float radiusMax = 70.f;

    bool applyVelocity = true;

    // XY mode
    float vxMin = -100.f, vxMax = 100.f;
    float vyMin = -100.f, vyMax = 100.f;

    // Polar mode
    float angleMin = 0.f, angleMax = 360.f;
    float magMin = 0.f, magMax = 100.f;

    bool applyColor = true;
};
