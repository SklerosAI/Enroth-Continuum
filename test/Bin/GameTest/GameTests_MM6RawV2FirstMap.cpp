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
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6PostWorldAdmissionForTest.h"
#include "Engine/MM6/MM6PostWorldOwnerValidator.h"
#include "Engine/MM6/MM6RawChildPublicationForTest.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "GUI/GUIMessageQueue.h"
#include "Io/Mouse.h"

namespace {

std::filesystem::path disposableProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 v2 world probe needs an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 v2 world probe needs an owner-only system-temp profile");
    return root;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

// Run alone after DISABLED_WriteRealStarterV2RestartSlot in a fresh child.
// The only publication is inside this disposable process, and the one-shot
// hook always aborts before timer unpause or input. Production SaveLoad stays
// guarded even when all represented owners match.
GAME_TEST(MM6RawV2FirstMapHeadless, DISABLED_BootstrapQuickLoadRefusesRespawnBeforeInput) {
    const std::filesystem::path profile = disposableProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only v2 profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path primary = profile / "saves" / "autosave.ec6";
    ASSERT_TRUE(std::filesystem::is_regular_file(primary));
    const auto before = slotBytes(primary);
    ASSERT_FALSE(before.empty());

    bool callbackRan = false;
    bool postWorldRan = false;
    bool rebuiltFirstMap = false;
    bool finalOwnerCheckPassed = false;
    bool cursorReady = false;
    std::string finalOwnerRefusal;
    std::string cursorRefusal;
    std::unique_ptr<mm6::StagedRawSessionFields> staged;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> admission;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view slotName) {
            if (slotName != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 v2 probe selected another slot");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto candidate = mm6::preflightStagedRawSessionFields(
                profile, slotName, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), GameTitle::MM6);
            if (!candidate || candidate->needsRecovery || !candidate->outdoorPlan ||
                candidate->indoorPlan || candidate->outdoorPlan->mapName != "oute3.odm" ||
                mm6::encodeRawSave(candidate->raw.saved, GameTitle::MM6) != before)
                throw std::runtime_error("MM6 v2 probe did not select the exact first-map primary");
            const auto preflight = mm6::testing::inspectRawChildPublication(
                *candidate, *pItemTable, GameTitle::MM6);
            if (preflight.canEnterGameLoop() || preflight.cursorImage.occupied ||
                candidate->party.party.armageddon_timer.ticks())
                throw std::runtime_error("MM6 v2 probe violates the bounded empty-cursor state");

            staged = std::make_unique<mm6::StagedRawSessionFields>(std::move(*candidate));
            *pParty = staged->party.party;
            *pNPCStats = staged->world.world.npcs;
            *gameTimer = staged->world.visuals.timer;
            *pActiveOverlayList = staged->world.visuals.overlays;
            pMapDeltas = std::move(staged->world.world.mapDeltas);
            mm6::chestTitleState() = staged->world.chestTitle;
            mm6::publishFreshChildPersistentEventZeros(
                &engine->_persistentVariables, staged->persistentEvents, GameTitle::MM6);
            mm6::publishRawCursorShopOwners(
                &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                staged->raw.saved,
                {std::move(staged->raw.cursor), std::move(staged->raw.shops)}, GameTitle::MM6);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>();
            admission = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                GameTitle::MM6, [&] {
                    postWorldRan = true;
                    rebuiltFirstMap = gameTimer->isPaused() &&
                        !engine->_pendingTransition &&
                        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                        uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                        !pActors.empty() && vChests.size() == 20 &&
                        !pOutdoor->mm6DeltaBytes.empty() &&
                        pPartyActionQueue->uNumActions == 0;
                    const std::vector<Actor> finalActors(pActors.begin(), pActors.end());
                    const mm6::PostWorldOwnerView owners{
                        engine->_currentLoadedMapId, uCurrentlyLoadedLevelType,
                        pMapTable, pItemTable, finalActors, pSpriteObjects, vChests,
                        pOutdoor->mm6DeltaBytes, &engine->_persistentVariables,
                        &mm6::chestTitleState()};
                    try {
                        mm6::requireRawV2PostWorldOwnerAgreement(
                            *staged, owners, GameTitle::MM6);
                        finalOwnerCheckPassed = true;
                    } catch (const std::exception &error) {
                        finalOwnerRefusal = error.what();
                        const auto &plan = *staged->outdoorPlan;
                        finalOwnerRefusal += " actors saved/sidecar/live=" +
                            std::to_string(plan.savedDelta.actors.size()) + "/" +
                            std::to_string(plan.activeSidecar.actors.size()) + "/" +
                            std::to_string(pActors.size());
                        finalOwnerRefusal += " sprites saved/sidecar/live=" +
                            std::to_string(plan.savedDelta.spriteObjects.size()) + "/" +
                            std::to_string(plan.activeSidecar.spriteObjects.size()) + "/" +
                            std::to_string(pSpriteObjects.size());
                    }
                    try {
                        const auto cursorPlan = mm6::stageRawCursorImagePlan(
                            *pParty, mm6::rawHeldCursorState(GameTitle::MM6),
                            *pItemTable, GameTitle::MM6);
                        mm6::requirePostWorldEmptyCursorReady(
                            cursorPlan, *pParty, *mouse,
                            engine->config->graphics.AlwaysCustomCursor.value(),
                            GameTitle::MM6);
                        cursorReady = true;
                    } catch (const std::exception &error) {
                        cursorRefusal = error.what();
                    }
                }, mm6::testing::PostWorldAdmissionResult::AbortToMainMenu);
            callbackRan = true;
        });
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });

    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] {
        admission.reset();
        chestOptIn.reset();
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(false);
        mm6::testing::setLoadGameOverride({});
    });
    EXPECT_TRUE(callbackRan);
    EXPECT_TRUE(postWorldRan);
    EXPECT_TRUE(rebuiltFirstMap);
    EXPECT_FALSE(finalOwnerCheckPassed);
    EXPECT_TRUE(finalOwnerRefusal.starts_with("MM6 rebuilt PID slot counts differ"))
        << finalOwnerRefusal;
    EXPECT_TRUE(cursorReady) << cursorRefusal;
    EXPECT_EQ(slotBytes(primary), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
