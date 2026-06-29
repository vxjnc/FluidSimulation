#include "python_find.hpp"

#include <format>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace python_find {
    std::string popen_result(const std::string& cmd) {
#ifdef _WIN32
        FILE* pipe = _popen(cmd.c_str(), "r");
#else
        FILE* pipe = popen(cmd.c_str(), "r");
#endif
        if (!pipe) {
            return {};
        }
        char buf[1024] = {};
        fgets(buf, sizeof(buf), pipe);
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        std::string s(buf);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
            s.pop_back();
        }
        return s;
    }

    std::string find_exe() {
#ifdef _WIN32
        for (const char* c : {"python.exe", "python3.exe"}) {
            std::string r = popen_result(std::format("where {} 2>nul", c));
            if (!r.empty()) {
                return r;
            }
        }
#else
        for (const char* c : {"python3", "python3.12", "python3.11", "python3.10"}) {
            std::string r = popen_result(std::format("which {} 2>/dev/null", c));
            if (!r.empty()) {
                return r;
            }
        }
#endif
        return {};
    }

    std::string find_libpython(const std::string& python_exe) {
#ifdef _WIN32
        return popen_result(python_exe + " -c \"import sysconfig; print("
                                         "sysconfig.get_config_var('BINDIR') + '\\\\python'"
                                         " + sysconfig.get_config_var('py_version_nodot') + '.dll')\"");
#else
        return popen_result(python_exe + " -c \"import sysconfig; print("
                                         "sysconfig.get_config_var('LIBDIR') + '/libpython'"
                                         " + sysconfig.get_config_var('LDVERSION') + '.so.1.0')\"");
#endif
    }

    std::string find_prefix(const std::string& python_exe) {
        return popen_result(python_exe + " -c \"import sys; print(sys.base_prefix)\"");
    }

    std::string find_libscripting() {
#ifdef _WIN32
        return "fluid_scripting.dll";
#else
        return "libfluid_scripting.so";
#endif
    }
}
