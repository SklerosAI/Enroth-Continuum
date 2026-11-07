#include <cstddef>
#include <cstdlib>
#include <cstdint>
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
#include "Engine/MM6/MM6V3SaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
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
        throw std::runtime_error("MM6 v3 replay needs an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 v3 replay needs an owner-only system-temp profile");
    return root;
}

std::filesystem::path v3CandidateInProfile(const std::filesystem::path &profile) {
    const char *value = std::getenv("ENROTH_MM6_V3_REPLAY_SLOT");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_regular_file(supplied))
        throw std::runtime_error("MM6 v3 replay needs an existing absolute candidate");
    const std::filesystem::path candidate = std::filesystem::canonical(supplied);
    if (candidate.parent_path() != std::filesystem::canonical(profile / "saves") ||
        candidate.extension() != ".ec6" ||
        candidate.filename() == mm6::NEW_GAME_AUTOSAVE_SLOT)
        throw std::runtime_error("MM6 v3 replay candidate must be a separate profile save");
    return candidate;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    constexpr std::uintmax_t maxSaveBytes = 64 * 1024 * 1024;
    if (std::filesystem::file_size(path) > maxSaveBytes)
        throw std::runtime_error("MM6 v3 replay candidate exceeds the EC6 bound");
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

// Run only after a separate strict post-entry v3 writer and owner admission
// produce ENROTH_MM6_V3_REPLAY_SLOT. This intentionally stays disabled while
// no exact v3 slot exists. It never enables production SaveLoad or input.
GAME_TEST(MM6V3PreEventReplayHeadless, DISABLED_FreshQuickLoadRestoresBeforeEventsAndAborts) {
    const char *enable = std::getenv("ENROTH_MM6_V3_REPLAY_ENABLE");
    if (!enable || std::string_view(enable) != "strict-v3")
        GTEST_SKIP() << "Run only after strict v3 writer and owner admission";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    const std::filesystem::path candidate = v3CandidateInProfile(profile);
    if (candidate.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_V3_REPLAY_SLOT to a strict v3 candidate";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto before = slotBytes(candidate);
    ASSERT_FALSE(before.empty());

    bool selected = false;
    bool checkpointRan = false;
    bool freshMapBeforeReplay = false;
    bool postWorldRan = false;
    bool finalOwnersAgree = false;
    std::string finalOwnerError;
    std::size_t expectedNativeActors = 0;
    std::size_t expectedActiveActors = 0;
    mm6::GeneratedNpcSaveFields expectedGenerated;
    std::unique_ptr<mm6::V3FullSessionPreflight> staged;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    std::unique_ptr<mm6::testing::ScopedV3PreEventReplayForTest> replay;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> postWorld;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view requested) {
            if (requested != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 v3 replay selected another menu route");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto candidateStage = mm6::stageV3FullSessionPreflight(
                before, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), engine->config->gameplay.MaxActors.value(),
                application->component<EngineRandomComponent>()->type(), GameTitle::MM6);
            if (candidateStage.actorMap.activeMap.mapName != "oute3.odm" ||
                mm6::encodeV3Save(candidateStage.actorMap.saved, GameTitle::MM6) != before ||
                candidateStage.armageddon.timerTicks != 0 ||
                candidateStage.cursorImage.occupied)
                throw std::runtime_error("MM6 v3 replay has no exact first-map candidate");
            staged = std::make_unique<mm6::V3FullSessionPreflight>(std::move(candidateStage));
            expectedNativeActors = staged->actorMap.activeMap.delta.actors.size();
            expectedActiveActors = staged->actorMap.activeActors.size();
            expectedGenerated = staged->actorMap.saved.generatedNpcs;

            *pParty = staged->represented.party.party;
            *pNPCStats = staged->represented.world.world.npcs;
            *gameTimer = staged->represented.world.visuals.timer;
            *pActiveOverlayList = staged->represented.world.visuals.overlays;
            pMapDeltas = std::move(staged->represented.world.world.mapDeltas);
            mm6::chestTitleState() = staged->represented.world.chestTitle;
            mm6::publishFreshChildPersistentEventZeros(
                &engine->_persistentVariables,
                staged->represented.persistentEvents, GameTitle::MM6);
            mm6::publishRawCursorShopOwners(
                &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                staged->represented.raw.saved,
                {std::move(staged->represented.raw.cursor),
                 std::move(staged->represented.raw.shops)}, GameTitle::MM6);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>();
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
                GameTitle::MM6, std::move(staged->actorMap));
            postWorld = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                GameTitle::MM6, [&] {
                    postWorldRan = true;
                    try {
                        if (!checkpoint->reachedMapLoad() || checkpoint->reachedInput() ||
                            !replay->published() || !replay->rngPublished() ||
                            pActors.size() != expectedActiveActors ||
                            !gameTimer->isPaused() || engine->_pendingTransition ||
                            pPartyActionQueue->uNumActions != 0)
                            throw std::runtime_error("MM6 v3 replay reached an unexpected world order");
                        mm6::requireGeneratedNpcActorAgreement(
                            expectedGenerated, pActors, "oute3.odm", GameTitle::MM6);
                        const auto observedGenerated = mm6::captureGeneratedNpcFields(
                            pActors, *pNPCStats, "oute3.odm", GameTitle::MM6);
                        if (mm6::encodeGeneratedNpcSection(observedGenerated, GameTitle::MM6) !=
                            mm6::encodeGeneratedNpcSection(expectedGenerated, GameTitle::MM6))
                            throw std::runtime_error("MM6 v3 generated NPC rows changed after map events");
                        const std::vector<Actor> finalActors(pActors.begin(), pActors.end());
                        const mm6::PostWorldOwnerView owners{
                            engine->_currentLoadedMapId, uCurrentlyLoadedLevelType,
                            pMapTable, pItemTable, finalActors, pSpriteObjects, vChests,
                            pOutdoor->mm6DeltaBytes, &engine->_persistentVariables,
                            &mm6::chestTitleState()};
                        mm6::requireRawV2PostWorldOwnerAgreement(
                            staged->represented, owners, GameTitle::MM6);
                        mm6::requirePostWorldEmptyCursorReady(
                            staged->cursorImage, *pParty, *mouse,
                            engine->config->graphics.AlwaysCustomCursor.value(),
                            GameTitle::MM6);
                        mm6::requirePostWorldArmageddonResumeAgreement(
                            *pParty, staged->armageddon, LEVEL_OUTDOOR,
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
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
