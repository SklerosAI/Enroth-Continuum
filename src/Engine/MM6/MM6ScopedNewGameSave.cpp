#include "MM6ScopedNewGameSave.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>

#include "Engine/Engine.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Timer.h"

#include "MM6ChestTitleState.h"
#include "MM6HeldItemSaveFields.h"
#include "MM6LoadedMapSource.h"
#include "MM6PrivateMapSource.h"

namespace mm6 {

namespace {

std::unique_ptr<PublishedFirstMapCache> saveNewGameFromMapSource(
    const std::filesystem::path &profileRoot, std::span<const std::string> expectedNames,
    const DefaultDeltaReader &readDefault, const MapBaseReader &readBase,
    const FirstMapEntryProbe &probe) {
    if (!engine || engine->gameTitle() != GameTitle::MM6 || !pParty || !pNPCStats ||
        !gameTimer || !pActiveOverlayList || !pOverlayTable || !pMapTable ||
        !pGames_LOD || !pItemTable || !probe)
        throw std::runtime_error("MM6 scoped new-game save requires a loaded MM6 runtime");
    const MapId firstMap = pMapTable->GetMapInfo(NEW_GAME_FIRST_MAP);
    if (firstMap == MAP_INVALID || !pGames_LOD->exists(NEW_GAME_FIRST_MAP))
        throw std::runtime_error("MM6 scoped new-game save lacks New Sorpigal map identity");

    // A held-item cursor can be rebuilt only after all-owner commit. Refuse a
    // pending spell, target, turn or attack rather than silently dropping it.
    const auto actions = capturePendingActionStateForSave(GameTitle::MM6);
    (void) stageHeldCursorRestorePlan(captureHeldItem(*pParty, GameTitle::MM6),
                                      *pItemTable, actions, GameTitle::MM6);

    const KnownSaveFields seed = captureFirstMapKnownSeed(
        *pParty, *pNPCStats, *gameTimer, *pActiveOverlayList, "New game",
        pOverlayTable->overlays.size(), GameTitle::MM6);
    const auto verifyItem = [](std::int32_t id) {
        return id >= 1 && id <= 580 &&
            !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
    };
    const FirstMapEntryProbe preCommitProbe = [&](const Party &staged,
        const std::unordered_map<std::string, Blob> &candidateMapDeltas,
        const ChestTitleState &candidateChest) {
        probe(staged, candidateMapDeltas, candidateChest);
        // Archive reads and detached staging have completed. If input or a
        // queued action appeared during that work, refuse the save before the
        // first disk write; leave the action queued for normal game handling.
        const auto currentActions = capturePendingActionStateForSave(GameTitle::MM6);
        (void) stageHeldCursorRestorePlan(captureHeldItem(*pParty, GameTitle::MM6),
                                          *pItemTable, currentActions, GameTitle::MM6);
    };
    return runScopedFirstMapEntryForTest(
        profileRoot, seed, *pParty, *pNPCStats, *gameTimer, *pActiveOverlayList,
        expectedNames, readDefault, readBase, verifyItem,
        pOverlayTable->overlays.size(), localChestSeedEntropy,
        &pMapDeltas, &chestTitleState(), preCommitProbe, GameTitle::MM6);
}

} // namespace

std::unique_ptr<PublishedFirstMapCache> saveNewGameMM6ScopedForTest(
    const std::filesystem::path &profileRoot,
    const std::filesystem::path &privateMapResearchRoot,
    const FirstMapEntryProbe &probe) {
    const PrivateMapSource maps(privateMapResearchRoot);
    return saveNewGameFromMapSource(profileRoot, maps.expectedNames(),
        [](std::string_view name) { return pGames_LOD->read(name); },
        [&maps](std::string_view name) { return maps.readBase(name); }, probe);
}

std::unique_ptr<PublishedFirstMapCache> saveNewGameMM6FromLoadedMapsScopedForTest(
    const std::filesystem::path &profileRoot, const FirstMapEntryProbe &probe) {
    if (!engine || !pGames_LOD || !pMapTable)
        throw std::runtime_error("MM6 loaded-map save requires map resources");
    const LoadedMapSource maps(*pGames_LOD, *pMapTable, engine->gameTitle());
    return saveNewGameFromMapSource(profileRoot, maps.expectedNames(),
        [&maps](std::string_view name) { return maps.readDefaultDelta(name); },
        [&maps](std::string_view name) { return maps.readBase(name); }, probe);
}

} // namespace mm6
