#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
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
#include "Engine/MM6/MM6V5FirstMapLoadPreflight.h"
#include "Engine/MM6/MM6V5LiveSaveCapture.h"
#include "Engine/MM6/MM6V5ActorRuntimeFields.h"
#include "Engine/MM6/MM6V5DecorationRuntimeFields.h"
#include "Engine/MM6/MM6V5PreEventActorReplayForTest.h"
#include "Engine/MM6/MM6V5PreEventDecorationReplayForTest.h"
#include "Engine/MM6/MM6V5PreEventSpriteReplayForTest.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/MM6/MM6V5SpriteRuntimeFields.h"
#include "MM6V4AfterActionCaptureForTest.h"
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

namespace {

std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 v5 post-action replay needs an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 v5 post-action replay needs an owner-only system-temp profile");
    return root;
}

std::filesystem::path postActionCandidateInProfile(const std::filesystem::path &profile) {
    const std::filesystem::path supplied = profile / "saves" / "candidate-v5-after-action.ec6";
    if (!std::filesystem::is_regular_file(supplied))
        throw std::runtime_error("MM6 v5 post-action replay needs an existing absolute candidate");
    const std::filesystem::path candidate = std::filesystem::canonical(supplied);
    if (candidate.parent_path() != std::filesystem::canonical(profile / "saves") ||
        candidate.filename() != "candidate-v5-after-action.ec6")
        throw std::runtime_error("MM6 v5 post-action replay candidate must be a separate profile save");
    return candidate;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    constexpr std::uintmax_t maxSaveBytes = 64 * 1024 * 1024;
    if (std::filesystem::file_size(path) > maxSaveBytes)
        throw std::runtime_error("MM6 v5 post-action replay candidate exceeds the EC6 bound");
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

mm6::CorePartyFields baselinePartyFields(std::span<const std::uint8_t> bytes) {
    const auto decoded = std::unique_ptr<mm6::V4SaveFields>(
        new mm6::V4SaveFields(mm6::decodeV4Save(bytes, GameTitle::MM6)));
    return decoded->v3.raw.core.party;
}

mm6::CorePartyFields actionPartyFields(std::span<const std::uint8_t> bytes) {
    const auto decoded = std::unique_ptr<mm6::V5SaveFields>(
        new mm6::V5SaveFields(mm6::decodeV5Save(bytes, GameTitle::MM6)));
    return decoded->v4.v3.raw.core.party;
}


} // namespace

// Both probes consume the separate post-action candidate in an owner-only
// disposable profile. The continuation admits only one bounded input and
// movement step after every saved owner has passed the existing replay gate.
enum class V5ReplayMode { AbortBeforeInput, FirstInput, SecondWrite };

