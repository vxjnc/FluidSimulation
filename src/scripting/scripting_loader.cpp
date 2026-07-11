
#include "scripting_loader.hpp"

#include <iostream>
#include <print>
#include <string>

#include <dbg.h>

#include "src/app.hpp"
#include "src/scripting/scripting_engine.hpp"
#include "src/utils/dynlib.hpp"
#include "src/utils/python_find.hpp"

#ifdef _WIN32
#include <filesystem>

#include <windows.h>
#endif

void ScriptingLoader::init(Application* app, std::string_view pythonPath) {
    std::string exe = pythonPath.empty() ? python_find::find_exe() : std::string(pythonPath);
    dbg(exe);
    if (exe.empty()) {
        std::println(std::cerr, "Failed to find python");
        engine_ = new ScriptingEngine();
        return;
    }

    std::string libpath = python_find::find_libpython(exe);
    dbg(libpath);
    if (libpath.empty()) {
        std::println(std::cerr, "Failed to find libpython path");
        engine_ = new ScriptingEngine();
        return;
    }

    libpython_ = DynLib(libpath, true);
    if (!libpython_.valid()) {
        std::println(std::cerr, "Failed to find libpython");
        engine_ = new ScriptingEngine();
        return;
    }

#ifdef _WIN32
    SetDllDirectoryA(std::filesystem::path(libpath).parent_path().string().c_str());
#endif

    auto u8 = python_find::find_libscripting().u8string();
    std::string libscripting_path(u8.begin(), u8.end());
    dbg(libscripting_path);
    libscripting_ = DynLib(libscripting_path, true);
    if (!libscripting_.valid()) {
        std::println(std::cerr, "Failed to find libscripting");
        libpython_.close();
        engine_ = new ScriptingEngine();
        return;
    }

    using factory_fn = ScriptingEngine*(ScriptHost*, std::string_view);
    auto factory = libscripting_.fn<factory_fn>("create_scripting_engine");
    if (!factory) {
        std::println(std::cerr, "Failed to find create_scripting_engine");
        libscripting_.close();
        libpython_.close();
        engine_ = new ScriptingEngine();
        return;
    }

    engine_ = factory(app, exe);
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
