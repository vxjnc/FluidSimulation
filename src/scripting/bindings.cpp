#ifdef SCRIPTING_AVAILABLE

#include <array>

#include <Python.h>

#include "src/app.hpp"
#include "src/scripting/python_api.hpp"
#include "src/scripting/python_binding.hpp"
#include "src/scripting/scripting_engine.hpp"

extern "C" void _Py_Dealloc(PyObject*) {}

namespace {
    struct PyFluidSource {
        std::array<float, 3> color;
        float x;
        float y;
        float vx;
        float vy;
        float radius;
        bool active;
        int mask;
    };

    inline PyFluidSource to_agregat(const FluidSource& src) {
        return PyFluidSource{
            .color = src.color,
            .x = src.x,
            .y = src.y,
            .vx = src.vx,
            .vy = src.vy,
            .radius = src.radius,
            .active = src.active,
            .mask = src.mode_mask,
        };
    }
}

static auto FluidSimMethods = py_util::make_table(
    {{"on_tick",
      [](PyObject* cb) {
          if (!py::Callable_Check(cb)) {
              py::Err_SetString(py::type_error, "argument must be callable");
          }
          ScriptingEngine::instance->set_tick_callback(cb);
      }},

     {"add_source",
      [](float x, float y, float vx, float vy, float radius, float r, float g, float b) {
          auto& sources = ScriptingEngine::instance->app->getSources();
          sources.push_back(FluidSource(x, y, vx, vy, radius, std::array<float, 3>{r, g, b}));
          return sources.size() - 1;
      }},

     {
         "remove_source",
         [](int idx) {
             auto& sources = ScriptingEngine::instance->app->getSources();
             if (idx < 0 || idx >= static_cast<int>(sources.size())) {
                 py::Err_SetString(py::type_error, "index out of range");
                 return;
             }
             sources.erase(sources.begin() + idx);
         },
     },

     {"set_source",
      [](int idx, float x, float y, float vx, float vy, float radius, float r, float g, float b, bool active,
         int mask) {
          auto& sources = ScriptingEngine::instance->app->getSources();
          if (idx < 0 || idx >= static_cast<int>(sources.size())) {
              py::Err_SetString(py::type_error, "index out of range");
              return;
          }
          auto& src = sources[idx];
          src.x = x;
          src.y = y;
          src.vx = vx;
          src.vy = vy;
          src.radius = radius;
          src.active = active;
          src.color = {r, g, b};
          src.mode_mask = mask;
      }},

     {"get_source",
      [](int idx) -> PyObject* {
          auto& sources = ScriptingEngine::instance->app->getSources();
          if (idx < 0 || idx >= static_cast<int>(sources.size())) {
              py::Err_SetString(py::type_error, "index out of range");
              return nullptr;
          }
          return py_util::to_py(to_agregat(sources[idx]));
      }},

     {"get_sources", []() -> PyObject* {
          const auto& sources = ScriptingEngine::instance->app->getSources();
          PyObject* py_tuple = py::tuple_new(sources.size());
          if (!py_tuple) {
              return nullptr;
          }

          for (size_t i = 0; i < sources.size(); ++i) {
              py::tuple_set_item(py_tuple, i, py_util::to_py(to_agregat(sources[i])));
          }
          return py_tuple;
      }}});

static PyModuleDef fluidsim_module = {PyModuleDef_HEAD_INIT,
                                      "fluidsim",
                                      nullptr,
                                      -1,
                                      FluidSimMethods.data(),
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      nullptr};

PyMODINIT_FUNC PyInit_fluidsim() {
    PyObject* module = py::Module_Create2(&fluidsim_module, PYTHON_API_VERSION);
    if (!module) {
        return nullptr;
    }

    py::module_add_int_constant(module, "VELOCITY", FluidSource::Mode::VELOCITY);
    py::module_add_int_constant(module, "DYE_ADDITIVE", FluidSource::Mode::DYE_ADDITIVE);
    py::module_add_int_constant(module, "DYE_REPLACE", FluidSource::Mode::DYE_REPLACE);

    py_util::register_namedtuple<PyFluidSource>(module, "FluidSource");
    return module;
}

#endif
