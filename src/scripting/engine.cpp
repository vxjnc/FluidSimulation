#include "engine.hpp"

#ifdef SCRIPTING_AVAILABLE

#include <string>

#include <dlfcn.h>

namespace scripting {

    static bool g_available = false;
    static void* g_lib = nullptr;

    static void (*s_Py_Initialize)() = nullptr;
    static void (*s_Py_Finalize)() = nullptr;
    static int (*s_PyRun_SimpleString)(const char*) = nullptr;
    static const char* (*s_Py_GetVersion)() = nullptr;

    static std::string popen_result(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return {};
        }
        char buf[512] = {};
        fgets(buf, sizeof(buf), pipe);
        pclose(pipe);
        std::string s(buf);
        if (!s.empty() && s.back() == '\n') {
            s.pop_back();
        }
        return s;
    }

    static std::string find_python_exe() {
        for (const char* c : {"python3", "python3.12", "python3.11", "python3.10"}) {
            std::string r = popen_result(std::string("which ") + c + " 2>/dev/null");
            if (!r.empty()) {
                return r;
            }
        }
        return {};
    }

    static std::string find_libpython(const std::string& python_exe) {
        return popen_result(python_exe + " -c \"import sysconfig; print("
                                         "sysconfig.get_config_var('LIBDIR') + '/libpython'"
                                         " + sysconfig.get_config_var('LDVERSION') + '.so.1.0')\"");
    }

    template <typename T> static bool resolve(void* lib, const char* name, T& fn) {
        fn = reinterpret_cast<T>(dlsym(lib, name));
        if (!fn) {
            return false;
        }
        return true;
    }

    bool init() {
        std::string python_exe = find_python_exe();
        if (python_exe.empty()) {
            return false;
        }

        std::string libpath = find_libpython(python_exe);
        if (libpath.empty()) {
            return false;
        }

        std::string prefix = popen_result(python_exe + " -c \"import sys; print(sys.prefix)\"");

        g_lib = dlopen(libpath.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!g_lib) {
            return false;
        }

        if (!resolve(g_lib, "Py_Initialize", s_Py_Initialize) ||
            !resolve(g_lib, "Py_Finalize", s_Py_Finalize) ||
            !resolve(g_lib, "PyRun_SimpleString", s_PyRun_SimpleString) ||
            !resolve(g_lib, "Py_GetVersion", s_Py_GetVersion)) {
            dlclose(g_lib);
            g_lib = nullptr;
            return false;
        }

        if (!prefix.empty()) {
            setenv("PYTHONHOME", prefix.c_str(), 1);
        }

        s_Py_Initialize();
        g_available = true;
        return true;
    }

    void shutdown() {
        if (g_available) {
            s_Py_Finalize();
        }
        g_available = false;
        if (g_lib) {
            dlclose(g_lib);
            g_lib = nullptr;
        }
    }

    bool is_available() { return g_available; }

    bool run_string(const std::string& code) {
        if (!g_available) {
            return false;
        }
        return s_PyRun_SimpleString(code.c_str()) == 0;
    }

}

#else

namespace scripting {
    bool init() { return false; }
    void shutdown() {}
    bool is_available() { return false; }
    bool run_string(const std::string&) { return false; }
}

#endif
