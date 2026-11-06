#pragma once

#include <cfloat>

#include "Application/Startup/GameStarterOptions.h"

#include "Utility/System/Path.h"

struct GameTestOptions : GameStarterOptions {
    Path testPath;
    Path rulesPath; // Explicit private MM6 rules directory, read only; copied to ramfs before startup.
    float speed = FLT_MAX; // Test playback speed.
    bool helpPrinted = false;
    bool listRequested = false;

    static GameTestOptions parse(int argc, char **argv);
};
