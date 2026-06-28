#pragma once

#include <cstddef>

struct FluidSource;
struct AppSettings;

class SourceWidget {
public:
    bool render(FluidSource& s, size_t idx, AppSettings& settings);
};
