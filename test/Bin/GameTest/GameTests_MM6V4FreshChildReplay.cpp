#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
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
#include "Engine/MM6/MM6PostWorldOwnerValidator.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/MM6/MM6V3FullSessionPreflight.h"
#include "Engine/MM6/MM6V3PreEventReplayForTest.h"
#include "Engine/MM6/MM6V4FullSessionPreflight.h"
#include "Engine/MM6/MM6V4OutdoorDecorationReplay.h"
#include "Engine/MM6/MM6V4SaveFields.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/MM6/MM6ChestRuntimeProjection.h"
#include "Engine/MM6/MM6ChestLiveOwnerAdapter.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"
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
        result[i].uDecorationDescID = pDecorationTable->decorationId(
            std::string(name.begin(), end));
        if (result[i].uDecorationDescID == DECORATION_NULL)
            throw std::runtime_error("MM6 v4 base decoration name lacks a template");
    }
    return result;
}

} // namespace

// Run only after a separate strict post-entry v4 writer and owner admission
// produce the exact disposable candidate-v4.ec6. This stays disabled while
// no exact v4 slot exists. It never enables production SaveLoad or input.
GAME_TEST(MM6V4FreshChildReplayHeadless, DISABLED_FreshQuickLoadRestoresBeforeEventsAndAborts) {
    const char *enable = std::getenv("ENROTH_MM6_V4_REPLAY_ENABLE");
    if (!enable || std::string_view(enable) != "strict-v4")
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
    ASSERT_FALSE(before.empty());
    ASSERT_FALSE(seedBefore.empty());

    bool selected = false;
    bool checkpointRan = false;
    bool freshMapBeforeReplay = false;
    bool postWorldRan = false;
    bool finalOwnersAgree = false;
    std::string finalOwnerError;
    std::size_t expectedNativeActors = 0;
    std::size_t expectedActiveActors = 0;
    mm6::GeneratedNpcSaveFields expectedGenerated;
    std::unique_ptr<mm6::V4FullSessionPreflight> staged;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedEventRuntimeRestoreForTest> eventRuntime;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    std::unique_ptr<mm6::testing::ScopedV3PreEventReplayForTest> replay;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> postWorld;
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
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>();
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
                        auto legacyPersistent = engine->_persistentVariables;
                        legacyPersistent.mapVars.fill(0);
                        legacyPersistent.decorVars.fill(0);
                        const mm6::PostWorldOwnerView owners{
                            engine->_currentLoadedMapId, uCurrentlyLoadedLevelType,
                            pMapTable, pItemTable, finalActors, pSpriteObjects, vChests,
                            pOutdoor->mm6DeltaBytes, &legacyPersistent,
                            &mm6::chestTitleState()};
                        mm6::requireRawV2PostWorldOwnerAgreement(
                            staged->v3.represented, owners, GameTitle::MM6);
                        const auto &savedRaw = staged->saved.v3.raw;
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
                }, mm6::testing::PostWorldAdmissionResult::AbortToMainMenu);
            selected = true;
        });
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });

    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] {
        postWorld.reset();
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
    EXPECT_EQ(slotBytes(candidate), before);
    EXPECT_EQ(slotBytes(profile / "saves" / "autosave.ec6"), seedBefore);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
