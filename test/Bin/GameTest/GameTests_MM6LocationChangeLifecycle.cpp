#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6EventRuntimeRestoreForTest.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6PostWorldAdmissionForTest.h"
#include "Engine/MM6/MM6PostRebuildPidSlots.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/MM6/MM6V3FullSessionPreflight.h"
#include "Engine/MM6/MM6V3PreEventReplayForTest.h"
#include "Engine/MM6/MM6V4FullSessionPreflight.h"
#include "Engine/MM6/MM6V4OutdoorDecorationReplay.h"
#include "Engine/MM6/MM6V4SaveFields.h"
#include "Engine/MM6/MM6V5FullSessionPreflight.h"
#include "Engine/MM6/MM6V5LiveSaveCapture.h"
#include "Engine/MM6/MM6V6OutdoorSaveFields.h"
#include "Engine/MM6/MM6VisitedOutdoorRuntimeOwner.h"
#include "Engine/MM6/MM6VisitedOutdoorReturnPlan.h"
#include "Engine/MM6/MM6VisitedReturnPreEventProbeForTest.h"
#include "Engine/MM6/MM6TransitionAutosaveInterceptForTest.h"
#include "Engine/MM6/MM6TransitionPostWorldCaptureForTest.h"
#include "Engine/MM6/MM6V5OutdoorDecorationSource.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/MM6/MM6ChestRuntimeProjection.h"
#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"
#include "Engine/MM6/MM6SavedActorPidPolicy.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/GUIWindow.h"
#include "Io/Mouse.h"
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6V5LaterOutdoorCaptureForTest.h"

namespace {

std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 v4 replay needs an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 v4 replay needs an owner-only system-temp profile");
    return root;
}

std::filesystem::path v4CandidateInProfile(const std::filesystem::path &profile) {
    const std::filesystem::path supplied = profile / "saves" / "candidate-v4.ec6";
    if (!std::filesystem::is_regular_file(supplied))
        throw std::runtime_error("MM6 v4 replay needs an existing absolute candidate");
    const std::filesystem::path candidate = std::filesystem::canonical(supplied);
    if (candidate.parent_path() != std::filesystem::canonical(profile / "saves") ||
        candidate.filename() != "candidate-v4.ec6")
        throw std::runtime_error("MM6 v4 replay candidate must be a separate profile save");
    return candidate;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    constexpr std::uintmax_t maxSaveBytes = 64 * 1024 * 1024;
    if (std::filesystem::file_size(path) > maxSaveBytes)
        throw std::runtime_error("MM6 v4 replay candidate exceeds the EC6 bound");
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::map<std::string, std::vector<std::uint8_t>> profileSaveBytes(
    const std::filesystem::path &profile) {
    std::map<std::string, std::vector<std::uint8_t>> files;
    for (const auto &entry : std::filesystem::directory_iterator(profile / "saves")) {
        if (!std::filesystem::is_regular_file(entry.symlink_status()))
            throw std::runtime_error("MM6 transition profile has a nonregular save entry");
        files.emplace(entry.path().filename().string(), slotBytes(entry.path()));
    }
    return files;
}

std::vector<LevelDecoration> sourceDecorations(const mm6::LoadedMapSource &maps,
                                               std::string_view mapName) {
    const Blob baseBytes = maps.readBase(mapName);
    auto source = std::make_unique<OutdoorLocation_MM7>();
    deserializeMM6Outdoor(baseBytes, source.get());
    std::vector<LevelDecoration> result(source->decorations.size());
    if (source->decorationNames.size() != result.size())
        throw std::runtime_error("MM6 v4 base decorations lack source names");
    for (std::size_t i = 0; i < result.size(); ++i) {
        reconstruct(source->decorations[i], &result[i]);
        const auto &name = source->decorationNames[i];
        const auto end = std::find(name.begin(), name.end(), '\0');
        result[i].uDecorationDescID = mm6::requireMM6OutdoorSourceDecorationId(
            source->decorations[i].uDecorationDescID,
            std::string_view(name.data(), end - name.begin()), *pDecorationTable);
    }
    return result;
}

} // namespace

