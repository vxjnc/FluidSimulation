#include "python_find.hpp"

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>

namespace python_find {
    namespace {
        template <typename... Args> std::string run_capture(Args&&... args) {
            constexpr size_t N = sizeof...(args) + 1;
            const char* argv[N] = {static_cast<const char*>(args)..., nullptr};

            reproc::process process;
            std::error_code ec = process.start(argv);
            if (ec) {
                return {};
            }

            std::string output;
            reproc::sink::string sink(output);
            ec = reproc::drain(process, sink, reproc::sink::null);
            if (ec) {
                return {};
            }

            while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
                output.pop_back();
            }
            return output;
        }
    }

    std::string find_exe() {
#ifdef _WIN32
        for (const char* c : {"python3.exe", "python.exe"}) {
            std::string r = run_capture("where", c);
            if (!r.empty()) {
                return r;
            }
        }
#else
        for (const char* c : {"python3", "python3.15", "python3.14", "python3.13", "python3.12"}) {
            std::string r = run_capture("which", c);
            if (!r.empty()) {
                return r;
            }
        }
#endif
        return {};
    }

    std::string find_libpython(const std::string& python_exe) {
#ifdef _WIN32
        return run_capture(python_exe.c_str(), "-c",
                           "import sysconfig; print(sysconfig.get_config_var('BINDIR') + '\\\\python3.dll')");
#else
        return run_capture(python_exe.c_str(), "-c",
                           "import sysconfig; print(sysconfig.get_config_var('LIBDIR') + '/libpython'"
                           " + sysconfig.get_config_var('LDVERSION') + '.so.1.0')");
#endif
    }

    std::string find_prefix(const std::string& python_exe) {
        return run_capture(python_exe.c_str(), "-c", "import sys; print(sys.base_prefix)");
    }

    std::string find_libscripting() {
#ifdef _WIN32
        return "fluid_scripting.dll";
#else
        return "libfluid_scripting.so";
#endif
    }
}
