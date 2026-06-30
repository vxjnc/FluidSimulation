#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "src/scripting/plugin_settings.hpp"
#include "src/scripting/script_source.hpp"
#include "src/scripting/scripting_engine.hpp"
#include "src/utils/file_utils.hpp"

class PluginManager {
public:
    void scan() {
        plugins_.clear();
        if (!std::filesystem::exists("plugins")) {
            return;
        }
        for (auto& entry : std::filesystem::directory_iterator("plugins")) {
            if (!entry.is_directory()) {
                continue;
            }
            if (!std::filesystem::exists(entry.path() / "main.py")) {
                continue;
            }
            plugins_.emplace_back(entry.path().filename().string());
        }
    }

    void run_enabled(ScriptingEngine& engine, PluginSettings& settings) {
        sources_.clear();
        for (const auto& name : plugins_) {
            if (!settings.enabled[name]) {
                continue;
            }
            ScriptSource source;
            source.code = FileUtils::read_file("plugins/" + name + "/main.py");
            sources_.emplace(name, std::move(source));
            engine.run_script(sources_[name]);
        }
    }

    void stop_all(ScriptingEngine& engine) {
        for (const auto& [name, source] : sources_) {
            engine.stop_script(source.id);
        }
        sources_.clear();
    }

    const std::vector<std::string>& plugins() const { return plugins_; }

    const ScriptSource* get_source(const std::string& name) const {
        auto it = sources_.find(name);
        return it != sources_.end() ? &it->second : nullptr;
    }

private:
    std::vector<std::string> plugins_;
    std::map<std::string, ScriptSource> sources_;
};
