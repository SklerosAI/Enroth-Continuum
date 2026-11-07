#pragma once

#include <filesystem>

#include "MM6NewGameSaveRuntime.h"

namespace mm6 {

// Production-shaped, read-only first-map preparation from the loaded MM6
// Games archive. Both the known-field session and outdoor plan own their
// inputs. This does not write a save, publish globals or open saveNewGame().
// The session remains partial until every mandatory MM6 save owner is joined.
struct PreparedLoadedFirstMap {
    PreparedNewGameSession session;
    DetachedOutdoorWorldPlan outdoor;
};

PreparedLoadedFirstMap prepareNewGameMM6FromLoadedMaps();

// Explicit native test seam at the SaveLoad boundary. The ordinary
// saveNewGame() MM6 guard remains closed. Callers provide an owner-local F3
// research root and native MM6 profile, then inspect the staged Party and
// candidate map/chest owners through a precommit callback. Pending actions
// are checked again after that callback, before any disk or process change.
std::unique_ptr<PublishedFirstMapCache> saveNewGameMM6ScopedForTest(
    const std::filesystem::path &profileRoot,
    const std::filesystem::path &privateMapResearchRoot,
    const FirstMapEntryProbe &probe);

// Same bounded temporary-profile route, with names and map bytes obtained
// only from the loaded MM6 game archive and map table.
std::unique_ptr<PublishedFirstMapCache> saveNewGameMM6FromLoadedMapsScopedForTest(
    const std::filesystem::path &profileRoot, const FirstMapEntryProbe &probe);

} // namespace mm6
