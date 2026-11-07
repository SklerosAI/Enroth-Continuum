#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6IndoorFaceLiveOwner.h"
#include "Engine/MM6/MM6IndoorFaceOverrides.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/MM6/MM6V6IndoorSaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "GUI/GUIWindow.h"
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6V6IndoorProbeSlot.h"
#include "MM6V6IndoorLiveRawOwnersForTest.h"

namespace {

std::string firstIndoorMap(const mm6::LoadedMapSource &maps) {
    const auto names = maps.expectedNames();
    const auto entry = std::find_if(names.begin(), names.end(), [](const std::string &name) {
        return name.ends_with(".dlv");
    });
    if (entry == names.end())
        throw std::runtime_error("MM6 v6 indoor probe lacks a source map pair");
    return entry->substr(0, entry->size() - 4) + ".blv";
}

} // namespace

// Run alone after the disposable v2 autosave seed. This constructs a v6
// indoor candidate in memory using one real BLV/DLV and the actual pre-event
// face checkpoint. A separate opt-in path captures genuine post-world owners.
// Neither production SaveLoad nor gameplay admission is opened.
GAME_TEST(MM6V6IndoorStage, DISABLED_PrivateSourceAndPreEventOwnersStageDetached) {
    const char *enable = std::getenv("ENROTH_MM6_V6_INDOOR_STAGE_ENABLE");
    const bool liveSessionWriter = enable &&
        std::string_view(enable) == "strict-indoor-v6-live-session-write";
    const bool postWorldWriter = enable &&
        (std::string_view(enable) == "strict-indoor-v6-postworld-write" ||
         liveSessionWriter);
    const bool writeCandidate = enable &&
        (std::string_view(enable) == "strict-indoor-v6-write" || postWorldWriter);
    const std::string_view candidateSlot = liveSessionWriter ?
        mm6::testing::V6_INDOOR_LIVE_SESSION_PROBE_SLOT :
        postWorldWriter ? mm6::testing::V6_INDOOR_POSTWORLD_PROBE_SLOT :
                          mm6::testing::V6_INDOOR_PROBE_SLOT;
    if (!enable || (!writeCandidate &&
                    std::string_view(enable) != "strict-indoor-v6-stage"))
        GTEST_SKIP() << "Opt in after creating a disposable MM6 v2 seed";
    const auto profile = mm6::testing::indoorProbeProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set an owner-only system-temp MM6 restart profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto originalSeed = mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6");
    auto raw = std::unique_ptr<mm6::RawSaveFields>(
        new mm6::RawSaveFields(mm6::decodeRawSave(originalSeed, GameTitle::MM6)));
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const std::string mapName = firstIndoorMap(maps);
    const MapId map = pMapTable->GetMapInfo(mapName);
    ASSERT_NE(map, MAP_INVALID);

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls())
        if (std::string_view(name).ends_with(".dlv") ||
            std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));

    bool completed = false;
    bool reached = false;
    bool aborted = false;
    std::vector<mm6::IndoorFaceState> preEventFaces;
    std::vector<std::int16_t> preEventDoorsData;
    std::vector<std::uint8_t> activeDelta;
    mm6::GeneratedNpcSaveFields generated;
    GameplayRandomState rng;
    mm6::EventRuntimeSaveFields eventRuntime;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    const auto parsedBase = deserializeMM6IndoorBase(maps.readBase(mapName));
    const auto projectedBase = projectMM6IndoorBase(parsedBase);
    game.runGameRoutine([&] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
        checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
            GameTitle::MM6,
            [&] {
                if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                    engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                    throw std::runtime_error("MM6 v6 indoor checkpoint preceded map reconstruction");
                preEventFaces = mm6::captureLiveIndoorFaceStates(
                    *pIndoor, projectedBase, GameTitle::MM6);
                preEventDoorsData = pIndoor->doorsData;
                activeDelta = pIndoor->mm6DeltaBytes;
                generated = mm6::captureGeneratedNpcFields(
                    pActors, *pNPCStats, mapName, GameTitle::MM6);
                rng = mm6::captureGameplayRngFields(
                    *application->component<EngineRandomComponent>(), GameTitle::MM6);
                mm6::EventRuntimeMapEntry active;
                active.mapName = mapName;
                active.respawnCount = pIndoor->dlv.respawnCount;
                active.lastRespawnDay = pIndoor->dlv.lastRespawnDay;
                active.mapVars = engine->_persistentVariables.mapVars;
                active.decorVars = engine->_persistentVariables.decorVars;
                active.bindings = mm6::captureEventRuntimeDecorationBindings(
                    pLevelDecorations, GameTitle::MM6);
                eventRuntime.maps.push_back(std::move(active));
                reached = true;
            },
            mm6::testing::PreEventCheckpointResult::AbortToMainMenu);
        try {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(true, 1);
        } catch (const mm6::testing::PreEventCheckpointAbortForTest &) {
            aborted = true;
        }
        completed = true;
    });
    for (int tick = 0; tick < 128 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);
    ASSERT_TRUE(aborted);
    ASSERT_TRUE(reached);
    ASSERT_FALSE(checkpoint->reachedMapLoad());
    ASSERT_FALSE(checkpoint->reachedInput());
    ASSERT_FALSE(preEventFaces.empty());
    ASSERT_FALSE(activeDelta.empty());

    const std::string deltaName = mapName.substr(0, mapName.size() - 4) + ".dlv";
    raw->metadata.header.mapFileName = mapName;
    raw->chest.activeDeltaName = deltaName;
    raw->chest.rawActiveDelta = activeDelta;
    const auto it = std::find_if(raw->maps.deltas.begin(), raw->maps.deltas.end(),
        [&deltaName](const mm6::SavedMapDelta &entry) { return entry.name == deltaName; });
    ASSERT_NE(it, raw->maps.deltas.end());
    it->bytes = activeDelta;
    auto rawBytes = mm6::encodeRawSave(*raw, GameTitle::MM6);

    std::unique_ptr<mm6::RawSaveFields> postWorldRaw;
    std::vector<Actor> postWorldActors;
    std::vector<SpriteObject> postWorldSprites;
    std::vector<LevelDecoration> postWorldDecorations;
    std::vector<mm6::IndoorFaceState> postWorldFaces;

    // A separate opt-in candidate observes the real NPC generation and map
    // reload boundary. The original fixture above remains a pre-event probe.
    if (postWorldWriter) {
        bool worldFinished = false;
        std::exception_ptr worldError;
        game.runGameRoutine([&] {
            try {
                checkpoint.reset();
                const auto freshNpcs = mm6::loadNpcStaticBaseline(
                    *engine->resources(), GameTitle::MM6);
                const auto verifyItem = [](std::int32_t id) {
                    return id >= 1 && id <= 580 &&
                        !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
                };
                auto represented = std::unique_ptr<mm6::StagedRawSessionFields>(
                    new mm6::StagedRawSessionFields(mm6::stageRawSessionFields(
                        rawBytes, freshNpcs, maps.expectedNames(),
                        [&maps](std::string_view name) { return maps.readBase(name); },
                        verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                        pTileTable->tileCount(), GameTitle::MM6)));
                *pParty = represented->party.party;
                mm6::chestTitleState() = represented->world.chestTitle;
                engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
                engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
                mm6::ScopedChestMapLoadOptIn chestOptIn(true);
                DoPrepareWorld(true, 1);
                if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                    engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                    throw std::runtime_error("MM6 v6 post-world writer lacked indoor world");
                generated = mm6::captureGeneratedNpcFields(
                    pActors, *pNPCStats, mapName, GameTitle::MM6);
                rng = mm6::captureGameplayRngFields(
                    *application->component<EngineRandomComponent>(), GameTitle::MM6);
                auto &active = eventRuntime.maps.front();
                active.mapVars = engine->_persistentVariables.mapVars;
                active.decorVars = engine->_persistentVariables.decorVars;
                active.bindings = mm6::captureEventRuntimeDecorationBindings(
                    pLevelDecorations, GameTitle::MM6);
                if (liveSessionWriter) {
                    mm6::testing::requireV6IndoorNativeResidualsForTest(
                        parsedBase, preEventDoorsData);
                    postWorldRaw = mm6::testing::captureV6IndoorLiveRawOwnersForTest(
                        *raw, mapName);
                }
                postWorldActors.assign(pActors.begin(), pActors.end());
                postWorldSprites = pSpriteObjects;
                postWorldDecorations = pLevelDecorations;
                postWorldFaces = mm6::captureLiveIndoorFaceStates(
                    *pIndoor, projectedBase, GameTitle::MM6);
            } catch (...) {
                worldError = std::current_exception();
            }
            worldFinished = true;
        });
        for (int tick = 0; tick < 128 && !worldFinished; ++tick)
            game.tick();
        ASSERT_TRUE(worldFinished);
        if (worldError)
            std::rethrow_exception(worldError);
        if (liveSessionWriter) {
            if (!postWorldRaw)
                throw std::runtime_error("MM6 v6 post-world raw owners were not captured");
            *raw = std::move(*postWorldRaw);
            rawBytes = mm6::encodeRawSave(*raw, GameTitle::MM6);
        }
    }

    bool stageFinished = false;
    std::exception_ptr stageError;
    std::unique_ptr<mm6::StagedV6IndoorSaveFields> staged;
    std::vector<std::uint8_t> candidateBytes;
    game.runGameRoutine([&] {
        try {
            const auto freshNpcs = mm6::loadNpcStaticBaseline(
                *engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto represented = std::unique_ptr<mm6::StagedRawSessionFields>(
                new mm6::StagedRawSessionFields(mm6::stageRawSessionFields(
                rawBytes, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), GameTitle::MM6)));
            if (!represented->indoorPlan || represented->outdoorPlan)
                throw std::runtime_error("MM6 v6 probe did not stage an indoor raw world");
            if (liveSessionWriter)
                mm6::testing::requireV6IndoorDetachedRawRoundTripForTest(
                    *raw, *represented, *pItemTable,
                    pOverlayTable->overlays.size());
            auto saved = std::make_unique<mm6::V6IndoorSaveFields>();
            saved->raw = std::move(*raw);
            saved->generatedNpcs = std::move(generated);
            saved->gameplayRng = std::move(rng);
            saved->eventRuntime = std::move(eventRuntime);
            mm6::captureV6IndoorNativeAnchors(
                *represented, &saved->actors, &saved->sprites, &saved->decorations);
            const std::vector<Actor> liveActors = postWorldWriter ?
                postWorldActors : std::vector<Actor>(pActors.begin(), pActors.end());
            saved->runtimeActors = mm6::captureIndoorActorRuntime(
                mapName, saved->actors, liveActors, GameTitle::MM6);
            saved->runtimeSprites = mm6::captureIndoorSpriteRuntime(
                mapName, saved->sprites,
                postWorldWriter ? postWorldSprites : pSpriteObjects, GameTitle::MM6);
            auto changedDecorations = postWorldWriter ?
                postWorldDecorations : pLevelDecorations;
            if (changedDecorations.empty())
                throw std::runtime_error("MM6 v6 IDRT probe has no decoration slot");
            if (!postWorldWriter)
                changedDecorations.front().uFlags = LevelDecorationFlags(
                    std::to_underlying(changedDecorations.front().uFlags) ^ 0x20u);
            const auto event = std::find_if(saved->eventRuntime.maps.begin(),
                                            saved->eventRuntime.maps.end(),
                [&mapName](const mm6::EventRuntimeMapEntry &entry) {
                    return entry.mapName == mapName;
                });
            if (event == saved->eventRuntime.maps.end())
                throw std::runtime_error("MM6 v6 IDRT probe lacks active EVRT map");
            saved->runtimeDecorations = mm6::captureIndoorDecorationRuntime(
                mapName, represented->indoorPlan->base,
                *event, *pDecorationTable,
                changedDecorations, GameTitle::MM6);
            auto changed = postWorldWriter ? postWorldFaces : preEventFaces;
            if (!postWorldWriter)
                changed.front().attributes ^= 1u;
            if (auto overrides = mm6::captureIndoorFaceOverrides(
                    mapName, preEventFaces, changed, GameTitle::MM6))
                saved->faceOverrides.maps.push_back(std::move(*overrides));
            candidateBytes = mm6::encodeV6IndoorSave(*saved, GameTitle::MM6);
            const auto readFaces = [&mapName, &preEventFaces](std::string_view requested) {
                if (requested != mapName)
                    throw std::runtime_error("MM6 v6 probe requested another face source");
                return preEventFaces;
            };
            staged.reset(new mm6::StagedV6IndoorSaveFields(mm6::stageV6IndoorSave(
                candidateBytes, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, *pDecorationTable,
                pOverlayTable->overlays.size(),
                pTileTable->tileCount(), readFaces, GameTitle::MM6)));
            if (liveSessionWriter)
                mm6::testing::requireV6IndoorDetachedRawRoundTripForTest(
                    staged->saved.raw, staged->represented, *pItemTable,
                    pOverlayTable->overlays.size());
        } catch (...) {
            stageError = std::current_exception();
        }
        stageFinished = true;
    });
    for (int tick = 0; tick < 128 && !stageFinished; ++tick)
        game.tick();
    ASSERT_TRUE(stageFinished);
    if (stageError)
        std::rethrow_exception(stageError);
    ASSERT_TRUE(staged);
    ASSERT_TRUE(staged->represented.indoorPlan);
    if (!postWorldWriter)
        EXPECT_EQ(staged->saved.faceOverrides.maps.size(), 1u);
    EXPECT_FALSE(staged->canEnterGameLoop());
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), originalSeed);
    if (writeCandidate) {
        mm6::testing::writeIndoorProbeCandidate(profile, candidateBytes, candidateSlot);
        EXPECT_EQ(mm6::testing::readIndoorProbeSlot(
            profile, candidateSlot), candidateBytes);
        EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), originalSeed);
    }
}
