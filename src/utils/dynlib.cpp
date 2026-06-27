#include "dynlib.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

DynLib::DynLib(std::string_view path, bool global) {
#ifdef _WIN32
    handle_ = LoadLibraryA(path.data());
#else
    int flags = RTLD_NOW | (global ? RTLD_GLOBAL : RTLD_LOCAL);
    handle_ = dlopen(path.data(), flags);
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

DynLib::~DynLib() { close(); }
