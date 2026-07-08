#include "platform_utils.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <climits>

#include <unistd.h>
#endif

namespace PlatformUtils {
    std::filesystem::path executable_dir() {
#ifdef _WIN32
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path();
#else
        char path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len == -1) {
            return {};
        }
        path[len] = '\0';
        return std::filesystem::path(path).parent_path();
#endif
    }
}
