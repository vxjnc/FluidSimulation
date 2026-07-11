#pragma once
#include <vector>

#include "src/compute/fluid_source.hpp"
#include "src/notification_manager.hpp"

class ScriptHost {
public:
    virtual ~ScriptHost() = default;

    virtual std::vector<FluidSource>& getSources() = 0;
    virtual NotificationManager& notifications() = 0;

    virtual uint32_t dyeWidth() const = 0;
    virtual uint32_t dyeHeight() const = 0;

    virtual uint32_t velWidth() const = 0;
    virtual uint32_t velHeight() const = 0;
};
