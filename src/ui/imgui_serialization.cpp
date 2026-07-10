#include "imgui_serialization.hpp"

#include "src/ui/imgui_manager.hpp"

void ImguiSerialization::ImguiReadLine(ImGuiContext*, ImGuiSettingsHandler* h, void* entry_data,
                                       const char* line) {
    auto* self = static_cast<ImGuiManager*>(h->UserData);
    if (!entry_data) {
        return;
    }

    auto section_hash = static_cast<ImGuiID>((intptr_t)entry_data);

    readSection(line, section_hash, ImHashStr("Visibility"), self->visibility) ||
        readSection(line, section_hash, ImHashStr("AppSettings"), *self->settings) ||
        readSection(line, section_hash, ImHashStr("RenderSettings"), self->settings->renderSettings) ||
        readSection(line, section_hash, ImHashStr("SplatSettings"), self->settings->splatSettings) ||
        readSection(line, section_hash, ImHashStr("UISettings"), self->settings->ui) ||
        readSection(line, section_hash, ImHashStr("Scripting"), self->settings->scripting) ||
        readSection(line, section_hash, ImHashStr("Plugins"), self->settings->plugins);
}

void ImguiSerialization::ImguiWriteAll(ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf) {
    auto* self = static_cast<ImGuiManager*>(h->UserData);
    writeSection(buf, h->TypeName, "Visibility", self->visibility);
    writeSection(buf, h->TypeName, "AppSettings", *self->settings);
    writeSection(buf, h->TypeName, "RenderSettings", self->settings->renderSettings);
    writeSection(buf, h->TypeName, "SplatSettings", self->settings->splatSettings);
    writeSection(buf, h->TypeName, "UISettings", self->settings->ui);
    writeSection(buf, h->TypeName, "Scripting", self->settings->scripting);
    writeSection(buf, h->TypeName, "Plugins", self->settings->plugins);
}
