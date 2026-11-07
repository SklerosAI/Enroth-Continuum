#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6PostWorldAdmissionForTest.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/MM6/MM6WorldSaveRuntime.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/GUIWindow.h"

namespace {

std::filesystem::path disposableProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 abort probe requires an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 abort probe requires an owner-only system-temp profile");
    return root;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

// Run alone in the fresh disposable child after the v1 first-map writer.
// This is a control test for the guarded post-world abort edge, not a v2
// publication test. The child must end after the assertion.
GAME_TEST(MM6PostWorldAbortHeadless, DISABLED_V1ControlQuickLoadReturnsBeforeInput) {
    const std::filesystem::path profile = disposableProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to a disposable v1 profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path primary = profile / "saves" / "autosave.ec6";
    ASSERT_TRUE(std::filesystem::is_regular_file(primary));
    const auto before = slotBytes(primary);
    ASSERT_FALSE(before.empty());

    bool loaded = false;
    bool postWorld = false;
    bool beforeInput = false;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> admission;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view fileName) {
            if (fileName != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 abort probe selected another slot");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            Party freshParty;
            NPCStats freshNpcs = *pNPCStats;
            Timer freshTimer;
            ActiveOverlayList freshOverlays;
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto staged = mm6::stageOfflineSaveSlotForTest(
                profile, fileName, freshParty, freshNpcs, freshTimer, freshOverlays,
                maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, pOverlayTable->overlays.size(), GameTitle::MM6);
            if (!staged || staged->needsRecovery || staged->bytes != before)
                throw std::runtime_error("MM6 abort probe did not stage its v1 slot");
            *pParty = std::move(staged->staged.party);
            *pNPCStats = std::move(staged->staged.worldAndVisuals.world.npcs);
            *gameTimer = std::move(staged->staged.worldAndVisuals.visuals.timer);
            *pActiveOverlayList = std::move(staged->staged.worldAndVisuals.visuals.overlays);
            pMapDeltas = std::move(staged->staged.worldAndVisuals.world.mapDeltas);
            mm6::chestTitleState() = std::move(staged->staged.worldAndVisuals.chestTitle);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>();
            admission = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                GameTitle::MM6, [&] {
                    postWorld = true;
                    beforeInput = gameTimer->isPaused() &&
                        !engine->_pendingTransition &&
                        engine->_currentLoadedMapId == pMapTable->GetMapInfo("oute3.odm") &&
                        uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
                        !pActors.empty() && vChests.size() == 20 &&
                        pPartyActionQueue->uNumActions == 0;
                }, mm6::testing::PostWorldAdmissionResult::AbortToMainMenu);
            loaded = true;
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
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(postWorld);
    EXPECT_TRUE(beforeInput);
    EXPECT_EQ(slotBytes(primary), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
