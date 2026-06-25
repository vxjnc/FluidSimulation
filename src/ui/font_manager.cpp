#include "font_manager.hpp"

#include "generated/fonts/JetBrainsMono_Regular.h"
#include "generated/fonts/NotoSans_Regular.h"

void FontManager::init(ImGuiIO& io) {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;

    ui_ = io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(static_cast<const void*>(s_font_NotoSans_Regular)),
                                         sizeof(s_font_NotoSans_Regular), 16.0f, &config);

    mono_ = io.Fonts->AddFontFromMemoryTTF(
        const_cast<void*>(static_cast<const void*>(s_font_JetBrainsMono_Regular)),
        sizeof(s_font_JetBrainsMono_Regular), 16.0f, &config);
}
