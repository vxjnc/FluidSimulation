#pragma once

#include <cstddef>

class FluidSource;
class AppSettings;

class SourceWidget {
public:
    bool render(FluidSource& s, size_t idx, AppSettings& settings);
};
