#ifdef SCRIPTING_AVAILABLE

#include <filesystem>
#include <optional>

#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <nfd.hpp>

#include "src/compute/fluid_source.hpp"
#include "src/scripting/scripting_engine.hpp"

namespace nb = nanobind;

NB_MODULE(_fluidsim_io, m) {
    m.def("output", [](const char* text) {
        if (text) {
            ScriptingEngine::instance->append_output(text);
        }
    });
}

int panel_tp_traverse(PyObject* self, visitproc visit, void* arg) {
    Py_VISIT(Py_TYPE(self));

    if (!nb::inst_ready(self)) {
        return 0;
    }

    ScriptPanel* p = nb::inst_ptr<ScriptPanel>(self);
    for (Widget& w : p->widgets) {
        if (Button* b = std::get_if<Button>(&w)) {
            nb::handle cb = nb::find(b->on_click);
            Py_VISIT(cb.ptr());
        }
    }
    return 0;
}

int panel_tp_clear(PyObject* self) {
    ScriptPanel* p = nb::inst_ptr<ScriptPanel>(self);
    for (Widget& w : p->widgets) {
        if (Button* b = std::get_if<Button>(&w)) {
            b->on_click = nullptr;
        }
    }
    return 0;
}

PyType_Slot panel_slots[] = {
    {Py_tp_traverse, (void*)panel_tp_traverse},
    {Py_tp_clear, (void*)panel_tp_clear},
    {0, nullptr},
};

NB_MODULE(fluidsim, m) {
    nb::class_<FluidSource>(m, "FluidSource")
        .def_rw("color", &FluidSource::color)
        .def_rw("x", &FluidSource::x)
        .def_rw("y", &FluidSource::y)
        .def_rw("vx", &FluidSource::vx)
        .def_rw("vy", &FluidSource::vy)
        .def_rw("radius", &FluidSource::radius)
        .def_rw("active", &FluidSource::active)
        .def_prop_rw(
            "mask", [](FluidSource& s) { return static_cast<FluidSource::Mode>(s.mode_mask); },
            [](FluidSource& s, FluidSource::Mode m) { s.mode_mask = static_cast<int>(m); })
        .def_rw("form", &FluidSource::form);

    nb::enum_<FluidSource::Mode>(m, "Mode", nb::is_flag())
        .value("VELOCITY", FluidSource::Mode::VELOCITY)
        .value("DYE_ADDITIVE", FluidSource::Mode::DYE_ADDITIVE)
        .value("DYE_REPLACE", FluidSource::Mode::DYE_REPLACE);

    nb::enum_<FluidSource::Form>(m, "Form")
        .value("CIRCLE", FluidSource::Form::CIRCLE)
        .value("LINE", FluidSource::Form::LINE)
        .value("RADIAL", FluidSource::Form::RADIAL);

    m.def("on_tick", [](std::optional<nb::callable> callback) {
        ScriptingEngine::instance->set_tick_callback(callback.has_value() ? callback->ptr() : nullptr);
    });

    m.def(
        "add_source",
        [](float x, float y, float vx, float vy, float radius, std::array<float, 3> color) -> FluidSource& {
            auto& sources = *ScriptingEngine::instance->sources;
            if (sources.size() >= sources.capacity()) {
                throw nb::index_error("source limit (1024) reached");
            }
            sources.push_back(FluidSource(x, y, vx, vy, radius, color));
            return sources.back();
        },
        nb::rv_policy::reference);

    m.def("remove_source", [](int idx) {
        auto& sources = *ScriptingEngine::instance->sources;
        if (idx < 0 || idx >= static_cast<int>(sources.size())) {
            throw nb::index_error("index out of range");
        }
        sources.erase(sources.begin() + idx);
    });

    m.def(
        "get_source",
        [](int idx) -> FluidSource& {
            auto& sources = *ScriptingEngine::instance->sources;
            if (idx < 0 || idx >= static_cast<int>(sources.size())) {
                throw nb::index_error("index out of range");
            }
            return sources[idx];
        },
        nb::rv_policy::reference);

    m.def("get_sources", []() {
        auto& sources = *ScriptingEngine::instance->sources;
        nb::list result;
        for (auto& src : sources) {
            result.append(nb::cast(src, nb::rv_policy::reference));
        }
        return result;
    });

    nb::class_<ScriptPanel>(m, "Panel", nb::type_slots(panel_slots))
        .def(nb::init<>())
        .def_rw("title", &ScriptPanel::title)
        .def("add_button",
             [](ScriptPanel& p, std::string id, std::string label,
                std::function<void(const std::map<std::string, ExportValue>&)> on_click) {
                 Button b;
                 b.id = std::move(id);
                 b.label = std::move(label);
                 b.on_click = std::move(on_click);
                 p.widgets.push_back(std::move(b));
             })
        .def("add_slider",
             [](ScriptPanel& p, std::string id, std::string label, float default_val, float min, float max) {
                 p.widgets.push_back(SliderF{std::move(id), std::move(label), default_val, min, max});
             })
        .def("add_checkbox",
             [](ScriptPanel& p, std::string id, std::string label, bool default_val) {
                 p.widgets.push_back(Checkbox{std::move(id), std::move(label), default_val});
             })
        .def("add_drag_int",
             [](ScriptPanel& p, std::string id, std::string label, int default_val) {
                 p.widgets.push_back(DragInt{std::move(id), std::move(label), default_val});
             })
        .def("sameline", [](ScriptPanel& p) { p.widgets.push_back(SameLine{}); });

    m.def("set_panel", [](ScriptPanel panel) { ScriptingEngine::instance->set_panel(std::move(panel)); });
    m.def("set_widget_value",
          [](std::string id, ExportValue value) { ScriptingEngine::instance->set_widget_value(id, value); });
    m.def("set_widget_label",
          [](std::string id, std::string label) { ScriptingEngine::instance->set_widget_label(id, label); });

    m.def(
        "open_file_dialog",
        [](std::optional<std::vector<std::pair<std::string, std::string>>> filters)
            -> std::optional<std::string> {
            std::vector<nfdu8filteritem_t> items;
            if (filters) {
                for (auto& [name, ext] : *filters) {
                    items.emplace_back(name.c_str(), ext.c_str());
                }
            }
            const std::string cwd = std::filesystem::current_path().string();
            NFD::UniquePath outPath;
            NFD::Init();
            auto result = NFD::OpenDialog(outPath, items.data(), static_cast<nfdfiltersize_t>(items.size()),
                                          cwd.c_str());
            NFD::Quit();
            if (result == NFD_OKAY) {
                return std::string(outPath.get());
            }

            return std::nullopt;
        },
        nb::arg("filters") = nb::none());

    m.def(
        "save_file_dialog",
        [](std::optional<std::vector<std::pair<std::string, std::string>>> filters,
           std::string default_name) -> std::optional<std::string> {
            std::vector<nfdu8filteritem_t> items;
            if (filters) {
                for (auto& [name, ext] : *filters) {
                    items.emplace_back(name.c_str(), ext.c_str());
                }
            }
            const std::string cwd = std::filesystem::current_path().string();
            NFD::UniquePath outPath;
            NFD::Init();
            auto result = NFD::SaveDialog(outPath, items.data(), static_cast<nfdfiltersize_t>(items.size()),
                                          cwd.c_str(), default_name.c_str());
            NFD::Quit();
            if (result == NFD_OKAY) {
                return std::string(outPath.get());
            }
            return std::nullopt;
        },
        nb::arg("filters") = nb::none());
}

#endif
