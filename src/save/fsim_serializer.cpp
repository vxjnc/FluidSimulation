#include "fsim_serializer.hpp"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <zlib.h>
#include <zpp_bits.h>

#include "src/save/fsim_file.hpp"

void FsimSerializer::save(const std::filesystem::path& path, const FsimFile& file) {
    std::vector<std::byte> raw;
    auto out = zpp::bits::out(raw);
    if (auto err = out(file); zpp::bits::failure(err)) {
        throw std::runtime_error("Serialization failed");
    }

    uLong compressedSize = compressBound(static_cast<uLong>(raw.size()));
    std::vector<std::byte> compressed(compressedSize);
    if (compress2(reinterpret_cast<Bytef*>(compressed.data()), &compressedSize,
                  reinterpret_cast<const Bytef*>(raw.data()), static_cast<uLong>(raw.size()),
                  Z_BEST_COMPRESSION) != Z_OK) {
        throw std::runtime_error("Compression failed");
    }
    compressed.resize(compressedSize);

    std::ofstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file for writing");
    }

    f.write(kMagic, sizeof(kMagic));
    uint32_t version = FsimFile::kVersion;
    uint64_t uncompressedSize = raw.size();
    f.write(reinterpret_cast<const char*>(&version), sizeof(version));
    f.write(reinterpret_cast<const char*>(&uncompressedSize), sizeof(uncompressedSize));
    f.write(reinterpret_cast<const char*>(compressed.data()),
            static_cast<std::streamsize>(compressed.size()));
}

FsimFile FsimSerializer::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file for reading");
    }

    char magic[4];
    f.read(magic, sizeof(magic));
    if (std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) {
        throw std::runtime_error("Invalid .fsim file");
    }

    uint32_t version;
    f.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version > FsimFile::kVersion) {
        throw std::runtime_error("Unsupported .fsim version");
    }

    uint64_t uncompressedSize;
    f.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));

    std::vector<char> compressed(std::istreambuf_iterator<char>(f), {});

    std::vector<std::byte> raw(uncompressedSize);
    uLongf outSize = static_cast<uLongf>(uncompressedSize);
    if (uncompress(reinterpret_cast<Bytef*>(raw.data()), &outSize,
                   reinterpret_cast<const Bytef*>(compressed.data()),
                   static_cast<uLong>(compressed.size())) != Z_OK) {
        throw std::runtime_error("Decompression failed");
    }

    FsimFile file;
    auto in = zpp::bits::in(raw);
    if (auto err = in(file); zpp::bits::failure(err)) {
        throw std::runtime_error("Deserialization failed");
    }

    return file;
}
