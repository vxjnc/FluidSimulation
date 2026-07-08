#pragma once

#include <filesystem>
#include <string>

namespace python_find {
    std::string find_exe();
    std::string find_libpython(const std::string& python_exe);
    std::string find_prefix(const std::string& python_exe);
    std::filesystem::path find_libscripting();
}
