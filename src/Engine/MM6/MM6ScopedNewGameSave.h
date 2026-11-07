#pragma once

#include <filesystem>

#include "MM6NewGameSaveRuntime.h"

namespace mm6 {

// Explicit native test seam at the SaveLoad boundary. The ordinary
// saveNewGame() MM6 guard remains closed. Callers provide an owner-local F3
// research root and native MM6 profile, then inspect the staged Party and
// candidate map/chest owners through a precommit callback. Pending actions
// are checked again after that callback, before any disk or process change.
std::unique_ptr<PublishedFirstMapCache> saveNewGameMM6ScopedForTest(
    const std::filesystem::path &profileRoot,
    const std::filesystem::path &privateMapResearchRoot,
    const FirstMapEntryProbe &probe);

} // namespace mm6
