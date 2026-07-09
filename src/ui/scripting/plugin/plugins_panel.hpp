#pragma once

#include <map>
#include <string>

class PluginManager;
class ScriptingEngine;
struct PluginSettings;

class PluginsPanel {
public:
    void render(bool& open, PluginManager& manager, PluginSettings& settings, ScriptingEngine& engine);

private:
    std::string selected_;
    size_t selectedIndex_ = 0;
    std::map<std::string, std::string> readme_cache_;
};
