#include "MM6NewGameSession.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"

#include "GUI/UI/UIStatusBar.h"

#include "Library/Logger/Logger.h"
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6ActiveActorOutdoorDeltaCapture.h"
#include "MM6ActiveActorSection.h"
#include "MM6BuffSaveFields.h"
#include "MM6ChestMapLoadRoute.h"
#include "MM6ChestSaveFields.h"
#include "MM6ChestSeedAndOwnership.h"
#include "MM6CoreSaveRuntime.h"
#include "MM6CreationItemSeed.h"
#include "MM6DurableSaveFields.h"
#include "MM6EventRuntimeSaveFields.h"
#include "MM6FirstMapManualSave.h"
#include "MM6GameplayRngSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6HirelingSaveRuntime.h"
#include "MM6LoadedMapSource.h"
#include "MM6NewGameSaveRuntime.h"
#include "MM6NpcSaveFields.h"
#include "MM6NpcStaticBaseline.h"
#include "MM6OutdoorWire.h"
#include "MM6PersistentEventPublication.h"
#include "MM6RawCursorImagePlan.h"
#include "MM6RawCursorShopProcessOwner.h"
#include "MM6RawNewGameSaveRuntime.h"
#include "MM6RawPartyInventoryStage.h"
#include "MM6RawSessionStage.h"
#include "MM6RawV2Admission.h"
#include "MM6RecoverySaveRuntime.h"
#include "MM6ResistanceSaveFields.h"
#include "MM6SaveMetadata.h"
#include "MM6ShopTitleState.h"
#include "MM6V4FullSessionPreflight.h"
#include "MM6V4OutdoorDecorationReplay.h"
#include "MM6V4SaveFields.h"
#include "MM6V5DerivedActorNames.h"

