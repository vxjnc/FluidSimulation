#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <string>

namespace python_find {
    std::string popen_result(const std::string& cmd);
    std::string find_exe();
    std::string find_libpython(const std::string& python_exe);
    std::string find_prefix(const std::string& python_exe);
    std::string find_libscripting();
}

#endif
