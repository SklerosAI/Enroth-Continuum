#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6EventRuntimeRestoreForTest.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6PostWorldAdmissionForTest.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6V3PreEventReplayForTest.h"
#include "Engine/MM6/MM6V5FirstMapLoadPreflight.h"
#include "Engine/MM6/MM6V5FullSessionPreflight.h"
#include "Engine/MM6/MM6V5LiveSaveCapture.h"
#include "Engine/MM6/MM6V5PreEventActorReplayForTest.h"
#include "Engine/MM6/MM6V5PreEventDecorationReplayForTest.h"
#include "Engine/MM6/MM6V5PreEventSpriteReplayForTest.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Timer.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"
#include "Library/Platform/Application/PlatformApplication.h"

static std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path root = std::filesystem::canonical(std::filesystem::path(value));
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 normal load needs an owner-only system-temp profile");
    return root;
}

static std::vector<std::uint8_t> fileBytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

static void writeBytes(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

GAME_TEST(MM6V5NormalLoadMenuHeadless, DISABLED_LoadMenuListsAndLoadsNumberedFirstMapSlot) {
    // The MM6 Load menu lists only loadable numbered slots, and the slot saved from the Save menu loads in a fresh
    // child with its party, map and owners intact before and after the first input.
    const char *enable = std::getenv("ENROTH_MM6_V5_NORMAL_LOAD_ENABLE");
    if (!enable || std::string_view(enable) != "strict-v5-normal-load")
        GTEST_SKIP() << "Run only after the normal Save menu child";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    const auto saved = fileBytes(slot);
    const auto savedBackup = fileBytes(profile / "saves/save000.ec6.bak");
    ASSERT_FALSE(saved.empty());
    ASSERT_FALSE(savedBackup.empty());
    const std::vector<std::uint8_t> malformed = {1, 2, 3};
    writeBytes(profile / "saves/save001.mm7", saved);
    writeBytes(profile / "saves/save002.ec6", malformed);
    mm6::CorePartyFields savedParty;
    game.runGameRoutine([&] {
        const auto savedFields = std::unique_ptr<mm6::V5SaveFields>(
            new mm6::V5SaveFields(mm6::decodeV5Save(saved, GameTitle::MM6)));
        savedParty = savedFields->v4.v3.raw.core.party;
    });

    std::string requested;
    std::string finalOwnerError;
    bool finalOwnersAgree = false;
    bool restoredBeforeInput = false;
    bool firstInputReached = false;
    bool movementApplied = false;
    std::size_t expectedNativeActors = 0;
    std::unique_ptr<mm6::V5FullSessionPreflight> staged;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedEventRuntimeRestoreForTest> eventRuntime;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> checkpoint;
    std::unique_ptr<mm6::testing::ScopedV3PreEventReplayForTest> replay;
    std::unique_ptr<mm6::testing::ScopedV5PreEventActorReplayForTest> actorReplay;
    std::unique_ptr<mm6::testing::ScopedV5PreEventDecorationReplayForTest> decorationReplay;
    std::unique_ptr<mm6::testing::ScopedV5PreEventSpriteReplayForTest> spriteReplay;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> postWorld;

    // Test-only publication. V5FirstMapLoadAttempt admits only its disposable candidate slot.
    const auto loadSlot = [&](std::string_view fileName) {
        requested = fileName;
        staged = mm6::stageV5FirstMapLoadPreflight(mm6::readFirstMapManualSlot(fileName), GameTitle::MM6);
        mm6::requireV5FirstMapPrepublicationAgreement(*staged, GameTitle::MM6);
        expectedNativeActors = staged->v4.v3.actorMap.activeMap.delta.actors.size();
        chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(true);
        eventRuntime = std::make_unique<mm6::testing::ScopedEventRuntimeRestoreForTest>(
            GameTitle::MM6, staged->v4.saved.eventRuntime);
        checkpoint = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(GameTitle::MM6, [] {});
        replay = std::make_unique<mm6::testing::ScopedV3PreEventReplayForTest>(
            GameTitle::MM6, std::move(staged->v4.v3.actorMap));
        actorReplay = std::make_unique<mm6::testing::ScopedV5PreEventActorReplayForTest>(
            GameTitle::MM6, staged->actors, staged->v4.saved.v3.activeActorPayload);
        decorationReplay = std::make_unique<mm6::testing::ScopedV5PreEventDecorationReplayForTest>(
            GameTitle::MM6, staged->decorations);
        spriteReplay = std::make_unique<mm6::testing::ScopedV5PreEventSpriteReplayForTest>(
            GameTitle::MM6, staged->sprites);
        postWorld = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
            GameTitle::MM6, mm6::testing::FailClosedDecisionTag{}, [&] {
                try {
                    if (!replay->published() || !replay->rngPublished() || !actorReplay->published() ||
                        !decorationReplay->published() || !spriteReplay->published() || !eventRuntime->restored())
                        throw std::runtime_error("MM6 normal load skipped a replay phase");
                    mm6::requireV5FirstMapPostWorldAgreement(*staged, expectedNativeActors);
                    const auto party = mm6::captureCoreFields(*pParty, GameTitle::MM6).party;
                    restoredBeforeInput = party.position == savedParty.position &&
                        party.viewYaw == savedParty.viewYaw &&
                        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm");
                    finalOwnersAgree = true;
                } catch (const std::exception &error) {
                    finalOwnerError = error.what();
                }
                return finalOwnersAgree;
            });
        *pParty = staged->v4.v3.represented.party.party;
        *pNPCStats = staged->v4.v3.represented.world.world.npcs;
        *gameTimer = staged->v4.v3.represented.world.visuals.timer;
        *pActiveOverlayList = staged->v4.v3.represented.world.visuals.overlays;
        pMapDeltas = std::move(staged->v4.v3.represented.world.world.mapDeltas);
        mm6::chestTitleState() = staged->v4.v3.represented.world.chestTitle;
        mm6::publishFreshChildPersistentEventZeros(
            &engine->_persistentVariables, staged->v4.v3.represented.persistentEvents, GameTitle::MM6);
        mm6::publishRawCursorShopOwners(
            &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
            staged->v4.v3.represented.raw.saved,
            {std::move(staged->v4.v3.represented.raw.cursor), std::move(staged->v4.v3.represented.raw.shops)},
            GameTitle::MM6);
        engine->_pendingTransition = MapDestination(pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
    };

    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });
    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 1);
    EXPECT_EQ(saveLoadMenu()->slots()[0].fileName, "save000.ec6");
    EXPECT_EQ(saveLoadMenu()->slots()[0].header.name, "ab");
    EXPECT_EQ(saveLoadMenu()->slots()[0].header.locationName, "oute3.odm");
    game.pressGuiButton("LoadMenu_Slot0");
    game.tick(2);

    // Without a load route the selected slot stays in the menu.
    game.pressGuiButton("LoadMenu_Load");
    game.tick(3); // The button animates for a frame, then posts the load for the next one.
    EXPECT_EQ(current_screen_type, SCREEN_LOADGAME);
    EXPECT_TRUE(engine->_pendingLoadFileName.empty());
    EXPECT_TRUE(requested.empty());

    game.runGameRoutine([&] { mm6::testing::setLoadGameOverride(loadSlot); });
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(3);
    if (finalOwnersAgree) {
        game.runGameRoutine([&] {
            firstInputReached = checkpoint->reachedInput() && !gameTimer->isPaused() &&
                !(engineFlags & ENGINE_SKIP_NEXT_WORLD_UPDATE) && !(engineFlags & ENGINE_SKIP_NEXT_USER_INPUT) &&
                engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                !engine->_pendingTransition && pPartyActionQueue->uNumActions == 0;
            if (firstInputReached) {
                pPartyActionQueue->Add(PARTY_TurnLeft);
                pPartyActionQueue->Add(PARTY_WalkForward);
            }
        });
        if (firstInputReached) {
            const auto beforePosition = pParty->pos;
            const auto beforeYaw = pParty->_viewYaw;
            game.tick(3); // The party turns and walks one step.
            game.runGameRoutine([&] {
                movementApplied = pParty->pos != beforePosition && pParty->_viewYaw != beforeYaw &&
                    engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                    !engine->_pendingTransition;
            });
        }
        game.runGameRoutine([] { uGameState = GAME_STATE_GAME_QUITTING_TO_MAIN_MENU; });
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
        staged.reset();
        mm6::testing::setLoadGameOverride({});
        mm6::testing::setNativeProfileRootForTest({});
    });
    EXPECT_EQ(requested, "save000.ec6");
    EXPECT_TRUE(finalOwnersAgree) << finalOwnerError;
    EXPECT_TRUE(restoredBeforeInput);
    EXPECT_TRUE(firstInputReached);
    EXPECT_TRUE(movementApplied);
    EXPECT_EQ(fileBytes(slot), saved);
    EXPECT_EQ(fileBytes(profile / "saves/save000.ec6.bak"), savedBackup);
    EXPECT_EQ(fileBytes(profile / "saves/save002.ec6"), malformed);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
