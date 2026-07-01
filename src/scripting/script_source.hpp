#pragma once

#include <limits>
#include <string>

struct ScriptSource {
    size_t id = next_id_();
    std::string code;

    static constexpr size_t INVALID_ID = std::numeric_limits<size_t>::max();

private:
    static size_t next_id_() {
        static size_t counter = 1;
        return counter++;
    }
};
