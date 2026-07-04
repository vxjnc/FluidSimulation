#pragma once

#include <string>

#include "src/utils/observable.hpp"

struct ScriptingSettings {
    Observable<std::string> pythonPath;
};
