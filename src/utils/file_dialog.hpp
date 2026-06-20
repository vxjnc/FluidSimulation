#pragma once
#include <filesystem>
#include <span>
#include <string>
#include <utility>

#include <nfd.hpp>

namespace FileDialog {
    template <typename F>
    void Save(const std::string& defaultName, std::span<const nfdu8filteritem_t> filters, F&& onResult) {
        const std::string cwd = std::filesystem::current_path().string();
        NFD::UniquePath outPath;

        if (NFD::SaveDialog(outPath, filters.data(), static_cast<nfdfiltersize_t>(filters.size()),
                            cwd.c_str(), defaultName.c_str()) == NFD_OKAY) {
            std::forward<F>(onResult)(outPath.get());
        }
    }

    template <typename F> void Open(std::span<const nfdu8filteritem_t> filters, F&& onResult) {
        const std::string cwd = std::filesystem::current_path().string();
        NFD::UniquePath outPath;

        if (NFD::OpenDialog(outPath, filters.data(), static_cast<nfdfiltersize_t>(filters.size()),
                            cwd.c_str()) == NFD_OKAY) {
            std::forward<F>(onResult)(outPath.get());
        }
    }
}
