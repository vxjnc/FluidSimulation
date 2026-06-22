#pragma once

#include <cstdio>
#include <string_view>

#include <imgui_internal.h>
#include <pfr.hpp>

#include "src/utils/observable.hpp"
#include "src/utils/string_escaping.hpp"

struct ImguiSerialization {
    template <typename T> static constexpr bool readField(const char* line, const char* name, T& out) {
        char fmt[64];
        if constexpr (is_observable_v<T>) {
            typename is_observable<T>::inner_type tmp;
            if (readField(line, name, tmp)) {
                out = tmp;
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            std::snprintf(fmt, sizeof(fmt), "%s=%%d", name);
            int v = 0;
            if (std::sscanf(line, fmt, &v) == 1) {
                out = (v != 0);
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, int>) {
            std::snprintf(fmt, sizeof(fmt), "%s=%%d", name);
            return std::sscanf(line, fmt, &out) == 1;
        }
        else if constexpr (std::is_same_v<T, float>) {
            std::snprintf(fmt, sizeof(fmt), "%s=%%f", name);
            return std::sscanf(line, fmt, &out) == 1;
        }
        else if constexpr (std::is_same_v<T, std::array<float, 3>>) {
            std::snprintf(fmt, sizeof(fmt), "%s=%%f,%%f,%%f", name);
            return std::sscanf(line, fmt, &out[0], &out[1], &out[2]) == 3;
        }
        else if constexpr (std::is_enum_v<T>) {
            std::snprintf(fmt, sizeof(fmt), "%s=%%d", name);
            int v = 0;
            if (std::sscanf(line, fmt, &v) == 1) {
                out = static_cast<T>(v);
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            size_t len = std::strlen(name);
            if (std::strncmp(line, name, len) == 0 && line[len] == '=') {
                out = StringEscaping::decodeString(line + len + 1);
                return true;
            }
            return false;
        }
        else {
            return false;
        }
    }
    template <typename Obj>
    static void readSection(const char* line, ImGuiID section_hash, ImGuiID section_name_hash, Obj& obj) {
        if (section_hash != section_name_hash) {
            return;
        }
        pfr::for_each_field_with_name(obj, [&](std::string_view name, auto& value) {
            readField(line, std::string(name).c_str(), value);
        });
    }
    static void ImguiReadLine(ImGuiContext*, ImGuiSettingsHandler* h, void* entry_data, const char* line);

    template <typename T>
    void static constexpr writeField(ImGuiTextBuffer* buf, const char* name, const T& value) {
        if constexpr (is_observable_v<T>) {
            writeField(buf, name, static_cast<typename is_observable<T>::inner_type>(value));
        }
        else if constexpr (std::is_same_v<T, bool>) {
            buf->appendf("%s=%d\n", name, value ? 1 : 0);
        }
        else if constexpr (std::is_same_v<T, int>) {
            buf->appendf("%s=%d\n", name, value);
        }
        else if constexpr (std::is_same_v<T, float>) {
            buf->appendf("%s=%.6f\n", name, value);
        }
        else if constexpr (std::is_same_v<T, std::array<float, 3>>) {
            buf->appendf("%s=%.6f,%.6f,%.6f\n", name, value[0], value[1], value[2]);
        }
        else if constexpr (std::is_enum_v<T>) {
            buf->appendf("%s=%d\n", name, static_cast<int>(value));
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            buf->appendf("%s=%s\n", name, StringEscaping::encodeString(value).c_str());
        }
    };
    template <typename Obj>
    static void writeSection(ImGuiTextBuffer* buf, const char* type_name, const char* section_name,
                             Obj& obj) {
        buf->appendf("[%s][%s]\n", type_name, section_name);
        pfr::for_each_field_with_name(obj, [&](std::string_view name, auto& value) {
            writeField(buf, std::string(name).c_str(), value);
        });
        buf->append("\n");
    }
    static void ImguiWriteAll(ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf);
};
