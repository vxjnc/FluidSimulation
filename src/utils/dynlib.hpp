#pragma once
#include <string>

class DynLib {
public:
    DynLib() = default;
    explicit DynLib(const std::string& path, bool global = false);
    ~DynLib() { close(); }

    DynLib(const DynLib&) = delete;
    DynLib& operator=(const DynLib&) = delete;
    DynLib(DynLib&& o) noexcept : handle_(o.handle_) { o.handle_ = nullptr; }
    DynLib& operator=(DynLib&& o) noexcept {
        if (this != &o) {
            close();
            handle_ = o.handle_;
            o.handle_ = nullptr;
        }
        return *this;
    }

    bool valid() const { return handle_ != nullptr; }
    void* sym(const char* name) const;
    void close();

    template <typename Fn> Fn* fn(const char* name) const { return reinterpret_cast<Fn*>(sym(name)); }

private:
    void* handle_ = nullptr;
};
