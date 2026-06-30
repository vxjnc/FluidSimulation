#include "file_utils.hpp"

#include <fstream>

namespace FileUtils {
    std::string read_file(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return "";
        }

        auto size = file.tellg();
        std::string code;
        code.resize(size);

        file.seekg(0, std::ios::beg);
        file.read(&code[0], size);

        return std::move(code);
    }
}
