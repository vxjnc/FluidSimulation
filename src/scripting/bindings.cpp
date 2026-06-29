#ifdef SCRIPTING_AVAILABLE

#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "src/compute/fluid_source.hpp"
#include "src/scripting/scripting_engine.hpp"

namespace nb = nanobind;

NB_MODULE(_fluidsim_io, m) {
    m.def("output", [](const char* text) {
        if (text && ScriptingEngine::instance->current_script) {
            ScriptingEngine::instance->current_script->append_output(text);
        }
    });
}

NB_MODULE(fluidsim, m) {
    nb::class_<FluidSource>(m, "FluidSource")
        .def_rw("color", &FluidSource::color)
        .def_rw("x", &FluidSource::x)
        .def_rw("y", &FluidSource::y)
        .def_rw("vx", &FluidSource::vx)
        .def_rw("vy", &FluidSource::vy)
        .def_rw("radius", &FluidSource::radius)
        .def_rw("active", &FluidSource::active)
        .def_rw("mask", &FluidSource::mode_mask);

    nb::enum_<FluidSource::Mode>(m, "Mode", nb::is_arithmetic())
        .value("VELOCITY", FluidSource::Mode::VELOCITY)
        .value("DYE_ADDITIVE", FluidSource::Mode::DYE_ADDITIVE)
        .value("DYE_REPLACE", FluidSource::Mode::DYE_REPLACE);

    m.def("on_tick", [](nb::callable cb) { ScriptingEngine::instance->set_tick_callback(cb.ptr()); });

    m.def("add_source", [](float x, float y, float vx, float vy, float radius, std::array<float, 3> color) {
        auto& sources = *ScriptingEngine::instance->sources;
        sources.push_back(FluidSource(x, y, vx, vy, radius, color));
        return sources.size() - 1;
    });

    m.def("remove_source", [](int idx) {
        auto& sources = *ScriptingEngine::instance->sources;
        if (idx < 0 || idx >= static_cast<int>(sources.size())) {
            throw nb::index_error("index out of range");
        }
        sources.erase(sources.begin() + idx);
    });

    m.def("set_source", [](int idx, float x, float y, float vx, float vy, float radius,
                           std::array<float, 3> color, bool active, int mask) {
        auto& sources = *ScriptingEngine::instance->sources;
        if (idx < 0 || idx >= static_cast<int>(sources.size())) {
            throw nb::index_error("index out of range");
        }
        auto& src = sources[idx];
        src.x = x;
        src.y = y;
        src.vx = vx;
        src.vy = vy;
        src.radius = radius;
        src.active = active;
        src.color = color;
        src.mode_mask = mask;
    });

    m.def("get_source", [](int idx) {
        auto& sources = *ScriptingEngine::instance->sources;
        if (idx < 0 || idx >= static_cast<int>(sources.size())) {
            throw nb::index_error("index out of range");
        }
        return sources[idx];
    });

    m.def("get_sources", []() {
        const auto& sources = *ScriptingEngine::instance->sources;
        nb::list result;
        for (const auto& src : sources) {
            result.append(src);
        }
        return result;
    });

    nb::class_<PluginPanel>(m, "Panel")
        .def(nb::init<>())
        .def("add_button",
             [](PluginPanel& p, std::string id, std::string label, nb::callable on_click) {
                 Button b;
                 b.id = std::move(id);
                 b.label = std::move(label);
                 b.on_click = [on_click](const std::map<std::string, ExportValue>& vars) { on_click(vars); };
                 p.widgets.push_back(std::move(b));
             })
        .def("add_slider",
             [](PluginPanel& p, std::string id, std::string label, float default_val, float min, float max) {
                 p.widgets.push_back(SliderF{std::move(id), std::move(label), default_val, min, max});
             })
        .def("add_checkbox",
             [](PluginPanel& p, std::string id, std::string label, bool default_val) {
                 p.widgets.push_back(Checkbox{std::move(id), std::move(label), default_val});
             })
        .def("add_drag_int", [](PluginPanel& p, std::string id, std::string label, int default_val) {
            p.widgets.push_back(DragInt{std::move(id), std::move(label), default_val});
        });

    m.def("set_panel", [](PluginPanel panel) {
        if (ScriptingEngine::instance->current_script) {
            ScriptingEngine::instance->current_script->panel = std::move(panel);
        }
    });
}

#endif
