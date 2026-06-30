#pragma once

#include <optional>
#include <utility>

#include <Python.h>

#include "src/ui/script/plugin/plugin_panel.hpp"

struct ScriptRuntime {
    ScriptRuntime() = default;
    ScriptRuntime(const ScriptRuntime&) = delete;
    ScriptRuntime& operator=(const ScriptRuntime&) = delete;
    ScriptRuntime(ScriptRuntime&& other) noexcept
        : globals(std::exchange(other.globals, nullptr)),
          tick_callback(std::exchange(other.tick_callback, nullptr)), panel(std::move(other.panel)) {
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
        if (tick_callback) {
            Py_DECREF(tick_callback);
        }
        if (globals) {
            Py_DECREF(globals);
        }
    }

    bool is_alive() const { return tick_callback != nullptr || panel.has_value(); }

    PyObject* globals = nullptr;
    PyObject* tick_callback = nullptr;
    std::optional<PluginPanel> panel;
};
