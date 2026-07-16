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

    virtual uint32_t simWidth() const = 0;
    virtual uint32_t simHeight() const = 0;

    virtual void setObstacles(std::span<const uint32_t> data) = 0;
    virtual void setVelocity(std::span<const float> data) = 0;
    virtual void setDye(std::span<const float> data) = 0;
};
