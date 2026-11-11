#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
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
#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6EventRuntimeRestoreForTest.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NativeActorNpcPolicy.h"
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
#include "Engine/MM6/MM6V5ActorRuntimeFields.h"
#include "Engine/MM6/MM6V5DecorationRuntimeFields.h"
#include "Engine/MM6/MM6V5PreEventActorReplayForTest.h"
#include "Engine/MM6/MM6V5PreEventDecorationReplayForTest.h"
#include "Engine/MM6/MM6V5PreEventSpriteReplayForTest.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/MM6/MM6V6OutdoorSaveFields.h"
#include "Engine/MM6/MM6VisitedOutdoorReturnPlan.h"
#include "Engine/MM6/MM6V5OutdoorDecorationSource.h"
#include "Engine/MM6/MM6V5SpriteRuntimeFields.h"
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
        throw std::runtime_error("MM6 v5 later outdoor replay needs an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 v5 later outdoor replay needs an owner-only system-temp profile");
    return root;
}

std::filesystem::path laterCandidateInProfile(const std::filesystem::path &profile,
                                              std::string_view slot) {
    if (slot != "candidate-v5-oute2.ec6" &&
        slot != "candidate-v5-oute2-after-input.ec6" &&
        slot != "candidate-v6-outdoor.ec6")
        throw std::runtime_error("MM6 v5 later outdoor replay has no admitted slot");
    const std::filesystem::path supplied = profile / "saves" / std::string(slot);
    if (!std::filesystem::is_regular_file(supplied))
        throw std::runtime_error("MM6 v5 later outdoor replay needs an existing absolute candidate");
    const std::filesystem::path candidate = std::filesystem::canonical(supplied);
    if (candidate.parent_path() != std::filesystem::canonical(profile / "saves") ||
        candidate.filename().string() != slot)
        throw std::runtime_error("MM6 v5 later outdoor replay candidate must be a separate profile save");
    return candidate;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    constexpr std::uintmax_t maxSaveBytes = 64 * 1024 * 1024;
    if (std::filesystem::file_size(path) > maxSaveBytes)
        throw std::runtime_error("MM6 v5 later outdoor replay candidate exceeds the EC6 bound");
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

mm6::CorePartyFields actionPartyFields(std::span<const std::uint8_t> bytes) {
    const auto decoded = std::unique_ptr<mm6::V5SaveFields>(
        new mm6::V5SaveFields(mm6::decodeV5Save(bytes, GameTitle::MM6)));
    return decoded->v4.v3.raw.core.party;
}

std::vector<LevelDecoration> sourceDecorations(const mm6::LoadedMapSource &maps,
                                               std::string_view mapName) {
    const Blob baseBytes = maps.readBase(mapName);
    auto source = std::make_unique<OutdoorLocation_MM7>();
    deserializeMM6Outdoor(baseBytes, source.get());
    std::vector<LevelDecoration> result(source->decorations.size());
    if (source->decorationNames.size() != result.size())
        throw std::runtime_error("MM6 v5 base decorations lack source names");
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

// A separate writer enters the non-start outdoor map from a strict disposable
// source. Each fresh child first proves every saved owner before admitting
// either an abort or one bounded input sequence.
enum class LaterReplayMode {
    AbortBeforeInput, FirstInput, SecondWrite, SecondReplay, V6VisitedRead
};

void runV5LaterOutdoorFreshChildReplay(EngineController &game, LaterReplayMode mode) {
    const bool allowInput = mode == LaterReplayMode::FirstInput ||
                            mode == LaterReplayMode::SecondWrite;
    const bool secondReplay = mode == LaterReplayMode::SecondReplay;
    const bool v6Read = mode == LaterReplayMode::V6VisitedRead;
    const char *enable = std::getenv(
        mode == LaterReplayMode::AbortBeforeInput ?
            "ENROTH_MM6_V5_OUTE2_REPLAY_ENABLE" :
        mode == LaterReplayMode::FirstInput ?
            "ENROTH_MM6_V5_OUTE2_FIRST_INPUT_ENABLE" :
        mode == LaterReplayMode::SecondWrite ?
            "ENROTH_MM6_V5_OUTE2_SECOND_WRITE_ENABLE" :
        mode == LaterReplayMode::SecondReplay ?
            "ENROTH_MM6_V5_OUTE2_SECOND_REPLAY_ENABLE" :
            "ENROTH_MM6_V6_OUTDOOR_REPLAY_ENABLE");
    const std::string_view expectedEnable =
        mode == LaterReplayMode::AbortBeforeInput ? "strict-v5-oute2" :
        mode == LaterReplayMode::FirstInput ? "strict-v5-oute2-first-input" :
        mode == LaterReplayMode::SecondWrite ? "strict-v5-oute2-second-write" :
        mode == LaterReplayMode::SecondReplay ?
            "strict-v5-oute2-second-replay" :
            "strict-v6-outdoor-replay";
    if (!enable || std::string_view(enable) != expectedEnable)
        GTEST_SKIP() << "Run only after strict v5 writer and owner admission";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    const std::filesystem::path candidate = laterCandidateInProfile(
        profile, v6Read ? "candidate-v6-outdoor.ec6" :
                 secondReplay ? "candidate-v5-oute2-after-input.ec6" :
                                "candidate-v5-oute2.ec6");
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto wireBefore = slotBytes(candidate);
    std::vector<std::uint8_t> before;
    if (v6Read) {
        const auto outer = std::unique_ptr<mm6::V6OutdoorSaveFields>(
            new mm6::V6OutdoorSaveFields(
                mm6::decodeV6OutdoorSave(wireBefore, GameTitle::MM6)));
        before = mm6::encodeV5Save(outer->active, GameTitle::MM6);
    } else {
        before = wireBefore;
    }
    const auto priorV5Before =
        slotBytes(profile / "saves" / "candidate-v5-oute2.ec6");
    const auto v4Before = slotBytes(profile / "saves" / "candidate-v4.ec6");
    const auto seedBefore = slotBytes(profile / "saves" / "autosave.ec6");
    ASSERT_FALSE(before.empty());
    ASSERT_FALSE(priorV5Before.empty());
    ASSERT_FALSE(v4Before.empty());
    ASSERT_FALSE(seedBefore.empty());
    const auto actionParty = actionPartyFields(before);
    if (secondReplay) {
        const auto priorParty = actionPartyFields(priorV5Before);
        ASSERT_NE(actionParty.position, priorParty.position);
        ASSERT_NE(actionParty.viewYaw, priorParty.viewYaw);
    }

    bool selected = false;
    bool checkpointRan = false;
    bool freshMapBeforeReplay = false;
    bool postWorldRan = false;
    bool finalOwnersAgree = false;
    bool firstInputReached = false;
    bool movementApplied = false;
    bool secondCandidateWritten = false;
    bool visitedPreflighted = false;
    bool visitedPostWorldValidated = false;
    bool returnPlanRefusedWithoutArrival = false;
    bool returnPlanStayedDetached = false;
    bool returnArrivalStayedDetached = false;
    std::string finalOwnerError;
    std::size_t expectedNativeActors = 0;
    std::size_t expectedActiveActors = 0;
    mm6::GeneratedNpcSaveFields expectedGenerated;
    std::unique_ptr<mm6::V5FullSessionPreflight> staged;
    std::unique_ptr<mm6::V6OutdoorSaveFields> visitedSaved;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedEventRuntimeRestoreForTest> eventRuntime;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    std::unique_ptr<mm6::testing::ScopedV3PreEventReplayForTest> replay;
    std::unique_ptr<mm6::testing::ScopedV5PreEventActorReplayForTest> actorReplay;
    std::unique_ptr<mm6::testing::ScopedV5PreEventDecorationReplayForTest> decorationReplay;
    std::unique_ptr<mm6::testing::ScopedV5PreEventSpriteReplayForTest> spriteReplay;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> postWorld;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view requested) {
            if (requested != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 v5 later outdoor replay selected another menu route");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            std::unique_ptr<mm6::V5FullSessionPreflight> selectedV6Stage;
            if (v6Read) {
                const auto validateVisited = [&](std::span<const std::uint8_t> proposed) {
                    auto outer = std::unique_ptr<mm6::V6OutdoorSaveFields>(
                        new mm6::V6OutdoorSaveFields(
                            mm6::decodeV6OutdoorSave(proposed, GameTitle::MM6)));
                    if (outer->visited.size() != 1 ||
                        outer->visited.front().mapName != "oute3.odm" ||
                        mm6::encodeV5Save(outer->active, GameTitle::MM6) != before)
                        throw std::runtime_error(
                            "MM6 outdoor v6 fresh-child map set differs");
                    mm6::requireVisitedOutdoorRuntimeOwner(
                        outer->visited.front(), "oute3.odm",
                        maps.readBase("oute3.odm"), *pDecorationTable,
                        GameTitle::MM6);
                    auto returnPlan = std::unique_ptr<mm6::VisitedOutdoorReturnPlan>(
                        new mm6::VisitedOutdoorReturnPlan(
                            mm6::stageVisitedOutdoorReturnPlan(
                                *outer, "oute2.odm", "oute3.odm",
                                maps.readBase("oute3.odm"), *pDecorationTable,
                                std::nullopt, GameTitle::MM6)));
                    const auto &currentChest = outer->active.v4.v3.raw.chest;
                    returnPlanRefusedWithoutArrival =
                        returnPlan->refusal ==
                            mm6::VisitedOutdoorReturnRefusal::MissingArrival &&
                        !returnPlan->canPublish() &&
                        !returnPlan->canWriteCheckpoint() &&
                        returnPlan->visited.nativeDelta ==
                            outer->visited.front().nativeDelta &&
                        returnPlan->returnChest.activeDeltaName ==
                            outer->visited.front().nativeDelta.name &&
                        returnPlan->returnChest.rawActiveDelta ==
                            outer->visited.front().nativeDelta.bytes &&
                        returnPlan->returnChest.lcgState == currentChest.lcgState &&
                        returnPlan->returnChest.generatedArtifactFlags ==
                            currentChest.generatedArtifactFlags;
                    if (!returnPlanRefusedWithoutArrival)
                        throw std::runtime_error(
                            "MM6 visited return plan did not refuse missing arrival");
                    auto rebuiltActive = sourceDecorations(maps, "oute2.odm");
                    selectedV6Stage = mm6::stageV5FullSessionPreflightOnHeap(
                        mm6::encodeV5Save(outer->active, GameTitle::MM6),
                        freshNpcs, maps.expectedNames(),
                        [&maps](std::string_view name) {
                            return maps.readBase(name);
                        }, verifyItem, *pItemTable,
                        pOverlayTable->overlays.size(), pTileTable->tileCount(),
                        engine->config->gameplay.MaxActors.value(),
                        application->component<EngineRandomComponent>()->type(),
                        rebuiltActive, *pDecorationTable, GameTitle::MM6);
                    if (selectedV6Stage->v4.v3.actorMap.activeMap.mapName !=
                        "oute2.odm")
                        throw std::runtime_error(
                            "MM6 outdoor v6 active source map differs");
                };
                const auto selectedFile =
                    mm6::preflightRecoverableV6OutdoorSaveFileForTest(
                        profile, "candidate-v6-outdoor.ec6", validateVisited);
                if (!selectedFile || selectedFile->needsRecovery ||
                    selectedFile->bytes != wireBefore)
                    throw std::runtime_error(
                        "MM6 outdoor v6 fresh-child slot selection differs");
                visitedSaved.reset(new mm6::V6OutdoorSaveFields(
                    mm6::decodeV6OutdoorSave(selectedFile->bytes, GameTitle::MM6)));
                visitedPreflighted = true;
            }
            auto rebuiltDecorations = sourceDecorations(maps, "oute2.odm");
            auto candidateStage = v6Read ? std::move(selectedV6Stage) :
                mm6::stageV5FullSessionPreflightOnHeap(
                before, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), engine->config->gameplay.MaxActors.value(),
                application->component<EngineRandomComponent>()->type(),
                rebuiltDecorations, *pDecorationTable, GameTitle::MM6);
            if (candidateStage->v4.v3.actorMap.activeMap.mapName != "oute2.odm" ||
                candidateStage->v4.saved.v3.raw.core.party.position != actionParty.position ||
                candidateStage->v4.saved.v3.raw.core.party.viewYaw != actionParty.viewYaw ||
                candidateStage->v4.v3.armageddon.timerTicks != 0 ||
                candidateStage->v4.v3.cursorImage.occupied)
                throw std::runtime_error("MM6 v5 later outdoor replay has no exact second-outdoor candidate");
            staged = std::move(candidateStage);
            expectedNativeActors = staged->v4.v3.actorMap.activeMap.delta.actors.size();
            expectedActiveActors = staged->actors.actors.size();
            expectedGenerated = staged->v4.saved.v3.generatedNpcs;

            *pParty = staged->v4.v3.represented.party.party;
            *pNPCStats = staged->v4.v3.represented.world.world.npcs;
            *gameTimer = staged->v4.v3.represented.world.visuals.timer;
            *pActiveOverlayList = staged->v4.v3.represented.world.visuals.overlays;
            pMapDeltas = std::move(staged->v4.v3.represented.world.world.mapDeltas);
            mm6::chestTitleState() = staged->v4.v3.represented.world.chestTitle;
            mm6::publishFreshChildPersistentEventZeros(
                &engine->_persistentVariables,
                staged->v4.v3.represented.persistentEvents, GameTitle::MM6);
            mm6::publishRawCursorShopOwners(
                &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                staged->v4.v3.represented.raw.saved,
                {std::move(staged->v4.v3.represented.raw.cursor),
                 std::move(staged->v4.v3.represented.raw.shops)}, GameTitle::MM6);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute2.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(true);
            eventRuntime = std::make_unique<mm6::testing::ScopedEventRuntimeRestoreForTest>(
                GameTitle::MM6, staged->v4.saved.eventRuntime);
            checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
                GameTitle::MM6, [&] {
                    checkpointRan = true;
                    freshMapBeforeReplay = gameTimer->isPaused() &&
                        engine->_pendingTransition &&
                        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute2.odm") &&
                        uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                        pActors.size() == expectedNativeActors &&
                        pNPCStats->uNewlNPCBufPos == 0 &&
                        pPartyActionQueue->uNumActions == 0;
                });
            replay = std::make_unique<mm6::testing::ScopedV3PreEventReplayForTest>(
                GameTitle::MM6, std::move(staged->v4.v3.actorMap));
            actorReplay = std::make_unique<mm6::testing::ScopedV5PreEventActorReplayForTest>(
                GameTitle::MM6, staged->actors,
                staged->v4.saved.v3.activeActorPayload);
            decorationReplay = std::make_unique<mm6::testing::ScopedV5PreEventDecorationReplayForTest>(
                GameTitle::MM6, staged->decorations);
            spriteReplay = std::make_unique<mm6::testing::ScopedV5PreEventSpriteReplayForTest>(
                GameTitle::MM6, staged->sprites);
            postWorld = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                GameTitle::MM6, [&] {
                    postWorldRan = true;
                    try {
                        if (!checkpoint->reachedMapLoad() || checkpoint->reachedInput() ||
                            !replay->published() || !replay->rngPublished() ||
                            !actorReplay->published() || !decorationReplay->published() ||
                            !spriteReplay->published() ||
                            !eventRuntime->restored() ||
                            pActors.size() != expectedActiveActors ||
                            expectedActiveActors < expectedNativeActors ||
                            engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute2.odm") ||
                            uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
                            !gameTimer->isPaused() || engine->_pendingTransition ||
                            pPartyActionQueue->uNumActions != 0)
                            throw std::runtime_error("MM6 v5 later outdoor replay reached an unexpected world order");
                        if (v6Read) {
                            if (!visitedSaved || !visitedPreflighted)
                                throw std::runtime_error(
                                    "MM6 outdoor v6 visited owner was not staged");
                            const mm6::LoadedMapSource sourceMaps(
                                *pGames_LOD, *pMapTable, GameTitle::MM6);
                            mm6::requireVisitedOutdoorRuntimeOwner(
                                visitedSaved->visited.front(), "oute3.odm",
                                sourceMaps.readBase("oute3.odm"),
                                *pDecorationTable, GameTitle::MM6);
                            visitedPostWorldValidated = true;
                            const auto coreBefore =
                                mm6::captureCoreFields(*pParty, GameTitle::MM6);
                            const auto chestBefore = mm6::chestTitleState();
                            const Blob &oldCache = pMapDeltas.at("oute3.ddm");
                            const auto *oldBegin = static_cast<const std::uint8_t *>(
                                oldCache.data());
                            const std::vector<std::uint8_t> cacheBefore(
                                oldBegin, oldBegin + oldCache.size());
                            auto returnPlan = std::unique_ptr<mm6::VisitedOutdoorReturnPlan>(
                                new mm6::VisitedOutdoorReturnPlan(
                                    mm6::stageVisitedOutdoorReturnPlan(
                                        *visitedSaved, "oute2.odm", "oute3.odm",
                                        sourceMaps.readBase("oute3.odm"),
                                        *pDecorationTable, std::nullopt,
                                        GameTitle::MM6)));
                            auto boundaryParty = std::make_unique<Party>(*pParty);
                            // The saved active Party may carry an offer-blocking
                            // state. This fixture models an eligible border
                            // attempt only on a detached copy.
                            boundaryParty->uFlags &= ~(
                                PARTY_FLAG_WATER_DAMAGE | PARTY_FLAG_AIRBORNE |
                                PARTY_FLAG_STANDING_ON_WATER);
                            boundaryParty->bFlying = 0;
                            boundaryParty->pos.x = 0;
                            boundaryParty->pos.y = -mm6::MM6_OUTDOOR_BORDER_EDGE - 1;
                            auto returnArrival = mm6::stageVisitedOutdoorReturnArrival(
                                *visitedSaved, "oute2.odm", "oute3.odm",
                                sourceMaps, *pDecorationTable,
                                *boundaryParty, GameTitle::MM6);
                            auto inBoundsParty = std::make_unique<Party>(*boundaryParty);
                            inBoundsParty->pos.y = 0;
                            const auto invalidArrival =
                                mm6::stageVisitedOutdoorReturnArrival(
                                    *visitedSaved, "oute2.odm", "oute3.odm",
                                    sourceMaps,
                                    *pDecorationTable, *inBoundsParty, GameTitle::MM6);
                            returnArrivalStayedDetached =
                                returnArrival.refusal ==
                                    mm6::OutdoorReturnArrivalRefusal::None &&
                                returnArrival.detachedParty &&
                                returnArrival.placement &&
                                returnArrival.detachedParty->pos ==
                                    Vec3f(0, mm6::MM6_OUTDOOR_BORDER_EDGE - 1,
                                          returnArrival.placement->pos.z) &&
                                invalidArrival.refusal ==
                                    mm6::OutdoorReturnArrivalRefusal::InvalidRoute &&
                                !invalidArrival.detachedParty &&
                                !returnArrival.canPublish() &&
                                !returnArrival.canWriteCheckpoint();
                            if (!returnArrivalStayedDetached)
                                throw std::runtime_error(
                                    "MM6 detached return arrival refused: stage=" +
                                    std::to_string(static_cast<int>(returnArrival.refusal)) +
                                    " invalid=" +
                                    std::to_string(static_cast<int>(invalidArrival.refusal)) +
                                    " hasParty=" +
                                    std::to_string(returnArrival.detachedParty != nullptr) +
                                    " hasPlacement=" +
                                    std::to_string(returnArrival.placement.has_value()));
                            const Blob &oldCacheAfter = pMapDeltas.at("oute3.ddm");
                            const auto *afterBegin = static_cast<const std::uint8_t *>(
                                oldCacheAfter.data());
                            returnPlanStayedDetached =
                                returnPlan->refusal ==
                                    mm6::VisitedOutdoorReturnRefusal::MissingArrival &&
                                !returnPlan->canPublish() &&
                                !returnPlan->canWriteCheckpoint() &&
                                mm6::captureCoreFields(*pParty, GameTitle::MM6) ==
                                    coreBefore &&
                                mm6::chestTitleState().ready == chestBefore.ready &&
                                mm6::chestTitleState().lcgState ==
                                    chestBefore.lcgState &&
                                mm6::chestTitleState().generatedArtifactFlags ==
                                    chestBefore.generatedArtifactFlags &&
                                returnArrivalStayedDetached &&
                                std::vector<std::uint8_t>(
                                    afterBegin, afterBegin + oldCacheAfter.size()) ==
                                    cacheBefore;
                            if (!returnPlanStayedDetached)
                                throw std::runtime_error(
                                    "MM6 visited return plan changed live owners");
                        }
                        mm6::requireGeneratedNpcActorAgreement(
                            expectedGenerated, pActors, "oute2.odm", GameTitle::MM6);
                        const auto observedGenerated = mm6::captureGeneratedNpcFields(
                            pActors, *pNPCStats, "oute2.odm", GameTitle::MM6);
                        if (mm6::encodeGeneratedNpcSection(observedGenerated, GameTitle::MM6) !=
                            mm6::encodeGeneratedNpcSection(expectedGenerated, GameTitle::MM6))
                            throw std::runtime_error("MM6 v5 generated NPC rows changed after map events");
                        const std::vector<Actor> finalActors(pActors.begin(), pActors.end());
                        const auto finalActorRuntime = mm6::captureActorRuntimeFields(
                            "oute2.odm", staged->v4.saved.v3.activeActorPayload,
                            finalActors, GameTitle::MM6);
                        if (mm6::encodeActorRuntimeSection(finalActorRuntime, GameTitle::MM6) !=
                            mm6::encodeActorRuntimeSection(staged->actors, GameTitle::MM6))
                            throw std::runtime_error("MM6 v5 ARTR changed after map events");
                        const auto finalSprites = mm6::captureSpriteRuntimeFields(
                            "oute2.odm", pOutdoor->mm6DeltaBytes, pSpriteObjects,
                            GameTitle::MM6);
                        if (mm6::encodeSpriteRuntimeSection(finalSprites, GameTitle::MM6) !=
                            mm6::encodeSpriteRuntimeSection(staged->sprites, GameTitle::MM6))
                            throw std::runtime_error("MM6 v5 SPRT changed after map events");
                        if (engine->_persistentVariables.mapVars != staged->v4.activeEventRuntime.mapVars ||
                            engine->_persistentVariables.decorVars != staged->v4.activeEventRuntime.decorVars)
                            throw std::runtime_error("MM6 v5 EVRT arrays changed after map events");
                        const mm6::LoadedMapSource loadedMaps(*pGames_LOD, *pMapTable,
                                                               GameTitle::MM6);
                        const auto finalDecorations = mm6::captureDecorationRuntimeFields(
                            "oute2.odm", loadedMaps.readBase("oute2.odm"),
                            staged->v4.activeEventRuntime, *pDecorationTable,
                            pLevelDecorations, GameTitle::MM6);
                        if (mm6::encodeDecorationRuntimeSection(finalDecorations, GameTitle::MM6) !=
                            mm6::encodeDecorationRuntimeSection(staged->decorations, GameTitle::MM6))
                            throw std::runtime_error("MM6 v5 DECR changed after map events");
                        const auto outdoorWire = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
                        const auto nativeBytes = outdoorWire.part(mm6::OutdoorDeltaPart::PersistentBytes);
                        const auto expectedNative = mm6::capturePersistentEventBoundary(
                            staged->v4.v3.represented.persistentEvents, GameTitle::MM6);
                        if (nativeBytes.size() != expectedNative.size() ||
                            !std::equal(nativeBytes.begin(), nativeBytes.end(),
                                        expectedNative.begin(), expectedNative.end()))
                            throw std::runtime_error("MM6 v5 native event bytes changed after map events");
                        const auto &savedRaw = staged->v4.saved.v3.raw;
                        if (pMapTable->pInfos[engine->_currentLoadedMapId].fileName !=
                                savedRaw.metadata.header.mapFileName ||
                            !staged->v4.v3.represented.outdoorPlan ||
                            staged->v4.v3.represented.outdoorPlan->mapName != "oute2.odm")
                            throw std::runtime_error("MM6 v5 final map differs from HEAD");
                        mm6::requireNoActiveSavedActorOverlays(
                            savedRaw.metadata.overlays, GameTitle::MM6);
                        const auto &pidPlan = *staged->v4.v3.represented.outdoorPlan;
                        if (pidPlan.savedDelta.actors.size() != expectedNativeActors ||
                            pidPlan.activeSidecar.actors.size() != expectedNativeActors ||
                            finalActors.size() < expectedNativeActors ||
                            pidPlan.savedDelta.spriteObjects.size() !=
                                pidPlan.activeSidecar.spriteObjects.size() ||
                            pidPlan.savedDelta.spriteObjects.size() != pSpriteObjects.size())
                            throw std::runtime_error("MM6 later outdoor native PID slot counts changed");
                        for (std::size_t slot = 0; slot < expectedNativeActors; ++slot) {
                            const auto &saved = pidPlan.savedDelta.actors[slot];
                            const auto &sidecar = pidPlan.activeSidecar.actors[slot];
                            const Actor &live = finalActors[slot];
                            const auto refuse = [slot](std::string_view field) {
                                throw std::runtime_error("MM6 later outdoor PID " +
                                                         std::string(field) + " differs at slot " +
                                                         std::to_string(slot));
                            };
                            if (saved.npcId != sidecar.npcId)
                                refuse("native versus sidecar NPC ID");
                            if (saved.npcId != live.npcId) {
                                // MM6 map setup assigns a street peasant a 5000+
                                // NPC row after reading the DDM's nonzero NPC link.
                                // GNPC and exact ARTR above bind that row and the
                                // final actor to this same ordered PID slot.
                                const auto row = std::find_if(
                                    expectedGenerated.rows.begin(), expectedGenerated.rows.end(),
                                    [slot](const mm6::GeneratedNpcRow &candidate) {
                                        return candidate.actorSlot == slot;
                                    });
                                if (!mm6::getsStreetNpc(live.monsterInfo.id, saved.npcId) ||
                                    row == expectedGenerated.rows.end() ||
                                    live.npcId != 5000 +
                                        std::distance(expectedGenerated.rows.begin(), row) ||
                                    row->monster != live.monsterInfo.id)
                                    refuse("generated NPC binding");
                            }
                            if (saved.monsterId != sidecar.monsterId ||
                                saved.monsterId != std::to_underlying(live.monsterId))
                                refuse("monster ID");
                            if (saved.monsterInfo.id != sidecar.monsterInfo.id ||
                                saved.monsterInfo.id != std::to_underlying(live.monsterInfo.id))
                                refuse("monster info ID");
                            if (saved.initialPosition != sidecar.initialPosition ||
                                saved.initialPosition.toFloat() != live.initialPosition)
                                refuse("initial position");
                            if (live.id != slot)
                                refuse("runtime slot ID");
                        }
                        for (std::size_t slot = 0;
                             slot < pidPlan.savedDelta.spriteObjects.size(); ++slot) {
                            const auto &saved = pidPlan.savedDelta.spriteObjects[slot];
                            const auto &sidecar = pidPlan.activeSidecar.spriteObjects[slot];
                            const SpriteObject &live = pSpriteObjects[slot];
                            if (saved.spriteId != sidecar.spriteId ||
                                saved.spriteId != std::to_underlying(live.spriteId) ||
                                saved.objectDescId != sidecar.objectDescId ||
                                saved.objectDescId != live.uObjectDescID ||
                                saved.initialPosition != sidecar.initialPosition ||
                                saved.initialPosition.toFloat() != live.initialPosition)
                                throw std::runtime_error("MM6 later outdoor sprite PID anchor changed at slot " +
                                                         std::to_string(slot));
                        }
                        const auto &chest = mm6::chestTitleState();
                        if (!chest.ready || chest.lcgState != savedRaw.chest.lcgState ||
                            chest.generatedArtifactFlags != savedRaw.chest.generatedArtifactFlags ||
                            pOutdoor->mm6DeltaBytes != savedRaw.chest.rawActiveDelta ||
                            vChests.size() != 20)
                            throw std::runtime_error("MM6 v5 final CHST owner changed");
                        mm6::ChestRecordSet chestRecords{};
                        if (!mm6::copyChestRecords(outdoorWire.part(mm6::OutdoorDeltaPart::Chests),
                                                   &chestRecords))
                            throw std::runtime_error("MM6 v5 final chest section is incomplete");
                        for (int index = 0; index < 20; ++index) {
                            mm6::ChestRuntimeProjection projection{vChests[index], chestRecords[index]};
                            const auto replayed = mm6::stageChestRuntimeRecord(
                                GameTitle::MM6, projection);
                            if (!replayed)
                                throw std::runtime_error("MM6 v5 final chest projection refused at index " +
                                                         std::to_string(index));
                            if (*replayed != chestRecords[index])
                                throw std::runtime_error("MM6 v5 final chest projection differed at index " +
                                                         std::to_string(index));
                        }
                        const auto finalCore = mm6::captureCoreFields(*pParty, GameTitle::MM6);
                        if (finalCore != savedRaw.core)
                            throw std::runtime_error("MM6 v5 PART/CHAR owner changed after map events");
                        const auto durable = mm6::captureDurableFields(*pParty, GameTitle::MM6);
                        if (durable.bits != savedRaw.bits || durable.effects != savedRaw.effects)
                            throw std::runtime_error("MM6 v5 QEST/EFFT owner changed after map events");
                        if (mm6::captureResistanceFields(*pParty, GameTitle::MM6) != savedRaw.resistances ||
                            mm6::captureBuffFields(*pParty, GameTitle::MM6) != savedRaw.buffs ||
                            mm6::captureRecoveryFields(*pParty, GameTitle::MM6) != savedRaw.recovery)
                            throw std::runtime_error("MM6 v5 Party resistance, buff or recovery owner changed");
                        if (mm6::encodeHirelingSection(
                                mm6::captureHirelingFields(*pParty, GameTitle::MM6)) !=
                            mm6::encodeHirelingSection(savedRaw.hirelings))
                            throw std::runtime_error("MM6 v5 HIRE owner changed after map events");
                        mm6::StagedRawPartyInventories liveInventories{*pParty, savedRaw.inventory};
                        if (mm6::captureRawPartyInventories(liveInventories, *pItemTable,
                                                             GameTitle::MM6) != savedRaw.inventory)
                            throw std::runtime_error("MM6 v5 INVR owner changed after map events");
                        mm6::requirePostWorldEmptyCursorReady(
                            staged->v4.v3.cursorImage, *pParty, *mouse,
                            engine->config->graphics.AlwaysCustomCursor.value(),
                            GameTitle::MM6);
                        mm6::requirePostWorldArmageddonResumeAgreement(
                            *pParty, staged->v4.v3.armageddon, LEVEL_OUTDOOR,
                            GameTitle::MM6);
                        finalOwnersAgree = true;
                    } catch (const std::exception &error) {
                        finalOwnerError = error.what();
                    }
                    if (allowInput && !finalOwnersAgree)
                        throw std::runtime_error("MM6 later outdoor first-input admission refused: " +
                                                 finalOwnerError);
                }, allowInput ? mm6::testing::PostWorldAdmissionResult::Continue :
                                mm6::testing::PostWorldAdmissionResult::AbortToMainMenu);
            selected = true;
        });
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });

    game.skipLoadingScreen();
    game.tick(allowInput ? 3 : 2);
    if (allowInput && finalOwnersAgree) {
        game.runGameRoutine([&] {
            firstInputReached = checkpoint->reachedInput() &&
                !gameTimer->isPaused() &&
                !(engineFlags & ENGINE_SKIP_NEXT_WORLD_UPDATE) &&
                !(engineFlags & ENGINE_SKIP_NEXT_USER_INPUT) &&
                engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute2.odm") &&
                uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                !engine->_pendingTransition &&
                pPartyActionQueue->uNumActions == 0;
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
                    engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute2.odm") &&
                    !engine->_pendingTransition;
                if (movementApplied && mode == LaterReplayMode::SecondWrite) {
                    mm6::testing::writeV5LaterOutdoorAfterInputCandidateForTest(
                        profile, v4Before, before);
                    secondCandidateWritten = true;
                }
            });
        }
        game.runGameRoutine([] {
            uGameState = GAME_STATE_GAME_QUITTING_TO_MAIN_MENU;
        });
        game.tick(2);
    }
    game.runGameRoutine([&] {
        postWorld.reset();
        spriteReplay.reset();
        decorationReplay.reset();
        actorReplay.reset();
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
    if (v6Read) {
        EXPECT_TRUE(visitedPreflighted);
        EXPECT_TRUE(visitedPostWorldValidated);
        EXPECT_TRUE(returnPlanRefusedWithoutArrival);
        EXPECT_TRUE(returnPlanStayedDetached);
        EXPECT_TRUE(returnArrivalStayedDetached);
    }
    if (allowInput) {
        EXPECT_TRUE(firstInputReached);
        EXPECT_TRUE(movementApplied);
    }
    if (mode == LaterReplayMode::SecondWrite) {
        EXPECT_TRUE(secondCandidateWritten);
        const auto second = profile / "saves/candidate-v5-oute2-after-input.ec6";
        ASSERT_TRUE(std::filesystem::is_regular_file(second));
        EXPECT_FALSE(slotBytes(second).empty());
    }
    EXPECT_EQ(slotBytes(candidate), wireBefore);
    EXPECT_EQ(slotBytes(profile / "saves" / "candidate-v5-oute2.ec6"), priorV5Before);
    EXPECT_EQ(slotBytes(profile / "saves" / "candidate-v4.ec6"), v4Before);
    EXPECT_EQ(slotBytes(profile / "saves" / "autosave.ec6"), seedBefore);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

GAME_TEST(MM6V5LaterOutdoorFreshChildHeadless, DISABLED_SecondOutdoorRestoresOwnersBeforeInputAndAborts) {
    runV5LaterOutdoorFreshChildReplay(game, LaterReplayMode::AbortBeforeInput);
}

GAME_TEST(MM6V5LaterOutdoorFreshChildHeadless, DISABLED_SecondOutdoorAdmitsBoundedFirstInput) {
    runV5LaterOutdoorFreshChildReplay(game, LaterReplayMode::FirstInput);
}

GAME_TEST(MM6V5LaterOutdoorFreshChildHeadless, DISABLED_SecondOutdoorWritesAfterReloadedInput) {
    runV5LaterOutdoorFreshChildReplay(game, LaterReplayMode::SecondWrite);
}

GAME_TEST(MM6V5LaterOutdoorFreshChildHeadless, DISABLED_SecondOutdoorAfterInputRestoresOwnersInNewChild) {
    runV5LaterOutdoorFreshChildReplay(game, LaterReplayMode::SecondReplay);
}

GAME_TEST(MM6OutdoorV6FreshChildHeadless, DISABLED_ActiveAndVisitedOwnersStageBeforeInputAndAbort) {
    runV5LaterOutdoorFreshChildReplay(game, LaterReplayMode::V6VisitedRead);
}
