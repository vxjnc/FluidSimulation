#pragma once

#include <string>

namespace scripting {
    bool init();
    void shutdown();
    bool is_available();
    bool run_string(const std::string& code);
}