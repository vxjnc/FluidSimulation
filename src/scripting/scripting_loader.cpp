#ifdef SCRIPTING_AVAILABLE

#include "scripting_loader.hpp"

#include "scripting_engine.hpp"

#include <format>
#include <string>

#include <dlfcn.h>

namespace {
    std::string popen_result(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return {};
        }
        char buf[1024] = {};
        if (fgets(buf, sizeof(buf), pipe) == nullptr) {
            pclose(pipe);
            return {};
        }
        pclose(pipe);
        std::string s(buf);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
            s.pop_back();
        }
        return s;
    }

    std::string find_python_exe() {
        for (const char* c : {"python3", "python3.12", "python3.11", "python3.10"}) {
            std::string r = popen_result(std::format("which {} 2>/dev/null", c));
            if (!r.empty()) {
                return r;
            }
        }
        return {};
    }

    std::string find_libpython(const std::string& python_exe) {
        return popen_result(python_exe + " -c \"import sysconfig; print("
                                         "sysconfig.get_config_var('LIBDIR') + '/libpython'"
                                         " + sysconfig.get_config_var('LDVERSION') + '.so.1.0')\"");
    }
}

void ScriptingLoader::init(std::vector<FluidSource>* sources, std::string_view pythonPath) {
    std::string exe = pythonPath.empty() ? find_python_exe() : std::string(pythonPath);
    if (exe.empty()) {
        engine_ = new ScriptingEngine();
        return;
    }

    std::string libpath = find_libpython(exe);
    if (libpath.empty()) {
        engine_ = new ScriptingEngine();
        return;
    }

    libpython_ = dlopen(libpath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!libpython_) {
        engine_ = new ScriptingEngine();
        return;
    }

    libscripting_ = dlopen("libscripting.so", RTLD_NOW | RTLD_GLOBAL);
    if (!libscripting_) {
        dlclose(libpython_);
        libpython_ = nullptr;
        engine_ = new ScriptingEngine();
        return;
    }

    using factory_fn = ScriptingEngine*(std::vector<FluidSource>*, std::string_view);
    auto factory = reinterpret_cast<factory_fn*>(dlsym(libscripting_, "create_scripting_engine"));
    if (!factory) {
        dlclose(libscripting_);
        dlclose(libpython_);
        libscripting_ = libpython_ = nullptr;
        engine_ = new ScriptingEngine();
        return;
    }

    engine_ = factory(sources, exe);
}

ScriptingLoader::~ScriptingLoader() {
    if (engine_) {
        using destroy_fn = void(ScriptingEngine*);
        auto destroy = reinterpret_cast<destroy_fn*>(dlsym(libscripting_, "destroy_scripting_engine"));
        if (destroy) {
            destroy(engine_);
        }
        engine_ = nullptr;
    }
    if (libscripting_) {
        dlclose(libscripting_);
        libscripting_ = nullptr;
    }
    if (libpython_) {
        dlclose(libpython_);
        libpython_ = nullptr;
    }
}

#endif
