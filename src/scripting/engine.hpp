#pragma once

#include <functional>
#include <string>

namespace scripting {
    bool init();
    void shutdown();
    bool is_available();
    bool run_string(const std::string& code);

    void set_tick_callback(void* cb);
    void run_tick();

    void set_output_handler(std::function<void(std::string_view)> handler);
}
