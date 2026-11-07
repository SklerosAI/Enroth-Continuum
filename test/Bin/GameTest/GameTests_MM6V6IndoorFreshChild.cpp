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

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6EventRuntimeSaveFields.h"
#include "Engine/MM6/MM6EventRuntimeRestoreForTest.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6IndoorFaceLiveOwner.h"
#include "Engine/MM6/MM6IndoorFaceOverrides.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6PostWorldAdmissionForTest.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6V6IndoorSaveFields.h"
#include "Engine/MM6/MM6V6IndoorPreEventOwnerGate.h"
#include "Engine/MM6/MM6V6PreEventIndoorDecorationReplayForTest.h"
#include "Engine/MM6/MM6V6PreEventIndoorSpriteReplayForTest.h"
#include "Engine/MM6/MM6V6PreEventIndoorActorReplayForTest.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Party.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "Engine/Graphics/Overlays.h"
#include "GUI/GUIWindow.h"
#include "GUI/GUIMessageQueue.h"
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6V6IndoorProbeSlot.h"
#include "MM6V6IndoorLiveRawOwnersForTest.h"

// Run only after the strict v6 disposable writer, in a distinct child. It
// reconstructs the saved DLV through the real indoor loader and observes its
// pre-OnMapReload face base, then re-stages every detached v6 owner. A second
// bounded load publishes IDRT, ISRT and IART before OnMapReload. The post-world
// mode also restores GNPC and RNGS, then checks exact owners after events.
// An additional opt-in mode allows a bounded first input only after those
// owner checks; production SaveLoad remains closed.
GAME_TEST(MM6V6IndoorFreshChild, DISABLED_RestagesSeparateCandidateBeforeEvents) {
    const char *enable = std::getenv("ENROTH_MM6_V6_INDOOR_READER_ENABLE");
    const bool writeAfterInput = enable &&
        std::string_view(enable) == "strict-indoor-v6-first-input-writer";
    const bool afterInputReader = enable &&
        std::string_view(enable) == "strict-indoor-v6-after-input-reader";
    const bool firstInputReader = enable &&
        (std::string_view(enable) == "strict-indoor-v6-first-input-reader" ||
         writeAfterInput);
    const bool liveSessionReader = enable &&
        (std::string_view(enable) == "strict-indoor-v6-live-session-reader" ||
         firstInputReader || afterInputReader);
    const bool postWorldReader = enable &&
        (std::string_view(enable) == "strict-indoor-v6-postworld-reader" ||
         liveSessionReader);
    if (!enable || (std::string_view(enable) != "strict-indoor-v6-reader" &&
                    !postWorldReader))
        GTEST_SKIP() << "Run after strict disposable v6 indoor writer";
    const std::string_view candidateSlot = afterInputReader ?
        mm6::testing::V6_INDOOR_AFTER_INPUT_PROBE_SLOT : liveSessionReader ?
        mm6::testing::V6_INDOOR_LIVE_SESSION_PROBE_SLOT :
        postWorldReader ? mm6::testing::V6_INDOOR_POSTWORLD_PROBE_SLOT :
                          mm6::testing::V6_INDOOR_PROBE_SLOT;
    const auto profile = mm6::testing::indoorProbeProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set an owner-only system-temp MM6 restart profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto before = mm6::testing::readIndoorProbeSlot(profile, candidateSlot);
    const auto seedBefore = mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6");
    auto decoded = std::unique_ptr<mm6::V6IndoorSaveFields>(
        new mm6::V6IndoorSaveFields(mm6::decodeV6IndoorSave(before, GameTitle::MM6)));
    const std::string mapName = decoded->raw.metadata.header.mapFileName;
    ASSERT_TRUE(mapName.ends_with(".blv"));
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const MapId map = pMapTable->GetMapInfo(mapName);
    ASSERT_NE(map, MAP_INVALID);
    const auto parsedBase = deserializeMM6IndoorBase(maps.readBase(mapName));
    const auto projectedBase = projectMM6IndoorBase(parsedBase);

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    pMapDeltas.clear();
    for (const auto &entry : decoded->raw.maps.deltas)
        pMapDeltas.emplace(entry.name, Blob::copy(entry.bytes.data(), entry.bytes.size()));

    bool completed = false;
    bool captured = false;
    bool aborted = false;
    std::vector<mm6::IndoorFaceState> preEventFaces;
    std::vector<std::int16_t> preEventDoorsData;
    std::vector<std::uint8_t> rebuiltDelta;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    game.runGameRoutine([&] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
        checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
            GameTitle::MM6,
            [&] {
                if (uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                    engine->_currentLoadedMapId != map || !pIndoor->bLoaded)
                    throw std::runtime_error("MM6 v6 reader checkpoint preceded indoor rebuild");
                preEventFaces = mm6::captureLiveIndoorFaceStates(
                    *pIndoor, projectedBase, GameTitle::MM6);
                preEventDoorsData = pIndoor->doorsData;
                rebuiltDelta = pIndoor->mm6DeltaBytes;
                captured = true;
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
    ASSERT_TRUE(captured);
    ASSERT_TRUE(aborted);
    ASSERT_FALSE(checkpoint->reachedMapLoad());
    ASSERT_FALSE(checkpoint->reachedInput());
    ASSERT_EQ(rebuiltDelta, decoded->raw.chest.rawActiveDelta);

    bool stageFinished = false;
    std::exception_ptr stageError;
    std::unique_ptr<mm6::StagedV6IndoorSaveFields> staged;
    game.runGameRoutine([&] {
        try {
            const auto freshNpcs = mm6::loadNpcStaticBaseline(
                *engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            const auto readFaces = [&mapName, &preEventFaces](std::string_view requested) {
                if (requested != mapName)
                    throw std::runtime_error("MM6 v6 reader requested another face source");
                return preEventFaces;
            };
            staged.reset(new mm6::StagedV6IndoorSaveFields(mm6::stageV6IndoorSave(
                before, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, *pDecorationTable,
                pOverlayTable->overlays.size(),
                pTileTable->tileCount(), readFaces, GameTitle::MM6)));
            if (liveSessionReader)
                mm6::testing::requireV6IndoorDetachedRawRoundTripForTest(
                    staged->saved.raw, staged->represented, *pItemTable,
                    pOverlayTable->overlays.size());
            (void) mm6::requireEventRuntimeMapEntry(
                staged->saved.eventRuntime, mapName,
                pIndoor->dlv.respawnCount, pIndoor->dlv.lastRespawnDay,
                pLevelDecorations, GameTitle::MM6);
            if (postWorldReader) {
                if (pNPCStats->uNewlNPCBufPos != 0 ||
                    std::any_of(pActors.begin(), pActors.end(),
                        [](const Actor &actor) { return actor.npcId >= 5000; }))
                    throw std::runtime_error(
                        "MM6 v6 first-load actor base already generated NPC links");
            } else {
                try {
                    mm6::requireGeneratedNpcActorAgreement(
                        staged->saved.generatedNpcs, pActors, mapName, GameTitle::MM6);
                } catch (const std::exception &error) {
                    throw std::runtime_error("MM6 v6 IART first-load GNPC join: " +
                                             std::string(error.what()));
                }
            }
            mm6::requireV6IndoorPreEventOwnerAgreement(
                *staged, mapName, pActors, pSpriteObjects, pLevelDecorations,
                GameTitle::MM6);
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
    EXPECT_FALSE(staged->represented.outdoorPlan);
    EXPECT_FALSE(staged->canEnterGameLoop());
    EXPECT_EQ(mm6::encodeV6IndoorSave(staged->saved, GameTitle::MM6), before);
    auto afterDecorations = pLevelDecorations;
    const auto event = std::find_if(staged->saved.eventRuntime.maps.begin(),
                                   staged->saved.eventRuntime.maps.end(),
        [&mapName](const mm6::EventRuntimeMapEntry &entry) {
            return entry.mapName == mapName;
        });
    ASSERT_NE(event, staged->saved.eventRuntime.maps.end());
    mm6::applyIndoorDecorationRuntimeAfterMapReload(
        staged->saved.runtimeDecorations, staged->represented.indoorPlan->base,
        *event, *pDecorationTable, &afterDecorations, GameTitle::MM6);
    ASSERT_FALSE(afterDecorations.empty());
    if (!postWorldReader)
        EXPECT_EQ(std::to_underlying(afterDecorations.front().uFlags),
                  (std::to_underlying(pLevelDecorations.front().uFlags) ^ 0x20u));
    EXPECT_EQ(afterDecorations.front().uDecorationDescID,
              staged->decorationReplay.front().uDecorationDescID);
    auto afterSprites = pSpriteObjects;
    mm6::applyIndoorSpriteRuntimeAfterMapReload(
        staged->saved.runtimeSprites, mapName, staged->saved.sprites,
        &afterSprites, GameTitle::MM6);
    ASSERT_EQ(afterSprites.size(), staged->spriteReplay.size());
    const auto detachedSprites = mm6::captureIndoorSpriteRuntime(
        mapName, staged->saved.sprites, afterSprites, GameTitle::MM6);
    EXPECT_EQ(mm6::encodeSpriteRuntimeSectionAs(
        detachedSprites, mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6),
        mm6::encodeSpriteRuntimeSectionAs(
        staged->saved.runtimeSprites, mm6::SpriteRuntimeSectionKind::IndoorV6,
        GameTitle::MM6));
    std::vector<Actor> afterActors(pActors.begin(), pActors.end());
    mm6::applyIndoorActorRuntimeToDetachedList(
        staged->saved.runtimeActors, mapName, staged->saved.actors,
        &afterActors, GameTitle::MM6);
    ASSERT_EQ(afterActors.size(), staged->actorReplay.size());
    const auto detachedActors = mm6::captureIndoorActorRuntime(
        mapName, staged->saved.actors, afterActors, GameTitle::MM6);
    EXPECT_EQ(mm6::encodeActorRuntimeSectionAs(
        detachedActors, mm6::ActorRuntimeSectionKind::IndoorV6, GameTitle::MM6),
        mm6::encodeActorRuntimeSectionAs(
        staged->saved.runtimeActors, mm6::ActorRuntimeSectionKind::IndoorV6,
        GameTitle::MM6));
    auto afterEvent = preEventFaces;
    mm6::applyIndoorFaceOverridesAfterMapReload(
        staged->saved.faceOverrides, mapName, &afterEvent, GameTitle::MM6);
    if (!postWorldReader)
        EXPECT_EQ(afterEvent.front().attributes, preEventFaces.front().attributes ^ 1u);
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, candidateSlot), before);
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), seedBefore);

    if (firstInputReader) {
        game.runGameRoutine([] { uGameState = GAME_FINISHED; });
        game.tick(2);
        ASSERT_EQ(GetCurrentMenuID(), MENU_MAIN);
        bool selected = false;
        bool postWorldChecked = false;
        bool firstInputReached = false;
        bool movementApplied = false;
        bool wroteAfterInput = false;
        std::exception_ptr postInputError;
        std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chest;
        std::unique_ptr<mm6::testing::ScopedEventRuntimeRestoreForTest> events;
        std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> preEvent;
        std::unique_ptr<mm6::testing::ScopedV6PreEventIndoorDecorationReplayForTest> decorations;
        std::unique_ptr<mm6::testing::ScopedV6PreEventIndoorSpriteReplayForTest> sprites;
        std::unique_ptr<mm6::testing::ScopedV6PreEventIndoorActorReplayForTest> actors;
        std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> admission;
        game.runGameRoutine([&] {
            checkpoint.reset();
            mm6::testing::setLoadGameOverride([&](std::string_view requested) {
                if (requested != "autosave.ec6")
                    throw std::runtime_error("MM6 v6 indoor input selected another menu route");
                *pParty = staged->represented.party.party;
                *pNPCStats = staged->represented.world.world.npcs;
                *gameTimer = staged->represented.world.visuals.timer;
                *pActiveOverlayList = staged->represented.world.visuals.overlays;
                pMapDeltas.clear();
                for (const auto &[name, blob] : staged->represented.world.world.mapDeltas)
                    pMapDeltas.emplace(name, Blob::copy(blob));
                mm6::chestTitleState() = staged->represented.world.chestTitle;
                mm6::publishFreshChildPersistentEventZeros(
                    &engine->_persistentVariables,
                    staged->represented.persistentEvents, GameTitle::MM6);
                mm6::publishRawCursorShopOwners(
                    &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                    staged->represented.raw.saved,
                    {staged->represented.raw.cursor, staged->represented.raw.shops},
                    GameTitle::MM6);
                engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
                engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
                chest = std::make_unique<mm6::ScopedChestMapLoadOptIn>(true);
                events = std::make_unique<mm6::testing::ScopedEventRuntimeRestoreForTest>(
                    GameTitle::MM6, staged->saved.eventRuntime);
                preEvent = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
                    GameTitle::MM6, [&] {
                        mm6::requireV6IndoorPreEventOwnerAgreement(
                            *staged, mapName, pActors, pSpriteObjects,
                            pLevelDecorations, GameTitle::MM6);
                    });
                decorations = std::make_unique<
                    mm6::testing::ScopedV6PreEventIndoorDecorationReplayForTest>(
                        GameTitle::MM6, mapName, staged->decorationReplay);
                sprites = std::make_unique<
                    mm6::testing::ScopedV6PreEventIndoorSpriteReplayForTest>(
                        GameTitle::MM6, staged->saved.runtimeSprites,
                        staged->saved.sprites);
                actors = std::make_unique<
                    mm6::testing::ScopedV6PreEventIndoorActorReplayForTest>(
                        GameTitle::MM6, staged->saved.runtimeActors,
                        staged->saved.actors, staged->saved.generatedNpcs,
                        staged->saved.gameplayRng, true);
                admission = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                    GameTitle::MM6, [&] {
                        if (!preEvent->reachedMapLoad() || preEvent->reachedInput() ||
                            !events->restored() || !decorations->published() ||
                            !sprites->published() || !actors->published() ||
                            !actors->generatedBypassConsumed() || !actors->rngPublished() ||
                            engine->_currentLoadedMapId != map ||
                            uCurrentlyLoadedLevelType != LEVEL_INDOOR ||
                            !pIndoor->bLoaded || !gameTimer->isPaused() ||
                            engine->_pendingTransition || pPartyActionQueue->uNumActions != 0)
                            throw std::runtime_error("MM6 v6 indoor input reached unexpected world order");
                        mm6::testing::requireV6IndoorNativeResidualsForTest(
                            parsedBase, preEventDoorsData);
                        const auto raw = mm6::testing::captureV6IndoorLiveRawOwnersForTest(
                            staged->saved.raw, mapName);
                        mm6::testing::requireV6IndoorRawSectionsEqualForTest(
                            *raw, staged->saved.raw, "MM6 v6 indoor pre-input raw owner");
                        mm6::requireGeneratedNpcActorAgreement(
                            staged->saved.generatedNpcs, pActors, mapName, GameTitle::MM6);
                        const auto generated = mm6::captureGeneratedNpcFields(
                            pActors, *pNPCStats, mapName, GameTitle::MM6);
                        if (mm6::encodeGeneratedNpcSection(generated, GameTitle::MM6).payload !=
                            mm6::encodeGeneratedNpcSection(
                                staged->saved.generatedNpcs, GameTitle::MM6).payload)
                            throw std::runtime_error("MM6 v6 indoor pre-input GNPC differs");
                        const auto rng = mm6::captureGameplayRngFields(
                            *application->component<EngineRandomComponent>(), GameTitle::MM6);
                        if (mm6::encodeGameplayRngSection(rng, GameTitle::MM6).payload !=
                            mm6::encodeGameplayRngSection(
                                staged->saved.gameplayRng, GameTitle::MM6).payload)
                            throw std::runtime_error("MM6 v6 indoor pre-input RNG differs");
                        const auto decorationRuntime = mm6::captureIndoorDecorationRuntime(
                            mapName, staged->represented.indoorPlan->base, *event,
                            *pDecorationTable, pLevelDecorations, GameTitle::MM6);
                        if (decorationRuntime != staged->saved.runtimeDecorations)
                            throw std::runtime_error("MM6 v6 indoor pre-input IDRT differs");
                        const auto spriteRuntime = mm6::captureIndoorSpriteRuntime(
                            mapName, staged->saved.sprites, pSpriteObjects, GameTitle::MM6);
                        if (mm6::encodeSpriteRuntimeSectionAs(
                                spriteRuntime, mm6::SpriteRuntimeSectionKind::IndoorV6,
                                GameTitle::MM6) !=
                            mm6::encodeSpriteRuntimeSectionAs(
                                staged->saved.runtimeSprites,
                                mm6::SpriteRuntimeSectionKind::IndoorV6,
                                GameTitle::MM6))
                            throw std::runtime_error("MM6 v6 indoor pre-input ISRT differs");
                        const std::vector<Actor> liveActors(pActors.begin(), pActors.end());
                        const auto actorRuntime = mm6::captureIndoorActorRuntime(
                            mapName, staged->saved.actors, liveActors, GameTitle::MM6);
                        if (mm6::encodeActorRuntimeSectionAs(
                                actorRuntime, mm6::ActorRuntimeSectionKind::IndoorV6,
                                GameTitle::MM6) !=
                            mm6::encodeActorRuntimeSectionAs(
                                staged->saved.runtimeActors,
                                mm6::ActorRuntimeSectionKind::IndoorV6,
                                GameTitle::MM6))
                            throw std::runtime_error("MM6 v6 indoor pre-input IART differs");
                        postWorldChecked = true;
                    });
                selected = true;
            });
            mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
            engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
        });
        game.skipLoadingScreen();
        game.tick(3);
        if (postWorldChecked) {
            game.runGameRoutine([&] {
                firstInputReached = preEvent->reachedInput() &&
                    !gameTimer->isPaused() &&
                    !(engineFlags & ENGINE_SKIP_NEXT_WORLD_UPDATE) &&
                    !(engineFlags & ENGINE_SKIP_NEXT_USER_INPUT) &&
                    engine->_currentLoadedMapId == map &&
                    uCurrentlyLoadedLevelType == LEVEL_INDOOR &&
                    !engine->_pendingTransition && pPartyActionQueue->uNumActions == 0;
                if (firstInputReached) {
                    pPartyActionQueue->Add(PARTY_TurnLeft);
                    pPartyActionQueue->Add(PARTY_WalkForward);
                }
            });
            if (firstInputReached) {
                const auto beforePosition = pParty->pos;
                const auto beforeYaw = pParty->_viewYaw;
                game.tick(3);
                game.runGameRoutine([&] {
                    movementApplied = pParty->pos != beforePosition &&
                        pParty->_viewYaw != beforeYaw &&
                        pPartyActionQueue->uNumActions == 0 &&
                        engine->_currentLoadedMapId == map &&
                        !engine->_pendingTransition;
                    if (movementApplied && writeAfterInput) {
                        try {
                            mm6::testing::requireV6IndoorNativeResidualsForTest(
                                parsedBase, preEventDoorsData);
                            auto saved = std::make_unique<mm6::V6IndoorSaveFields>(
                                staged->saved);
                            saved->raw = *mm6::testing::captureV6IndoorLiveRawOwnersForTest(
                                staged->saved.raw, mapName);
                            saved->generatedNpcs = mm6::captureGeneratedNpcFields(
                                pActors, *pNPCStats, mapName, GameTitle::MM6);
                            saved->gameplayRng = mm6::captureGameplayRngFields(
                                *application->component<EngineRandomComponent>(),
                                GameTitle::MM6);
                            auto active = std::find_if(saved->eventRuntime.maps.begin(),
                                saved->eventRuntime.maps.end(),
                                [&mapName](const mm6::EventRuntimeMapEntry &entry) {
                                    return entry.mapName == mapName;
                                });
                            if (active == saved->eventRuntime.maps.end())
                                throw std::runtime_error("MM6 v6 after-input EVRT map is missing");
                            active->mapVars = engine->_persistentVariables.mapVars;
                            active->decorVars = engine->_persistentVariables.decorVars;
                            active->bindings = mm6::captureEventRuntimeDecorationBindings(
                                pLevelDecorations, GameTitle::MM6);
                            saved->runtimeDecorations = mm6::captureIndoorDecorationRuntime(
                                mapName, staged->represented.indoorPlan->base, *active,
                                *pDecorationTable, pLevelDecorations, GameTitle::MM6);
                            saved->runtimeSprites = mm6::captureIndoorSpriteRuntime(
                                mapName, saved->sprites, pSpriteObjects, GameTitle::MM6);
                            const std::vector<Actor> liveActors(pActors.begin(), pActors.end());
                            saved->runtimeActors = mm6::captureIndoorActorRuntime(
                                mapName, saved->actors, liveActors, GameTitle::MM6);
                            const auto liveFaces = mm6::captureLiveIndoorFaceStates(
                                *pIndoor, projectedBase, GameTitle::MM6);
                            saved->faceOverrides.maps.clear();
                            if (auto face = mm6::captureIndoorFaceOverrides(
                                    mapName, preEventFaces, liveFaces, GameTitle::MM6))
                                saved->faceOverrides.maps.push_back(std::move(*face));
                            const auto candidate = mm6::encodeV6IndoorSave(
                                *saved, GameTitle::MM6);
                            const auto freshNpcs = mm6::loadNpcStaticBaseline(
                                *engine->resources(), GameTitle::MM6);
                            const auto verifyItem = [](std::int32_t id) {
                                return id >= 1 && id <= 580 &&
                                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
                            };
                            const auto readFaces = [&mapName, &preEventFaces](
                                std::string_view requested) {
                                if (requested != mapName)
                                    throw std::runtime_error(
                                        "MM6 v6 after-input requested another face source");
                                return preEventFaces;
                            };
                            auto restaged = std::unique_ptr<mm6::StagedV6IndoorSaveFields>(
                                new mm6::StagedV6IndoorSaveFields(mm6::stageV6IndoorSave(
                                    candidate, freshNpcs, maps.expectedNames(),
                                    [&maps](std::string_view name) { return maps.readBase(name); },
                                    verifyItem, *pItemTable, *pDecorationTable,
                                    pOverlayTable->overlays.size(), pTileTable->tileCount(),
                                    readFaces, GameTitle::MM6)));
                            mm6::testing::requireV6IndoorDetachedRawRoundTripForTest(
                                restaged->saved.raw, restaged->represented, *pItemTable,
                                pOverlayTable->overlays.size());
                            if (restaged->canEnterGameLoop() ||
                                mm6::encodeV6IndoorSave(restaged->saved, GameTitle::MM6) !=
                                    candidate ||
                                mm6::testing::readIndoorProbeSlot(profile, candidateSlot) != before ||
                                mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6") != seedBefore)
                                throw std::runtime_error(
                                    "MM6 v6 after-input detached transaction changed source");
                            mm6::testing::writeIndoorProbeCandidate(
                                profile, candidate,
                                mm6::testing::V6_INDOOR_AFTER_INPUT_PROBE_SLOT);
                            wroteAfterInput = true;
                        } catch (...) {
                            postInputError = std::current_exception();
                        }
                    }
                });
            }
            game.runGameRoutine([] { uGameState = GAME_STATE_GAME_QUITTING_TO_MAIN_MENU; });
            game.tick(2);
        }
        game.runGameRoutine([&] {
            admission.reset();
            decorations.reset();
            sprites.reset();
            actors.reset();
            preEvent.reset();
            events.reset();
            chest.reset();
            mm6::testing::setMainMenuAutosaveQuickLoadOverride(false);
            mm6::testing::setLoadGameOverride({});
        });
        EXPECT_TRUE(selected);
        EXPECT_TRUE(postWorldChecked);
        EXPECT_TRUE(firstInputReached);
        EXPECT_TRUE(movementApplied);
        if (postInputError)
            std::rethrow_exception(postInputError);
        if (writeAfterInput)
            EXPECT_TRUE(wroteAfterInput);
        EXPECT_FALSE(staged->canEnterGameLoop());
        EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, candidateSlot), before);
        EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), seedBefore);
        if (writeAfterInput)
            EXPECT_FALSE(mm6::testing::readIndoorProbeSlot(
                profile, mm6::testing::V6_INDOOR_AFTER_INPUT_PROBE_SLOT).empty());
        EXPECT_EQ(window->nativeHandle(), nullptr);
        return;
    }

    // Rebuild from the same immutable candidate a second time. The first
    // preparation only observed the pre-event face baseline; this one crosses
    // onMapLoad with a test-only indoor IDRT publication. No input is queued.
    pMapDeltas.clear();
    if (liveSessionReader) {
        for (const auto &[name, blob] : staged->represented.world.world.mapDeltas)
            pMapDeltas.emplace(name, Blob::copy(blob));
    } else {
        for (const auto &entry : decoded->raw.maps.deltas)
            pMapDeltas.emplace(entry.name,
                Blob::copy(entry.bytes.data(), entry.bytes.size()));
    }
    bool worldFinished = false;
    std::exception_ptr worldError;
    std::unique_ptr<mm6::testing::ScopedEventRuntimeRestoreForTest> eventRestore;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> worldCheckpoint;
    std::unique_ptr<mm6::testing::ScopedV6PreEventIndoorDecorationReplayForTest> liveReplay;
    std::unique_ptr<mm6::testing::ScopedV6PreEventIndoorSpriteReplayForTest> liveSpriteReplay;
    std::unique_ptr<mm6::testing::ScopedV6PreEventIndoorActorReplayForTest> liveActorReplay;
    game.runGameRoutine([&] {
        try {
            checkpoint.reset(); // Tear down its game-thread-local scope here.
            *pParty = staged->represented.party.party;
            mm6::chestTitleState() = staged->represented.world.chestTitle;
            if (liveSessionReader) {
                *pNPCStats = staged->represented.world.world.npcs;
                *gameTimer = staged->represented.world.visuals.timer;
                *pActiveOverlayList = staged->represented.world.visuals.overlays;
                mm6::publishFreshChildPersistentEventZeros(
                    &engine->_persistentVariables,
                    staged->represented.persistentEvents, GameTitle::MM6);
                mm6::publishRawCursorShopOwners(
                    &mm6::rawHeldCursorState(GameTitle::MM6),
                    &mm6::shopTitleState(), staged->represented.raw.saved,
                    {staged->represented.raw.cursor,
                     staged->represented.raw.shops}, GameTitle::MM6);
            }
            engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            eventRestore = std::make_unique<mm6::testing::ScopedEventRuntimeRestoreForTest>(
                GameTitle::MM6, staged->saved.eventRuntime);
            worldCheckpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
                GameTitle::MM6, [&] {
                    mm6::requireV6IndoorPreEventOwnerAgreement(
                        *staged, mapName, pActors, pSpriteObjects, pLevelDecorations,
                        GameTitle::MM6);
                });
            liveReplay = std::make_unique<
                mm6::testing::ScopedV6PreEventIndoorDecorationReplayForTest>(
                    GameTitle::MM6, mapName, staged->decorationReplay);
            liveSpriteReplay = std::make_unique<
                mm6::testing::ScopedV6PreEventIndoorSpriteReplayForTest>(
                    GameTitle::MM6, staged->saved.runtimeSprites,
                    staged->saved.sprites);
            liveActorReplay = std::make_unique<
                mm6::testing::ScopedV6PreEventIndoorActorReplayForTest>(
                    GameTitle::MM6, staged->saved.runtimeActors,
                    staged->saved.actors, staged->saved.generatedNpcs,
                    staged->saved.gameplayRng, postWorldReader);
            mm6::ScopedChestMapLoadOptIn chestOptIn(true);
            DoPrepareWorld(true, 1);
            if (!worldCheckpoint->reachedMapLoad() || worldCheckpoint->reachedInput() ||
                !eventRestore->restored() || !liveReplay->published() ||
                !liveSpriteReplay->published() || !liveActorReplay->published() ||
                (postWorldReader &&
                 (!liveActorReplay->generatedBypassConsumed() ||
                  !liveActorReplay->rngPublished())) ||
                engine->_currentLoadedMapId != map ||
                uCurrentlyLoadedLevelType != LEVEL_INDOOR || !pIndoor->bLoaded)
                throw std::runtime_error("MM6 v6 IDRT did not cross the post-world boundary");
            const auto observed = mm6::captureIndoorDecorationRuntime(
                mapName, staged->represented.indoorPlan->base, *event,
                *pDecorationTable, pLevelDecorations, GameTitle::MM6);
            if (observed != staged->saved.runtimeDecorations)
                throw std::runtime_error("MM6 v6 IDRT changed after map reload events");
            const auto observedSprites = mm6::captureIndoorSpriteRuntime(
                mapName, staged->saved.sprites, pSpriteObjects, GameTitle::MM6);
            if (mm6::encodeSpriteRuntimeSectionAs(
                    observedSprites, mm6::SpriteRuntimeSectionKind::IndoorV6,
                    GameTitle::MM6) !=
                mm6::encodeSpriteRuntimeSectionAs(
                    staged->saved.runtimeSprites,
                    mm6::SpriteRuntimeSectionKind::IndoorV6, GameTitle::MM6))
                throw std::runtime_error("MM6 v6 ISRT changed after map reload events");
            if (postWorldReader) {
                if (liveSessionReader) {
                    mm6::testing::requireV6IndoorNativeResidualsForTest(
                        parsedBase, preEventDoorsData);
                    const auto observedRaw = mm6::testing::captureV6IndoorLiveRawOwnersForTest(
                        staged->saved.raw, mapName);
                    mm6::testing::requireV6IndoorRawSectionsEqualForTest(
                        *observedRaw, staged->saved.raw,
                        "MM6 v6 post-world raw owner");
                }
                try {
                    mm6::requireGeneratedNpcActorAgreement(
                        staged->saved.generatedNpcs, pActors, mapName, GameTitle::MM6);
                } catch (const std::exception &error) {
                    throw std::runtime_error("MM6 v6 IART post-world GNPC join: " +
                                             std::string(error.what()));
                }
                const auto liveGenerated = mm6::captureGeneratedNpcFields(
                    pActors, *pNPCStats, mapName, GameTitle::MM6);
                if (mm6::encodeGeneratedNpcSection(liveGenerated, GameTitle::MM6).payload !=
                    mm6::encodeGeneratedNpcSection(
                        staged->saved.generatedNpcs, GameTitle::MM6).payload)
                    throw std::runtime_error(
                        "MM6 v6 post-world generated NPC records differ");
                const auto liveRng = mm6::captureGameplayRngFields(
                    *application->component<EngineRandomComponent>(), GameTitle::MM6);
                if (mm6::encodeGameplayRngSection(liveRng, GameTitle::MM6).payload !=
                    mm6::encodeGameplayRngSection(
                        staged->saved.gameplayRng, GameTitle::MM6).payload)
                    throw std::runtime_error("MM6 v6 post-world gameplay RNG differs");
                const std::vector<Actor> finalActors(pActors.begin(), pActors.end());
                const auto observedActors = mm6::captureIndoorActorRuntime(
                    mapName, staged->saved.actors, finalActors, GameTitle::MM6);
                if (mm6::encodeActorRuntimeSectionAs(
                        observedActors, mm6::ActorRuntimeSectionKind::IndoorV6,
                        GameTitle::MM6) !=
                    mm6::encodeActorRuntimeSectionAs(
                        staged->saved.runtimeActors,
                        mm6::ActorRuntimeSectionKind::IndoorV6, GameTitle::MM6))
                    throw std::runtime_error("MM6 v6 IART changed after map reload events");
            }
        } catch (...) {
            worldError = std::current_exception();
        }
        liveReplay.reset();
        liveSpriteReplay.reset();
        liveActorReplay.reset();
        worldCheckpoint.reset();
        eventRestore.reset();
        worldFinished = true;
    });
    for (int tick = 0; tick < 128 && !worldFinished; ++tick)
        game.tick();
    ASSERT_TRUE(worldFinished);
    if (worldError)
        std::rethrow_exception(worldError);
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, candidateSlot), before);
    EXPECT_EQ(mm6::testing::readIndoorProbeSlot(profile, "autosave.ec6"), seedBefore);
}
