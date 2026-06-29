
#include "scripting_loader.hpp"

#include <string>

#include "src/scripting/scripting_engine.hpp"
#include "src/utils/dynlib.hpp"
#include "src/utils/python_find.hpp"

void ScriptingLoader::init(std::vector<FluidSource>* sources, std::string_view pythonPath) {
    std::string exe = pythonPath.empty() ? python_find::find_exe() : std::string(pythonPath);
    if (exe.empty()) {
        engine_ = new ScriptingEngine();
        return;
    }

    std::string libpath = python_find::find_libpython(exe);
    if (libpath.empty()) {
        engine_ = new ScriptingEngine();
        return;
    }

    libpython_ = DynLib(libpath, true);
    if (!libpython_.valid()) {
        engine_ = new ScriptingEngine();
        return;
    }

    libscripting_ = DynLib(python_find::find_libscripting(), true);
    if (!libscripting_.valid()) {
        libpython_.close();
        engine_ = new ScriptingEngine();
        return;
    }

    using factory_fn = ScriptingEngine*(std::vector<FluidSource>*, std::string_view);
    auto factory = libscripting_.fn<factory_fn>("create_scripting_engine");
    if (!factory) {
        libscripting_.close();
        libpython_.close();
        engine_ = new ScriptingEngine();
        return;
    }

    engine_ = factory(sources, exe);
}

ScriptingLoader::~ScriptingLoader() {
    if (engine_) {
        auto destroy = libscripting_.fn<void(ScriptingEngine*)>("destroy_scripting_engine");
        if (destroy) {
            destroy(engine_);
        }
        engine_ = nullptr;
    }
    libscripting_.close();
    libpython_.close();
}
