#include "dynlib.hpp"

#include <dbg.h>

#ifdef _WIN32
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

DynLib::DynLib(const std::string& path, bool global) {
#ifdef _WIN32
    DWORD flags = std::filesystem::path(path).is_absolute() ? LOAD_WITH_ALTERED_SEARCH_PATH : 0;
    handle_ = LoadLibraryExA(path.c_str(), nullptr, flags);
    if (!handle_) {
        DWORD errorCode = GetLastError();
        dbg(path, errorCode);
    }
#else
    int flags = RTLD_NOW | (global ? RTLD_GLOBAL : RTLD_LOCAL);
    handle_ = dlopen(path.c_str(), flags);
    if (!handle_) {
        std::string errorStr = dlerror();
        dbg(path, errorStr);
    }
#endif
}
void* DynLib::sym(const char* name) const {
    if (!handle_) {
        return nullptr;
    }
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#else
    return dlsym(handle_, name);
#endif
}

void DynLib::close() {
    if (!handle_) {
        return;
    }
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle_));
#else
    dlclose(handle_);
#endif
    handle_ = nullptr;
}
