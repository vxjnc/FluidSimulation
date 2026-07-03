#pragma once

#include <functional>
#include <optional>
#include <utility>

#include <Python.h>

#include "src/ui/script/plugin/script_panel.hpp"

struct ScriptRuntime {
    ScriptRuntime() = default;
    ScriptRuntime(const ScriptRuntime&) = delete;
    ScriptRuntime& operator=(const ScriptRuntime&) = delete;
    ScriptRuntime(ScriptRuntime&& other) noexcept
        : globals(std::exchange(other.globals, nullptr)), tick_callback(std::move(other.tick_callback)),
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
        if (globals) {
            Py_DECREF(globals);
        }
    }

    bool is_alive() const { return static_cast<bool>(tick_callback) || panel.has_value(); }

    PyObject* globals = nullptr;
    std::function<void()> tick_callback;
    std::optional<ScriptPanel> panel;
};
