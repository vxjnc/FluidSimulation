#pragma once

#include <filesystem>

class FsimFile;

class FsimSerializer {
public:
    static void save(const std::filesystem::path& path, const FsimFile& file);
    static FsimFile load(const std::filesystem::path& path);

private:
    static constexpr char kMagic[4] = {'F', 'S', 'I', 'M'};
};
