#pragma once

#include <cfloat>

#include "Application/Startup/GameStarterOptions.h"

#include "Utility/System/Path.h"

struct GameTestOptions : GameStarterOptions {
    Path testPath;
    Path rulesPath; // Explicit private MM6 rules directory, read only; copied to ramfs before startup.
    bool disposableMm7Profile = false; // Exact-filter, owner-only temp profile for a two-process MM7 save gate.
    Path mm6ChainProfile; // Exact-filter, owner-only temp profile of an MM6 campaign chain chapter, empty when not in one.
    float speed = FLT_MAX; // Test playback speed.
    bool helpPrinted = false;
    bool listRequested = false;

    static GameTestOptions parse(int argc, char **argv);
};
