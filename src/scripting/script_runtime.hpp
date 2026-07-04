#pragma once

#include <functional>
#include <optional>
#include <utility>

#include <Python.h>
#include <nanobind/nanobind.h>

#include "src/scripting/script_panel.hpp"

namespace nb = nanobind;

struct ScriptRuntime {
    ScriptRuntime() = default;
    ScriptRuntime(const ScriptRuntime&) = delete;
    ScriptRuntime& operator=(const ScriptRuntime&) = delete;
    ScriptRuntime(ScriptRuntime&& other) noexcept
        : globals(std::move(other.globals)), tick_callback(std::move(other.tick_callback)),
          panel(std::move(other.panel)) {
        other.panel.reset();
    }
    ScriptRuntime& operator=(ScriptRuntime&&) = default;

    ~ScriptRuntime() {
        if (panel) {
            for (auto& w : panel->widgets) {
                if (auto* b = std::get_if<Button>(&w)) {
                    b->on_click = nullptr;
                }
            }
        }
    }

    bool is_alive() const { return static_cast<bool>(tick_callback) || panel.has_value(); }

    nb::dict globals;
    std::function<void()> tick_callback;
    std::optional<ScriptPanel> panel;
};