namespace mm6 {

static std::unique_ptr<RawSaveFields> stagedNewGameSave;

static bool isMM6ItemTemplate(std::int32_t id) {
    return id >= 1 && id <= 580 && !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
}

static std::unique_ptr<StagedRawSessionFields> stageNewGameSave(const LoadedMapSource &maps, const ChestTitleState &chest) {
    const auto seeded = seedMM6CreationItems(*pParty, *pItemTable, GameTitle::MM6);
    if (!seeded)
        throw std::runtime_error("the creation items have no native MM6 records");
    KnownSaveFields known = captureFirstMapKnownSeed(*pParty, *pNPCStats, *gameTimer, *pActiveOverlayList, "New game",
                                                     pOverlayTable->overlays.size(), GameTitle::MM6);
    known.chest.lcgState = chest.lcgState;
    known.chest.generatedArtifactFlags = chest.generatedArtifactFlags;
    const PreparedRawNewGameAutosave prepared = prepareOfflineRawNewGameAutosave(
        known, seeded->party, seeded->inventory, seeded->cursor, emptyNewGameShopTitleState(GameTitle::MM6), {},
        maps.expectedNames(), [&maps](std::string_view name) { return maps.readDefaultDelta(name); },
        [&maps](std::string_view name) { return maps.readBase(name); }, isMM6ItemTemplate, *pItemTable, GameTitle::MM6);
    const NpcStaticBaseline freshNpcs = loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
    auto staged = std::unique_ptr<StagedRawSessionFields>(new StagedRawSessionFields(stageRawSessionFields(
        prepared.bytes, freshNpcs, maps.expectedNames(), [&maps](std::string_view name) { return maps.readBase(name); },
        isMM6ItemTemplate, *pItemTable, pOverlayTable->overlays.size(), pTileTable->tileCount(), GameTitle::MM6)));
    if (staged->needsRecovery || !staged->outdoorPlan || staged->indoorPlan || staged->outdoorPlan->mapName != NEW_GAME_FIRST_MAP)
        throw std::runtime_error("the staged new-game save is not a first-map save");
    return staged;
}

static void publishNewGameSave(StagedRawSessionFields *staged) {
    *pParty = staged->party.party;
    *pNPCStats = staged->world.world.npcs;
    *gameTimer = staged->world.visuals.timer;
    *pActiveOverlayList = staged->world.visuals.overlays;
    pMapDeltas = std::move(staged->world.world.mapDeltas);
    chestTitleState() = staged->world.chestTitle;
    publishFreshChildPersistentEventZeros(&engine->_persistentVariables, staged->persistentEvents, GameTitle::MM6);
    publishRawCursorShopOwners(&rawHeldCursorState(GameTitle::MM6), &shopTitleState(), staged->raw.saved,
                               {std::move(staged->raw.cursor), std::move(staged->raw.shops)}, GameTitle::MM6);
}

bool startNewGameSession() {
    stagedNewGameSave.reset();
    try {
        if (!engine || engine->gameTitle() != GameTitle::MM6 || !pGames_LOD || !pMapTable || !pItemTable || !pParty ||
            !pOverlayTable || !pTileTable)
            throw std::runtime_error("MM6 new game needs a loaded MM6 runtime");
        const LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
        ChestTitleState chest;
        const auto gameRandomSeed = [] { return std::optional(static_cast<std::uint32_t>(grng->random(std::numeric_limits<int>::max()))); };
        if (!initializePortNewGameChestTitleState(GameTitle::MM6, &chest, gameRandomSeed))
            throw std::runtime_error("MM6 new game has no chest seed");
        std::unique_ptr<StagedRawSessionFields> staged;
        try {
            staged = stageNewGameSave(maps, chest);
        } catch (const std::exception &e) {
            MM_WARNING("MM6 new game can't be saved: {}", e.what());
        }

        if (staged) {
            auto saved = std::make_unique<RawSaveFields>(staged->raw.saved);
            publishNewGameSave(staged.get());
            stagedNewGameSave = std::move(saved);
        } else {
            std::unordered_map<std::string, Blob> mapDeltas;
            for (const std::string &name : maps.expectedNames())
                mapDeltas.emplace(name, maps.readDefaultDelta(name));
            pMapDeltas = std::move(mapDeltas);
            chestTitleState() = chest;
        }
        admitChestMapLoad(pMapTable->GetMapInfo(NEW_GAME_FIRST_MAP));
    } catch (const std::exception &e) {
        MM_ERROR("MM6 new game could not prepare the first map: {}", e.what());
    }
    if (!stagedNewGameSave && engine && engine->_statusBar)
        engine->_statusBar->setEvent("This party can't be saved yet");
    return stagedNewGameSave != nullptr;
}

static std::vector<std::uint8_t> captureNewGameAnchor() {
    if (!stagedNewGameSave || !engine || engine->gameTitle() != GameTitle::MM6 || !pMapTable || !pOutdoor ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR || engine->_currentLoadedMapId != pMapTable->GetMapInfo(NEW_GAME_FIRST_MAP))
        throw std::runtime_error("MM6 new-game anchor needs the new-game session on the first map");
    const LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    if (pMapDeltas.size() != maps.expectedNames().size())
        throw std::runtime_error("MM6 new-game anchor map cache shape changed");
    requireRawV2RepresentableState(*pParty, {}, GameTitle::MM6);
    (void) stageRawCursorImagePlan(*pParty, rawHeldCursorState(GameTitle::MM6), *pItemTable, GameTitle::MM6);

    const auto v4 = std::make_unique<V4SaveFields>();
    V3SaveFields &fields = v4->v3;
    fields.raw = *stagedNewGameSave;
    fields.raw.core = captureCoreFields(*pParty, GameTitle::MM6);
    const auto durable = captureDurableFields(*pParty, GameTitle::MM6);
    fields.raw.bits = durable.bits;
    fields.raw.effects = durable.effects;
    fields.raw.inventory = captureRawPartyInventories({*pParty, fields.raw.inventory}, *pItemTable, GameTitle::MM6);
    fields.raw.metadata = captureMetadata(fields.raw.metadata.header.displayName, GameTitle::MM6);
    fields.raw.npcs = captureNpcFields(*pNPCStats, GameTitle::MM6);
    fields.raw.resistances = captureResistanceFields(*pParty, GameTitle::MM6);
    fields.raw.buffs = captureBuffFields(*pParty, GameTitle::MM6);
    fields.raw.hirelings = captureHirelingFields(*pParty, GameTitle::MM6);
    fields.raw.recovery = captureRecoveryFields(*pParty, GameTitle::MM6);
    const auto held = captureRawHeldCursor(rawHeldCursorState(GameTitle::MM6), GameTitle::MM6);
    if (!held)
        throw std::runtime_error("MM6 new-game anchor cursor owner is not representable");
    fields.raw.heldItem = *held;
    fields.raw.shops = shopTitleState();

    const Blob delta = captureOutdoorDeltaWithActiveActors(*pOutdoor, GameTitle::MM6);
    const std::string activeName = "oute3.ddm";
    const auto *deltaBegin = static_cast<const std::uint8_t *>(delta.data());
    const std::vector<std::uint8_t> current(deltaBegin, deltaBegin + delta.size());
    fields.raw.chest = captureChestSaveFields(chestTitleState(), activeName, current, GameTitle::MM6);
    for (SavedMapDelta &entry : fields.raw.maps.deltas) {
        const auto it = pMapDeltas.find(entry.name);
        if (it == pMapDeltas.end())
            throw std::runtime_error("MM6 new-game anchor map cache lacks a saved delta");
        const auto *begin = static_cast<const std::uint8_t *>(it->second.data());
        if (entry.name == activeName)
            entry.bytes = current;
        else if (!std::ranges::equal(entry.bytes, std::span(begin, it->second.size())))
            throw std::runtime_error("MM6 new-game anchor nonactive map cache changed");
    }
    const std::vector<Actor> actors(pActors.begin(), pActors.end());
    fields.activeActorPayload = encodeActiveActorSection(actors, GameTitle::MM6);
    fields.generatedNpcs = captureGeneratedNpcFields(pActors, *pNPCStats, NEW_GAME_FIRST_MAP, GameTitle::MM6);
    const auto random = application->component<EngineRandomComponent>();
    fields.gameplayRng = captureGameplayRngFields(*random, GameTitle::MM6);
    EventRuntimeMapEntry eventRuntime;
    eventRuntime.mapName = NEW_GAME_FIRST_MAP;
    eventRuntime.respawnCount = pOutdoor->ddm.respawnCount;
    eventRuntime.lastRespawnDay = pOutdoor->ddm.lastRespawnDay;
    eventRuntime.mapVars = engine->_persistentVariables.mapVars;
    eventRuntime.decorVars = engine->_persistentVariables.decorVars;
    eventRuntime.bindings = captureEventRuntimeDecorationBindings(pLevelDecorations, GameTitle::MM6);
    v4->eventRuntime.maps.push_back(std::move(eventRuntime));
    std::vector<std::uint8_t> bytes = encodeV4Save(*v4, GameTitle::MM6);

    const NpcStaticBaseline freshNpcs = loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
    const auto preflight = std::unique_ptr<V4FullSessionPreflight>(new V4FullSessionPreflight(stageV4FullSessionPreflight(
        bytes, freshNpcs, maps.expectedNames(), [&maps](std::string_view name) { return maps.readBase(name); },
        isMM6ItemTemplate, *pItemTable, pOverlayTable->overlays.size(), pTileTable->tileCount(), pActors.size(),
        random->type(), pLevelDecorations, GameTitle::MM6)));
    const auto expectedEvents = capturePersistentEventBoundary(preflight->v3.represented.persistentEvents, GameTitle::MM6);
    const auto liveEvents = decodeOutdoorDelta(pOutdoor->mm6DeltaBytes).part(OutdoorDeltaPart::PersistentBytes);
    if (!std::ranges::equal(expectedEvents, liveEvents) ||
        preflight->activeEventRuntime.mapVars != engine->_persistentVariables.mapVars ||
        preflight->activeEventRuntime.decorVars != engine->_persistentVariables.decorVars)
        throw std::runtime_error("MM6 new-game anchor event owners changed");
    requireFirstMapOutdoorDecorationReplay(NEW_GAME_FIRST_MAP, maps.readBase(NEW_GAME_FIRST_MAP),
                                           preflight->activeEventRuntime, *pDecorationTable, pLevelDecorations,
                                           GameTitle::MM6);
    return bytes;
}

void armNewGameSessionAtFirstMap() {
    if (!stagedNewGameSave)
        return;
    try {
        const std::vector<Actor> live(pActors.begin(), pActors.end());
        const std::vector<Actor> canonical = canonicalActorsWithDerivedNamesRemoved(live, *pMonsterStats, GameTitle::MM6); // Spawns copy the static names, a loaded actor has none.
        for (std::size_t slot = 0; slot < canonical.size(); ++slot) {
            pActors[slot].monsterInfo.name = canonical[slot].monsterInfo.name;
            pActors[slot].monsterInfo.internalName = canonical[slot].monsterInfo.internalName;
        }
        armFirstMapManualSave(captureNewGameAnchor(), GameTitle::MM6);
    } catch (const std::exception &e) {
        MM_ERROR("MM6 new game can't be saved, its first map was refused: {}", e.what());
    }
    stagedNewGameSave.reset();
}

} // namespace mm6