void runV5AfterActionFreshChildReplay(EngineController &game, V5ReplayMode mode) {
    const bool allowInput = mode != V5ReplayMode::AbortBeforeInput;
    const char *enableName = mode == V5ReplayMode::AbortBeforeInput ?
        "ENROTH_MM6_V5_AFTER_ACTION_REPLAY_ENABLE" :
        mode == V5ReplayMode::FirstInput ?
            "ENROTH_MM6_V5_FIRST_INPUT_ENABLE" :
            "ENROTH_MM6_V5_SECOND_WRITE_ENABLE";
    const char *enable = std::getenv(enableName);
    const std::string_view expectedEnable = mode == V5ReplayMode::SecondWrite ?
        "strict-v5-second-write" : "strict-v5-after-action";
    if (!enable || std::string_view(enable) != expectedEnable)
        GTEST_SKIP() << "Run only after strict v5 writer and owner admission";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    const std::filesystem::path candidate = postActionCandidateInProfile(profile);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto before = slotBytes(candidate);
    const auto baselineBefore = slotBytes(profile / "saves" / "candidate-v4.ec6");
    const auto seedBefore = slotBytes(profile / "saves" / "autosave.ec6");
    ASSERT_FALSE(before.empty());
    ASSERT_FALSE(baselineBefore.empty());
    ASSERT_FALSE(seedBefore.empty());
    const auto baselineParty = baselinePartyFields(baselineBefore);
    const auto actionParty = actionPartyFields(before);
    ASSERT_NE(actionParty.position, baselineParty.position);
    ASSERT_NE(actionParty.viewYaw, baselineParty.viewYaw);

    bool selected = false;
    bool checkpointRan = false;
    bool freshMapBeforeReplay = false;
    bool postWorldRan = false;
    bool finalOwnersAgree = false;
    bool firstInputReached = false;
    bool movementApplied = false;
    bool secondCandidateWritten = false;
    std::string finalOwnerError;
    std::size_t expectedNativeActors = 0;
    std::size_t expectedActiveActors = 0;
    std::unique_ptr<mm6::V5FullSessionPreflight> staged;
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
                throw std::runtime_error("MM6 v5 post-action replay selected another menu route");
            auto candidateStage = mm6::stageV5FirstMapLoadPreflight(
                before, GameTitle::MM6);
            if (candidateStage->v4.v3.actorMap.activeMap.mapName != "oute3.odm" ||
                candidateStage->v4.saved.v3.raw.core.party.position != actionParty.position ||
                candidateStage->v4.saved.v3.raw.core.party.viewYaw != actionParty.viewYaw ||
                candidateStage->v4.v3.armageddon.timerTicks != 0 ||
                candidateStage->v4.v3.cursorImage.occupied)
                throw std::runtime_error("MM6 v5 post-action replay has no exact first-map candidate");
            staged = std::move(candidateStage);
            expectedNativeActors = staged->v4.v3.actorMap.activeMap.delta.actors.size();
            expectedActiveActors = staged->actors.actors.size();

            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(true);
            eventRuntime = std::make_unique<mm6::testing::ScopedEventRuntimeRestoreForTest>(
                GameTitle::MM6, staged->v4.saved.eventRuntime);
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
                GameTitle::MM6, std::move(staged->v4.v3.actorMap));
            actorReplay = std::make_unique<mm6::testing::ScopedV5PreEventActorReplayForTest>(
                GameTitle::MM6, staged->actors,
                staged->v4.saved.v3.activeActorPayload);
            decorationReplay = std::make_unique<mm6::testing::ScopedV5PreEventDecorationReplayForTest>(
                GameTitle::MM6, staged->decorations);
            spriteReplay = std::make_unique<mm6::testing::ScopedV5PreEventSpriteReplayForTest>(
                GameTitle::MM6, staged->sprites);
            postWorld = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                GameTitle::MM6, mm6::testing::FailClosedDecisionTag{}, [&] {
                    postWorldRan = true;
                    try {
                        if (!checkpoint->reachedMapLoad() || checkpoint->reachedInput() ||
                            !replay->published() || !replay->rngPublished() ||
                            !actorReplay->published() || !decorationReplay->published() ||
                            !spriteReplay->published() ||
                            !eventRuntime->restored() ||
                            pActors.size() != expectedActiveActors ||
                            expectedActiveActors < expectedNativeActors ||
                            engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute3.odm") ||
                            uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
                            !gameTimer->isPaused() || engine->_pendingTransition ||
                            pPartyActionQueue->uNumActions != 0)
                            throw std::runtime_error("MM6 v5 post-action replay reached an unexpected world order");
                        mm6::requireV5FirstMapPostWorldAgreement(
                            *staged, expectedNativeActors);
                        const auto finalCore = mm6::captureCoreFields(*pParty, GameTitle::MM6);
                        if (finalCore.party.position == baselineParty.position ||
                            finalCore.party.viewYaw == baselineParty.viewYaw)
                            throw std::runtime_error("MM6 v5 post-action movement was not restored");
                        finalOwnersAgree = true;
                    } catch (const std::exception &error) {
                        finalOwnerError = error.what();
                    }
                    return allowInput && finalOwnersAgree;
                });
            // Every throwing replay-scope constructor runs before the first
            // live owner assignment. Post-world callbacks execute later.
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
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
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
                engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
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
                    engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                    !engine->_pendingTransition;
                if (movementApplied && mode == V5ReplayMode::SecondWrite) {
                    mm6::testing::writeV5AfterReloadedInputCandidateForTest(
                        profile, baselineBefore, before);
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
    if (allowInput) {
        EXPECT_TRUE(firstInputReached);
        EXPECT_TRUE(movementApplied);
    }
    if (mode == V5ReplayMode::SecondWrite) {
        EXPECT_TRUE(secondCandidateWritten);
        const auto secondCandidate = profile / "saves" / "candidate-v5-after-reload-input.ec6";
        ASSERT_TRUE(std::filesystem::is_regular_file(secondCandidate));
        EXPECT_FALSE(slotBytes(secondCandidate).empty());
    }
    EXPECT_EQ(slotBytes(candidate), before);
    EXPECT_EQ(slotBytes(profile / "saves" / "candidate-v4.ec6"), baselineBefore);
    EXPECT_EQ(slotBytes(profile / "saves" / "autosave.ec6"), seedBefore);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

GAME_TEST(MM6V5AfterActionFreshChildReplayHeadless, DISABLED_PostActionQuickLoadRestoresSpritesBeforeInputAndAborts) {
    runV5AfterActionFreshChildReplay(game, V5ReplayMode::AbortBeforeInput);
}

GAME_TEST(MM6V5FirstInputHeadless, DISABLED_PostActionQuickLoadMovesOnceInRealLoop) {
    runV5AfterActionFreshChildReplay(game, V5ReplayMode::FirstInput);
}

GAME_TEST(MM6V5SecondWriteHeadless, DISABLED_PostReloadInputWritesDistinctCandidate) {
    runV5AfterActionFreshChildReplay(game, V5ReplayMode::SecondWrite);
}
