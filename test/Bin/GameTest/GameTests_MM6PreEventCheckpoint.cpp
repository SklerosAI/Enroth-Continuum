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
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6PreEventCheckpointForTest.h"
#include "Engine/MM6/MM6RawChildPublicationForTest.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/Actor.h"
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

namespace {

std::filesystem::path disposableProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 pre-event probe requires an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 pre-event probe requires an owner-only system-temp profile");
    return root;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

// Run alone in a fresh child after the real-starter v2 writer. This checks
// callback ordering and abort; it does not publish GNPC, RNGS or ACTR.
GAME_TEST(MM6PreEventCheckpointHeadless, DISABLED_FreshQuickLoadAbortsBeforeNpcAndEvents) {
    const std::filesystem::path profile = disposableProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to a disposable v2 profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path primary = profile / "saves" / "autosave.ec6";
    ASSERT_TRUE(std::filesystem::is_regular_file(primary));
    const auto before = slotBytes(primary);
    ASSERT_FALSE(before.empty());

    bool staged = false;
    bool checkpoint = false;
    bool rebuiltBeforeNpc = false;
    std::unique_ptr<mm6::testing::ScopedPreEventCheckpointForTest> preEvent;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view slotName) {
            if (slotName != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 pre-event probe selected another slot");
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
                throw std::runtime_error("MM6 pre-event probe did not stage its exact primary");
            const auto preflight = mm6::testing::inspectRawChildPublication(
                *candidate, *pItemTable, GameTitle::MM6);
            if (preflight.canEnterGameLoop())
                throw std::runtime_error("MM6 pre-event probe unexpectedly admits gameplay");

            *pParty = candidate->party.party;
            *pNPCStats = candidate->world.world.npcs;
            *gameTimer = candidate->world.visuals.timer;
            *pActiveOverlayList = candidate->world.visuals.overlays;
            pMapDeltas = std::move(candidate->world.world.mapDeltas);
            mm6::chestTitleState() = candidate->world.chestTitle;
            mm6::publishFreshChildPersistentEventZeros(
                &engine->_persistentVariables, candidate->persistentEvents, GameTitle::MM6);
            mm6::publishRawCursorShopOwners(
                &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                candidate->raw.saved,
                {std::move(candidate->raw.cursor), std::move(candidate->raw.shops)}, GameTitle::MM6);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            preEvent = std::make_unique<mm6::testing::ScopedPreEventCheckpointForTest>(
                GameTitle::MM6, [&] {
                    checkpoint = true;
                    rebuiltBeforeNpc = gameTimer->isPaused() &&
                        engine->_pendingTransition &&
                        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                        uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                        !pActors.empty() && !pOutdoor->mm6DeltaBytes.empty() &&
                        pNPCStats->uNewlNPCBufPos == 0 &&
                        pPartyActionQueue->uNumActions == 0;
                }, mm6::testing::PreEventCheckpointResult::AbortToMainMenu);
            staged = true;
        });
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });

    game.skipLoadingScreen();
    game.tick(2);
    EXPECT_TRUE(staged);
    EXPECT_TRUE(checkpoint);
    EXPECT_TRUE(rebuiltBeforeNpc);
    ASSERT_NE(preEvent, nullptr);
    EXPECT_FALSE(preEvent->reachedMapLoad());
    EXPECT_FALSE(preEvent->reachedInput());
    EXPECT_EQ(pNPCStats->uNewlNPCBufPos, 0);
    EXPECT_FALSE(engine->_pendingTransition);
    EXPECT_EQ(slotBytes(primary), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);
    game.runGameRoutine([&] {
        preEvent.reset();
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(false);
        mm6::testing::setLoadGameOverride({});
    });
}