// This test enters an accepted first-map session, then drives the real
// location-change state machine with one exact autosave interception. It is
// a lifecycle probe, not border travel or a production save path.
void runMM6LocationChangeLifecycle(EngineController &game, bool writeV6,
                                   bool probeReturn = false) {
    const char *enable = std::getenv(probeReturn ?
        "ENROTH_MM6_VISITED_RETURN_PROBE_ENABLE" :
        writeV6 ? "ENROTH_MM6_V6_OUTDOOR_WRITE_ENABLE" :
                  "ENROTH_MM6_LOCATION_CHANGE_ENABLE");
    if (!enable || std::string_view(enable) !=
            (probeReturn ? "strict-visited-return-probe" :
             writeV6 ? "strict-v6-outdoor-write" :
                       "strict-mm6-location-change"))
        GTEST_SKIP() << "Run only after strict v4 writer and owner admission";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    const std::filesystem::path candidate = v4CandidateInProfile(profile);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto before = slotBytes(candidate);
    const auto seedBefore = slotBytes(profile / "saves" / "autosave.ec6");
    const auto v5Path = profile / "saves" / "candidate-v5-oute2.ec6";
    if (writeV6 && !std::filesystem::is_regular_file(v5Path))
        throw std::runtime_error(
            "MM6 outdoor v6 writer needs a prior v5 control slot");
    const auto v5Before = writeV6 ? slotBytes(v5Path) :
        std::vector<std::uint8_t>{};
    const auto allSavesBefore = profileSaveBytes(profile);
    ASSERT_FALSE(before.empty());
    ASSERT_FALSE(seedBefore.empty());
    if (writeV6)
        ASSERT_FALSE(v5Before.empty());

    bool selected = false;
    bool checkpointRan = false;
    bool freshMapBeforeReplay = false;
    bool postWorldRan = false;
    bool finalOwnersAgree = false;
    bool firstInputReached = false;
    bool transitionQueued = false;
    bool autosaveIntercepted = false;
    bool targetMapReady = false;
    bool movementApplied = false;
    bool sourceHadChestPosition = false;
    bool visitedOwnerStaged = false;
    bool visitedOwnerSurvivedTransition = false;
    bool visitedFaultsRefused = false;
    bool visitedV6RoundTrip = false;
    bool visitedTwoMapSectionRoundTrip = false;
    bool visitedV6FaultsRefused = false;
    bool targetPreInputCaptured = false;
    bool v6CandidateWritten = false;
    bool v6AtomicFaultRefused = false;
    bool returnTransitionQueued = false;
    bool returnProbeReached = false;
    bool returnPreEventAgreed = false;
    std::vector<std::uint8_t> v6CandidateBytes;
    int sourceGeneratedCursor = -1;
    std::vector<std::uint8_t> oldMapCacheBefore;
    std::vector<std::uint8_t> targetV5BeforeInput;
    std::string finalOwnerError;
    std::size_t expectedNativeActors = 0;
    std::size_t expectedActiveActors = 0;
    mm6::GeneratedNpcSaveFields expectedGenerated;
    std::unique_ptr<mm6::V4FullSessionPreflight> staged;
    std::unique_ptr<mm6::VisitedOutdoorRuntimeOwner> visitedOwner;
    std::unique_ptr<mm6::V6OutdoorSaveFields> returnSaved;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedEventRuntimeRestoreForTest> eventRuntime;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    std::unique_ptr<mm6::testing::ScopedV3PreEventReplayForTest> replay;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> postWorld;
    std::unique_ptr<mm6::testing::ScopedTransitionPostWorldCaptureForTest>
        targetCapture;
    std::unique_ptr<mm6::testing::ScopedVisitedReturnPreEventProbeForTest>
        returnProbe;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view requested) {
            if (requested != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 v4 replay selected another menu route");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto rebuiltDecorations = sourceDecorations(maps, "oute3.odm");
            auto candidateStage = mm6::stageV4FullSessionPreflight(
                before, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), engine->config->gameplay.MaxActors.value(),
                application->component<EngineRandomComponent>()->type(),
                rebuiltDecorations, GameTitle::MM6);
            if (candidateStage.v3.actorMap.activeMap.mapName != "oute3.odm" ||
                mm6::encodeV4Save(candidateStage.saved, GameTitle::MM6) != before ||
                candidateStage.v3.armageddon.timerTicks != 0 ||
                candidateStage.v3.cursorImage.occupied)
                throw std::runtime_error("MM6 v4 replay has no exact first-map candidate");
            staged = std::make_unique<mm6::V4FullSessionPreflight>(std::move(candidateStage));
            expectedNativeActors = staged->v3.actorMap.activeMap.delta.actors.size();
            expectedActiveActors = staged->v3.actorMap.activeActors.size();
            expectedGenerated = staged->saved.v3.generatedNpcs;

            *pParty = staged->v3.represented.party.party;
            *pNPCStats = staged->v3.represented.world.world.npcs;
            *gameTimer = staged->v3.represented.world.visuals.timer;
            *pActiveOverlayList = staged->v3.represented.world.visuals.overlays;
            pMapDeltas = std::move(staged->v3.represented.world.world.mapDeltas);
            mm6::chestTitleState() = staged->v3.represented.world.chestTitle;
            mm6::publishFreshChildPersistentEventZeros(
                &engine->_persistentVariables,
                staged->v3.represented.persistentEvents, GameTitle::MM6);
            mm6::publishRawCursorShopOwners(
                &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                staged->v3.represented.raw.saved,
                {std::move(staged->v3.represented.raw.cursor),
                 std::move(staged->v3.represented.raw.shops)}, GameTitle::MM6);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(true);
            eventRuntime = std::make_unique<mm6::testing::ScopedEventRuntimeRestoreForTest>(
                GameTitle::MM6, staged->saved.eventRuntime);
            checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
                GameTitle::MM6, [&] {
                    checkpointRan = true;
                    freshMapBeforeReplay = gameTimer->isPaused() &&
                        engine->_pendingTransition &&
                        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                        uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                        pActors.size() == expectedNativeActors &&
                        pNPCStats->uNewlNPCBufPos == 0 &&
                        pPartyActionQueue->uNumActions == 0;
                });
            replay = std::make_unique<mm6::testing::ScopedV3PreEventReplayForTest>(
                GameTitle::MM6, std::move(staged->v3.actorMap));
            postWorld = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                GameTitle::MM6, [&] {
                    postWorldRan = true;
                    try {
                        if (!checkpoint->reachedMapLoad() || checkpoint->reachedInput() ||
                            !replay->published() || !replay->rngPublished() ||
                            !eventRuntime->restored() ||
                            pActors.size() != expectedActiveActors ||
                            expectedActiveActors < expectedNativeActors ||
                            engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute3.odm") ||
                            uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
                            !gameTimer->isPaused() || engine->_pendingTransition ||
                            pPartyActionQueue->uNumActions != 0)
                            throw std::runtime_error("MM6 v4 replay reached an unexpected world order");
                        mm6::requireGeneratedNpcActorAgreement(
                            expectedGenerated, pActors, "oute3.odm", GameTitle::MM6);
                        const auto observedGenerated = mm6::captureGeneratedNpcFields(
                            pActors, *pNPCStats, "oute3.odm", GameTitle::MM6);
                        if (mm6::encodeGeneratedNpcSection(observedGenerated, GameTitle::MM6) !=
                            mm6::encodeGeneratedNpcSection(expectedGenerated, GameTitle::MM6))
                            throw std::runtime_error("MM6 v4 generated NPC rows changed after map events");
                        const std::vector<Actor> finalActors(pActors.begin(), pActors.end());
                        if (mm6::encodeActiveActorSection(finalActors, GameTitle::MM6) !=
                            staged->saved.v3.activeActorPayload)
                            throw std::runtime_error("MM6 v4 ACTR changed after map events");
                        if (engine->_persistentVariables.mapVars != staged->activeEventRuntime.mapVars ||
                            engine->_persistentVariables.decorVars != staged->activeEventRuntime.decorVars)
                            throw std::runtime_error("MM6 v4 EVRT arrays changed after map events");
                        const mm6::LoadedMapSource loadedMaps(*pGames_LOD, *pMapTable,
                                                               GameTitle::MM6);
                        mm6::requireFirstMapOutdoorDecorationReplay(
                            "oute3.odm", loadedMaps.readBase("oute3.odm"),
                            staged->activeEventRuntime, *pDecorationTable,
                            pLevelDecorations, GameTitle::MM6);
                        const auto outdoorWire = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
                        const auto nativeBytes = outdoorWire.part(mm6::OutdoorDeltaPart::PersistentBytes);
                        const auto expectedNative = mm6::capturePersistentEventBoundary(
                            staged->v3.represented.persistentEvents, GameTitle::MM6);
                        if (nativeBytes.size() != expectedNative.size() ||
                            !std::equal(nativeBytes.begin(), nativeBytes.end(),
                                        expectedNative.begin(), expectedNative.end()))
                            throw std::runtime_error("MM6 v4 native event bytes changed after map events");
                        const auto &savedRaw = staged->saved.v3.raw;
                        if (pMapTable->pInfos[engine->_currentLoadedMapId].fileName !=
                                savedRaw.metadata.header.mapFileName ||
                            !staged->v3.represented.outdoorPlan ||
                            staged->v3.represented.outdoorPlan->mapName != "oute3.odm")
                            throw std::runtime_error("MM6 v4 final map differs from HEAD");
                        mm6::requireNoActiveSavedActorOverlays(
                            savedRaw.metadata.overlays, GameTitle::MM6);
                        mm6::requirePostRebuildPidSlotAgreement(
                            *staged->v3.represented.outdoorPlan, "oute3.odm",
                            std::span<const Actor>(finalActors).first(expectedNativeActors),
                            pSpriteObjects, GameTitle::MM6);
                        const auto &chest = mm6::chestTitleState();
                        if (!chest.ready || chest.lcgState != savedRaw.chest.lcgState ||
                            chest.generatedArtifactFlags != savedRaw.chest.generatedArtifactFlags ||
                            pOutdoor->mm6DeltaBytes != savedRaw.chest.rawActiveDelta ||
                            vChests.size() != 20)
                            throw std::runtime_error("MM6 v4 final CHST owner changed");
                        mm6::ChestRecordSet chestRecords{};
                        if (!mm6::copyChestRecords(outdoorWire.part(mm6::OutdoorDeltaPart::Chests),
                                                   &chestRecords))
                            throw std::runtime_error("MM6 v4 final chest section is incomplete");
                        for (int index = 0; index < 20; ++index) {
                            mm6::ChestRuntimeProjection projection{vChests[index], chestRecords[index]};
                            const auto replayed = mm6::stageChestRuntimeRecord(
                                GameTitle::MM6, projection);
                            if (!replayed)
                                throw std::runtime_error("MM6 v4 final chest projection refused at index " +
                                                         std::to_string(index));
                            if (*replayed != chestRecords[index])
                                throw std::runtime_error("MM6 v4 final chest projection differed at index " +
                                                         std::to_string(index));
                        }
                        if (mm6::captureCoreFields(*pParty, GameTitle::MM6) != savedRaw.core)
                            throw std::runtime_error("MM6 v4 PART/CHAR owner changed after map events");
                        const auto durable = mm6::captureDurableFields(*pParty, GameTitle::MM6);
                        if (durable.bits != savedRaw.bits || durable.effects != savedRaw.effects)
                            throw std::runtime_error("MM6 v4 QEST/EFFT owner changed after map events");
                        if (mm6::captureResistanceFields(*pParty, GameTitle::MM6) != savedRaw.resistances ||
                            mm6::captureBuffFields(*pParty, GameTitle::MM6) != savedRaw.buffs ||
                            mm6::captureRecoveryFields(*pParty, GameTitle::MM6) != savedRaw.recovery)
                            throw std::runtime_error("MM6 v4 Party resistance, buff or recovery owner changed");
                        if (mm6::encodeHirelingSection(
                                mm6::captureHirelingFields(*pParty, GameTitle::MM6)) !=
                            mm6::encodeHirelingSection(savedRaw.hirelings))
                            throw std::runtime_error("MM6 v4 HIRE owner changed after map events");
                        mm6::StagedRawPartyInventories liveInventories{*pParty, savedRaw.inventory};
                        if (mm6::captureRawPartyInventories(liveInventories, *pItemTable,
                                                             GameTitle::MM6) != savedRaw.inventory)
                            throw std::runtime_error("MM6 v4 INVR owner changed after map events");
                        mm6::requirePostWorldEmptyCursorReady(
                            staged->v3.cursorImage, *pParty, *mouse,
                            engine->config->graphics.AlwaysCustomCursor.value(),
                            GameTitle::MM6);
                        mm6::requirePostWorldArmageddonResumeAgreement(
                            *pParty, staged->v3.armageddon, LEVEL_OUTDOOR,
                            GameTitle::MM6);
                        finalOwnersAgree = true;
                    } catch (const std::exception &error) {
                        finalOwnerError = error.what();
                    }
                    if (!finalOwnersAgree)
                        throw std::runtime_error("MM6 location-change source admission refused: " +
                                                 finalOwnerError);
                }, mm6::testing::PostWorldAdmissionResult::Continue);
            selected = true;
        });
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });

    game.skipLoadingScreen();
    game.tick(3);
    if (finalOwnersAgree) {
        game.runGameRoutine([&] {
            const MapId source = pMapTable->GetMapInfo("oute3.odm");
            const MapId target = pMapTable->GetMapInfo("oute2.odm");
            firstInputReached = checkpoint->reachedInput() &&
                !gameTimer->isPaused() &&
                !(engineFlags & ENGINE_SKIP_NEXT_WORLD_UPDATE) &&
                !(engineFlags & ENGINE_SKIP_NEXT_USER_INPUT) &&
                engine->_currentLoadedMapId == source &&
                uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                !engine->_pendingTransition &&
                pPartyActionQueue->uNumActions == 0 &&
                target != MAP_INVALID;
            if (!firstInputReached)
                return;
            // Capture and preflight a complete source-map v5 candidate while
            // its runtime owners are still live. This remains in memory.
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable,
                                            GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto rebuilt = sourceDecorations(maps, "oute3.odm");
            const auto sourceV5 = mm6::captureV5FirstMapLiveSession(before);
            auto sourceStage = mm6::stageV5FullSessionPreflightOnHeap(
                sourceV5, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), pActors.size(),
                application->component<EngineRandomComponent>()->type(),
                rebuilt, *pDecorationTable, GameTitle::MM6);
            visitedOwner = std::make_unique<mm6::VisitedOutdoorRuntimeOwner>(
                mm6::stageVisitedOutdoorRuntimeOwner(
                    *sourceStage, "oute3.odm", maps.readBase("oute3.odm"),
                    *pDecorationTable, GameTitle::MM6));
            visitedOwnerStaged = !visitedOwner->canWriteCheckpoint();
            auto refuses = [&](auto &&mutate, auto &&restore) {
                mutate();
                bool refused = false;
                try {
                    mm6::requireVisitedOutdoorRuntimeOwner(
                        *visitedOwner, "oute3.odm", maps.readBase("oute3.odm"),
                        *pDecorationTable, GameTitle::MM6);
                } catch (const std::exception &) {
                    refused = true;
                }
                restore();
                return refused;
            };
            const auto nativeByte = visitedOwner->nativeDelta.bytes.front();
            const bool deltaRefused = refuses(
                [&] { visitedOwner->nativeDelta.bytes.front() ^= 1; },
                [&] { visitedOwner->nativeDelta.bytes.front() = nativeByte; });
            const bool eventRefused = refuses(
                [&] { visitedOwner->event.mapName = "wrong.odm"; },
                [&] { visitedOwner->event.mapName = "oute3.odm"; });
            const bool spriteRefused = refuses(
                [&] { visitedOwner->sprites.mapName = "wrong.odm"; },
                [&] { visitedOwner->sprites.mapName = "oute3.odm"; });
            visitedFaultsRefused = deltaRefused && eventRefused && spriteRefused;
            if (!visitedOwnerStaged || !visitedFaultsRefused)
                throw std::runtime_error("MM6 visited outdoor owner gate refused");
            const Blob &oldCache = pMapDeltas.at("oute3.ddm");
            const auto *oldBegin = static_cast<const std::uint8_t *>(oldCache.data());
            oldMapCacheBefore.assign(oldBegin, oldBegin + oldCache.size());
            sourceGeneratedCursor = pNPCStats->uNewlNPCBufPos;
            sourceHadChestPosition = std::any_of(
                vChests.begin(), vChests.end(),
                [](const Chest &chest) { return chest.position.has_value(); });
            // The source save's one-shot ACTR/GNPC/RNGS and EVRT scopes have
            // completed their owner checks. A normal later map transition
            // must generate its own NPC rows rather than re-consume them.
            postWorld.reset();
            replay.reset();
            checkpoint.reset();
            eventRuntime.reset();
            chestOptIn.reset();
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>();
            targetCapture = std::make_unique<
                mm6::testing::ScopedTransitionPostWorldCaptureForTest>(
                    GameTitle::MM6, target, [&, target] {
                        if (!gameTimer->isPaused() ||
                            engine->_currentLoadedMapId != target ||
                            engine->_pendingTransition ||
                            pPartyActionQueue->uNumActions)
                            throw std::runtime_error(
                                "MM6 target pre-input source order differs: paused=" +
                                std::to_string(gameTimer->isPaused()) +
                                " target=" + std::to_string(
                                    engine->_currentLoadedMapId == target) +
                                " pending=" + std::to_string(
                                    bool(engine->_pendingTransition)) +
                                " actions=" + std::to_string(
                                    pPartyActionQueue->uNumActions));
                        targetV5BeforeInput =
                            mm6::testing::captureV5LaterOutdoorForTest(before);
                        targetPreInputCaptured = !targetV5BeforeInput.empty();
                    });
            mm6::testing::ScopedTransitionAutosaveInterceptForTest intercept(
                GameTitle::MM6, source, target);
            startMapTransition(MapDestination(target, MAP_START_POINT_PARTY));
            autosaveIntercepted = intercept.intercepted();
            transitionQueued = autosaveIntercepted &&
                uGameState == GAME_STATE_CHANGE_LOCATION &&
                engine->_pendingTransition &&
                engine->_pendingTransition->map() == target &&
                engine->_currentLoadedMapId == source;
        });
        if (transitionQueued) {
            // The controller can yield while PrepareWorld is still on its
            // loading screen, before _461103 resets the generated-NPC pool.
            // Wait for that exact screen to finish before inspecting oute2.
            game.skipLoadingScreen();
            game.tick(2);
            game.runGameRoutine([&] {
                const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable,
                                                GameTitle::MM6);
                const auto native = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
                const auto sourceDecor = sourceDecorations(maps, "oute2.odm");
                const Blob &oldCache = pMapDeltas.at("oute3.ddm");
                const auto *oldBegin = static_cast<const std::uint8_t *>(oldCache.data());
                const std::vector<std::uint8_t> oldNow(
                    oldBegin, oldBegin + oldCache.size());
                std::size_t linkedGenerated = 0;
                for (const Actor &actor : pActors)
                    linkedGenerated += actor.npcId >= 5000;
                if (linkedGenerated != static_cast<std::size_t>(pNPCStats->uNewlNPCBufPos))
                    throw std::runtime_error(
                        "MM6 location-change GNPC cursor mismatch: source " +
                        std::to_string(sourceGeneratedCursor) + ", target " +
                        std::to_string(pNPCStats->uNewlNPCBufPos) + ", linked " +
                        std::to_string(linkedGenerated));
                const auto generated = mm6::captureGeneratedNpcFields(
                    pActors, *pNPCStats, "oute2.odm", GameTitle::MM6);
                mm6::requireGeneratedNpcActorAgreement(
                    generated, pActors, "oute2.odm", GameTitle::MM6);
                targetMapReady = engine->_currentLoadedMapId ==
                        pMapTable->GetMapInfo("oute2.odm") &&
                    pMapTable->pInfos[engine->_currentLoadedMapId].fileName == "oute2.odm" &&
                    uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                    !engine->_pendingTransition &&
                    !gameTimer->isPaused() &&
                    pPartyActionQueue->uNumActions == 0 &&
                    pActors.size() >= native.actorCount &&
                    pSpriteObjects.size() == native.groundItemCount &&
                    vChests.size() == native.chestCount &&
                    pLevelDecorations.size() == sourceDecor.size() &&
                    pMapDeltas.size() == maps.expectedNames().size() &&
                    oldNow == oldMapCacheBefore &&
                    mm6::chestTitleState().ready;
                if (targetMapReady) {
                    mm6::requireVisitedOutdoorRuntimeOwner(
                        *visitedOwner, "oute3.odm", maps.readBase("oute3.odm"),
                        *pDecorationTable, GameTitle::MM6);
                    visitedOwnerSurvivedTransition =
                        !visitedOwner->canWriteCheckpoint() &&
                        visitedOwner->nativeDelta.bytes ==
                            visitedOwner->chest.rawActiveDelta &&
                        oldNow == oldMapCacheBefore;
                    if (!targetPreInputCaptured || !targetCapture->captured())
                        throw std::runtime_error(
                            "MM6 target v5 was not captured before input");
                    const auto &targetV5 = targetV5BeforeInput;
                    const auto verifyItem = [](std::int32_t id) {
                        return id >= 1 && id <= 580 &&
                            !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
                    };
                    auto targetDecorations = sourceDecorations(maps, "oute2.odm");
                    auto targetStage = mm6::stageV5FullSessionPreflightOnHeap(
                        targetV5,
                        mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6),
                        maps.expectedNames(),
                        [&maps](std::string_view name) { return maps.readBase(name); },
                        verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                        pTileTable->tileCount(), pActors.size(),
                        application->component<EngineRandomComponent>()->type(),
                        targetDecorations, *pDecorationTable, GameTitle::MM6);
                    std::vector<mm6::VisitedOutdoorRuntimeOwner> bothMaps;
                    bothMaps.push_back(mm6::stageVisitedOutdoorRuntimeOwner(
                        *targetStage, "oute2.odm", maps.readBase("oute2.odm"),
                        *pDecorationTable, GameTitle::MM6));
                    bothMaps.push_back(*visitedOwner);
                    const auto bothSection = mm6::encodeVisitedOutdoorSection(
                        bothMaps, GameTitle::MM6);
                    const auto bothDecoded = mm6::decodeVisitedOutdoorSection(
                        {{bothSection}, 6}, GameTitle::MM6);
                    for (const auto &owner : bothDecoded)
                        mm6::requireVisitedOutdoorRuntimeOwner(
                            owner, owner.mapName, maps.readBase(owner.mapName),
                            *pDecorationTable, GameTitle::MM6);
                    visitedTwoMapSectionRoundTrip = bothDecoded.size() == 2 &&
                        bothDecoded[0].mapName == "oute2.odm" &&
                        bothDecoded[1].mapName == "oute3.odm" &&
                        mm6::encodeVisitedOutdoorSection(
                            bothDecoded, GameTitle::MM6) == bothSection;
                    auto outdoor = std::make_unique<mm6::V6OutdoorSaveFields>();
                    outdoor->active = mm6::decodeV5Save(targetV5, GameTitle::MM6);
                    outdoor->visited.push_back(*visitedOwner);
                    const auto candidate = mm6::encodeV6OutdoorSave(
                        *outdoor, GameTitle::MM6);
                    auto decoded = std::make_unique<mm6::V6OutdoorSaveFields>(
                        mm6::decodeV6OutdoorSave(candidate, GameTitle::MM6));
                    mm6::requireVisitedOutdoorRuntimeOwner(
                        decoded->visited.front(), "oute3.odm",
                        maps.readBase("oute3.odm"), *pDecorationTable,
                        GameTitle::MM6);
                    visitedV6RoundTrip = !decoded->canWriteCheckpoint() &&
                        decoded->visited.size() == 1 &&
                        decoded->active.v4.v3.raw.metadata.header.mapFileName ==
                            "oute2.odm" &&
                        mm6::encodeV5Save(decoded->active, GameTitle::MM6) ==
                            targetV5 &&
                        mm6::encodeV6OutdoorSave(*decoded, GameTitle::MM6) ==
                            candidate;
                    const auto refuses = [](auto &&action) {
                        try {
                            action();
                        } catch (const std::exception &) {
                            return true;
                        }
                        return false;
                    };
                    auto malformed = mm6::decodeSaveContainer(candidate);
                    malformed.sections.back().payload.front() = 2;
                    const auto badVersion = mm6::encodeSaveContainer(malformed);
                    const bool versionRefused = refuses([&] {
                        (void) mm6::decodeV6OutdoorSave(badVersion, GameTitle::MM6);
                    });
                    malformed.sections.back().payload.front() = 1;
                    malformed.sections.back().payload.pop_back();
                    const auto truncated = mm6::encodeSaveContainer(malformed);
                    const bool truncatedRefused = refuses([&] {
                        (void) mm6::decodeV6OutdoorSave(truncated, GameTitle::MM6);
                    });
                    auto crossMap = mm6::decodeSaveContainer(candidate);
                    crossMap.sections.back().payload[7] = '2';
                    const auto crossMapBytes = mm6::encodeSaveContainer(crossMap);
                    const bool wireCrossMapRefused = refuses([&] {
                        (void) mm6::decodeV6OutdoorSave(crossMapBytes,
                                                        GameTitle::MM6);
                    });
                    decoded->visited.front().mapName = "oute2.odm";
                    const bool crossMapRefused = refuses([&] {
                        (void) mm6::encodeV6OutdoorSave(*decoded, GameTitle::MM6);
                    });
                    visitedV6FaultsRefused = versionRefused &&
                        truncatedRefused && wireCrossMapRefused &&
                        crossMapRefused;
                    if (!visitedV6RoundTrip || !visitedTwoMapSectionRoundTrip ||
                        !visitedV6FaultsRefused)
                        throw std::runtime_error(
                            "MM6 visited outdoor v6 candidate refused");
                    if (probeReturn)
                        returnSaved = std::move(outdoor);
                    if (writeV6) {
                        constexpr std::string_view slot =
                            "candidate-v6-outdoor.ec6";
                        const auto validate = [&](std::span<const std::uint8_t> proposed) {
                            auto outer = std::make_unique<mm6::V6OutdoorSaveFields>(
                                mm6::decodeV6OutdoorSave(proposed, GameTitle::MM6));
                            if (outer->visited.size() != 1 ||
                                outer->visited.front().mapName != "oute3.odm" ||
                                mm6::encodeV5Save(outer->active, GameTitle::MM6) !=
                                    targetV5 ||
                                mm6::encodeVisitedOutdoorSection(
                                    outer->visited, GameTitle::MM6) !=
                                mm6::encodeVisitedOutdoorSection(
                                    std::span<const mm6::VisitedOutdoorRuntimeOwner>(
                                        visitedOwner.get(), 1), GameTitle::MM6))
                                throw std::runtime_error(
                                    "MM6 outdoor v6 writer owners differ");
                            mm6::requireVisitedOutdoorRuntimeOwner(
                                outer->visited.front(), "oute3.odm",
                                maps.readBase("oute3.odm"), *pDecorationTable,
                                GameTitle::MM6);
                            auto activeDecorations = sourceDecorations(maps, "oute2.odm");
                            auto activeStage = mm6::stageV5FullSessionPreflightOnHeap(
                                mm6::encodeV5Save(outer->active, GameTitle::MM6),
                                mm6::loadNpcStaticBaseline(
                                    *engine->resources(), GameTitle::MM6),
                                maps.expectedNames(),
                                [&maps](std::string_view name) {
                                    return maps.readBase(name);
                                }, verifyItem, *pItemTable,
                                pOverlayTable->overlays.size(),
                                pTileTable->tileCount(), pActors.size(),
                                application->component<EngineRandomComponent>()->type(),
                                activeDecorations, *pDecorationTable,
                                GameTitle::MM6);
                            if (activeStage->v4.v3.actorMap.activeMap.mapName !=
                                "oute2.odm")
                                throw std::runtime_error(
                                    "MM6 outdoor v6 active source map differs");
                        };
                        const bool malformedRefused = refuses([&] {
                            mm6::writeAtomicV6OutdoorSaveFileForTest(
                                profile, slot, badVersion, validate);
                        });
                        const bool crossMapRefusedBeforeWrite = refuses([&] {
                            mm6::writeAtomicV6OutdoorSaveFileForTest(
                                profile, slot, crossMapBytes, validate);
                        });
                        const bool injectedFaultRefused = refuses([&] {
                            mm6::detail::writeAtomicV6OutdoorSaveFileForTest(
                                profile, slot, candidate, validate,
                                [](mm6::detail::AtomicSaveStep step) {
                                    if (step == mm6::detail::AtomicSaveStep::BeforePrimaryReplace)
                                        throw std::runtime_error(
                                            "MM6 v6 injected precommit refusal");
                                });
                        });
                        v6AtomicFaultRefused = malformedRefused &&
                            crossMapRefusedBeforeWrite &&
                            injectedFaultRefused &&
                            profileSaveBytes(profile) == allSavesBefore;
                        if (!v6AtomicFaultRefused)
                            throw std::runtime_error(
                                "MM6 outdoor v6 fault changed a source slot");
                        mm6::writeAtomicV6OutdoorSaveFileForTest(
                            profile, slot, candidate, validate);
                        const auto stagedFile =
                            mm6::preflightRecoverableV6OutdoorSaveFileForTest(
                                profile, slot, validate);
                        v6CandidateWritten = stagedFile &&
                            !stagedFile->needsRecovery &&
                            stagedFile->bytes == candidate &&
                            slotBytes(profile / "saves" / slot) == candidate;
                        if (!v6CandidateWritten)
                            throw std::runtime_error(
                                "MM6 outdoor v6 candidate readback differs");
                        v6CandidateBytes = candidate;
                    }
                    pPartyActionQueue->Add(PARTY_TurnLeft);
                    pPartyActionQueue->Add(PARTY_WalkForward);
                }
            });
            if (targetMapReady) {
                const auto beforePosition = pParty->pos;
                const auto beforeYaw = pParty->_viewYaw;
                game.tick(3);
                game.runGameRoutine([&] {
                    movementApplied = pParty->pos != beforePosition &&
                        pParty->_viewYaw != beforeYaw &&
                        pPartyActionQueue->uNumActions == 0 &&
                        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute2.odm") &&
                        !engine->_pendingTransition;
                });
            }
            if (probeReturn && movementApplied) {
                game.runGameRoutine([&] {
                    targetCapture.reset();
                    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable,
                                                    GameTitle::MM6);
                    auto boundaryParty = std::make_unique<Party>(*pParty);
                    boundaryParty->uFlags &= ~(
                        PARTY_FLAG_WATER_DAMAGE | PARTY_FLAG_AIRBORNE |
                        PARTY_FLAG_STANDING_ON_WATER);
                    boundaryParty->bFlying = 0;
                    boundaryParty->pos.x = 0;
                    boundaryParty->pos.y = -mm6::MM6_OUTDOOR_BORDER_EDGE - 1;
                    auto arrival = mm6::stageVisitedOutdoorReturnArrival(
                        *returnSaved, "oute2.odm", "oute3.odm", maps,
                        *pDecorationTable, *boundaryParty, GameTitle::MM6);
                    if (arrival.refusal !=
                            mm6::OutdoorReturnArrivalRefusal::None ||
                        !arrival.placement || !arrival.detachedParty)
                        throw std::runtime_error(
                            "MM6 visited return has no detached arrival");
                    const PartyPlacement placement = *arrival.placement;
                    const Vec3f livePosition = pParty->pos;
                    const MapId sourceMap = engine->_currentLoadedMapId;
                    const MapId targetMap = pMapTable->GetMapInfo("oute3.odm");
                    returnProbe = std::make_unique<
                        mm6::testing::ScopedVisitedReturnPreEventProbeForTest>(
                            GameTitle::MM6, targetMap,
                            [&, placement, livePosition, targetMap] {
                                int faceId = -2;
                                bool onWater = false;
                                const float targetFloor = ODM_GetFloorLevel(
                                    placement.pos, &onWater, &faceId);
                                returnPreEventAgreed =
                                    engine->_currentLoadedMapId == targetMap &&
                                    uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                                    engine->_pendingTransition &&
                                    engine->_pendingTransition->map() == targetMap &&
                                    gameTimer->isPaused() &&
                                    pParty->pos == livePosition &&
                                    pOutdoor->pTerrain.heightByPos(placement.pos) ==
                                        placement.pos.z &&
                                    targetFloor == placement.pos.z &&
                                    faceId == -1;
                                if (!returnPreEventAgreed)
                                    throw std::runtime_error(
                                        "MM6 visited return target-world floor/order differs");
                            });
                    mm6::testing::ScopedTransitionAutosaveInterceptForTest intercept(
                        GameTitle::MM6, sourceMap, targetMap);
                    startMapTransition(MapDestination(targetMap, placement));
                    returnTransitionQueued = intercept.intercepted() &&
                        uGameState == GAME_STATE_CHANGE_LOCATION &&
                        engine->_pendingTransition &&
                        engine->_pendingTransition->map() == targetMap;
                });
                if (returnTransitionQueued) {
                    // The location-change loading screen may yield before
                    // PrepareWorld. Keep the one-shot scope armed until the
                    // intercepted return actually reaches the target.
                    game.skipLoadingScreen();
                    game.tick(2);
                    game.runGameRoutine([&] {
                        returnProbeReached = returnProbe->reached();
                    });
                }
            }
        }
        if (!probeReturn) {
            game.runGameRoutine([] {
                uGameState = GAME_STATE_GAME_QUITTING_TO_MAIN_MENU;
            });
            game.tick(2);
        }
    }
    game.runGameRoutine([&] {
        postWorld.reset();
        targetCapture.reset();
        returnProbe.reset();
        replay.reset();
        checkpoint.reset();
        eventRuntime.reset();
        chestOptIn.reset();
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(false);
        mm6::testing::setLoadGameOverride({});
    });
    EXPECT_TRUE(selected);
    EXPECT_TRUE(checkpointRan);
    EXPECT_TRUE(freshMapBeforeReplay);
    EXPECT_TRUE(postWorldRan);
    EXPECT_TRUE(finalOwnersAgree) << finalOwnerError;
    EXPECT_TRUE(firstInputReached);
    EXPECT_TRUE(sourceHadChestPosition);
    EXPECT_TRUE(visitedOwnerStaged);
    EXPECT_TRUE(visitedFaultsRefused);
    EXPECT_TRUE(visitedOwnerSurvivedTransition);
    EXPECT_TRUE(visitedV6RoundTrip);
    EXPECT_TRUE(visitedTwoMapSectionRoundTrip);
    EXPECT_TRUE(visitedV6FaultsRefused);
    EXPECT_TRUE(targetPreInputCaptured);
    EXPECT_TRUE(autosaveIntercepted);
    EXPECT_TRUE(transitionQueued);
    EXPECT_TRUE(targetMapReady);
    EXPECT_TRUE(movementApplied);
    if (probeReturn) {
        EXPECT_TRUE(returnTransitionQueued);
        EXPECT_TRUE(returnProbeReached);
        EXPECT_TRUE(returnPreEventAgreed);
        EXPECT_EQ(profileSaveBytes(profile), allSavesBefore);
    }
    if (writeV6) {
        EXPECT_TRUE(v6AtomicFaultRefused);
        EXPECT_TRUE(v6CandidateWritten);
        EXPECT_EQ(slotBytes(v5Path), v5Before);
        auto remaining = profileSaveBytes(profile);
        const auto written = remaining.find("candidate-v6-outdoor.ec6");
        ASSERT_NE(written, remaining.end());
        EXPECT_EQ(written->second, v6CandidateBytes);
        remaining.erase(written);
        EXPECT_EQ(remaining, allSavesBefore);
    } else {
        EXPECT_EQ(profileSaveBytes(profile), allSavesBefore);
    }
    EXPECT_EQ(slotBytes(candidate), before);
    EXPECT_EQ(slotBytes(profile / "saves" / "autosave.ec6"), seedBefore);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

GAME_TEST(MM6LocationChangeLifecycleHeadless, DISABLED_FirstOutdoorToSecondWithoutDiskWrite) {
    runMM6LocationChangeLifecycle(game, false);
}

GAME_TEST(MM6OutdoorV6Headless, DISABLED_WritesActiveAndVisitedCandidate) {
    runMM6LocationChangeLifecycle(game, true);
}

GAME_TEST(MM6VisitedOutdoorReturnHeadless, DISABLED_AbortsBeforePlacementAndMapEvents) {
    runMM6LocationChangeLifecycle(game, false, true);
}
