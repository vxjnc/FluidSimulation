#pragma once
#ifdef SCRIPTING_AVAILABLE

class Script;

class ScriptConsole {
public:
    void render(float height, Script& script);
};

#endif
