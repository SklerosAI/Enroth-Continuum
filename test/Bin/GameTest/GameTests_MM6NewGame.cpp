#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6V5FirstMapLoadPreflight.h"
#include "Engine/MM6/MM6V5FullSessionPreflight.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/MapTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"

static std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path root = std::filesystem::canonical(std::filesystem::path(value));
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 new game test needs an owner-only system-temp profile");
    return root;
}

static bool enabled(std::string_view value) {
    const char *flag = std::getenv("ENROTH_MM6_NEW_GAME_ENABLE");
    return flag && std::string_view(flag) == value;
}

static std::vector<std::uint8_t> fileBytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

static Vec3f slotPosition(const std::vector<std::uint8_t> &bytes) {
    const auto fields = std::unique_ptr<mm6::V5SaveFields>(new mm6::V5SaveFields(mm6::decodeV5Save(bytes, GameTitle::MM6)));
    const auto &position = fields->v4.v3.raw.core.party.position;
    return Vec3f(position[0], position[1], position[2]);
}

static void createPartyThroughInput(EngineController &game) {
    // Each character takes its first three optional skills, two of which fill the free slots, and up to three points
    // in every attribute, until the party has no points left.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
}

static void openSaveMenu(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
}

GAME_TEST(MM6NewGameHeadless, NewPartyEntersFirstMap) {
    // MM6 New Game stopped at an uncaught throw from the new-game save right after the creation screen.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    test.startTaping();
    auto screenTape = tapes.screen();
    auto mapTape = tapes.map();
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_EQ(screenTape.back(), SCREEN_PARTY_CREATION);
    createPartyThroughInput(game);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(mapTape.back(), pMapTable->GetMapInfo("oute3.odm"));
    EXPECT_FALSE(ufs->exists("saves"));
}

GAME_TEST(MM6NewGameHeadless, DISABLED_NewPartySavesFromSaveMenu) {
    // A party made on the MM6 creation screen enters the first map and saves from the Save menu into a numbered slot.
    if (!enabled("strict-new-game-save"))
        GTEST_SKIP() << "Run only as the first new-game child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    ASSERT_FALSE(std::filesystem::exists(slot));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    test.startTaping();
    auto screenTape = tapes.screen();
    auto mapTape = tapes.map();
    auto statusTape = tapes.statusBar();
    auto armedTape = tapes.custom([] { return mm6::isFirstMapManualSaveArmed(); });
    auto positionTape = tapes.custom([] { return pParty->pos; });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_EQ(screenTape.back(), SCREEN_PARTY_CREATION);
    createPartyThroughInput(game);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(mapTape.back(), pMapTable->GetMapInfo("oute3.odm"));

    openSaveMenu(game);
    ASSERT_EQ(screenTape.back(), SCREEN_SAVEGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 1); // Only the new save slot.
    game.doubleClickGuiButton("SaveMenu_Slot0");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_N);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_G);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_EQ(armedTape, tape(false, true));
    ASSERT_TRUE(std::filesystem::is_regular_file(slot));
    EXPECT_FALSE(std::filesystem::exists(profile / "saves/save000.ec6.bak"));
    const auto saved = fileBytes(slot);
    game.runGameRoutine([&] {
        EXPECT_NO_THROW((void) mm6::stageV5FirstMapLoadPreflight(saved, GameTitle::MM6));
        EXPECT_CONTAINS(positionTape, slotPosition(saved));
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

GAME_TEST(MM6NewGameHeadless, DISABLED_NewPartySlotLoadsAndSavesAgain) {
    // The slot a new MM6 party saved loads from the Load menu in a fresh process, and saving again after a step keeps
    // the first save as the backup.
    if (!enabled("strict-new-game-reload"))
        GTEST_SKIP() << "Run only after the new-game save child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    const std::filesystem::path backup = profile / "saves/save000.ec6.bak";
    const auto saved = fileBytes(slot);
    ASSERT_FALSE(saved.empty());
    ASSERT_FALSE(std::filesystem::exists(backup));
    Vec3f savedPosition;
    game.runGameRoutine([&] {
        savedPosition = slotPosition(saved);
        mm6::testing::setNativeProfileRootForTest(profile);
    });

    test.startTaping();
    auto screenTape = tapes.screen();
    auto mapTape = tapes.map();
    auto statusTape = tapes.statusBar();
    auto positionTape = tapes.custom([] { return pParty->pos; });
    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 2); // The autosave of the new party, then its slot.
    EXPECT_EQ(saveLoadMenu()->slots()[0].fileName, "autosave.ec6");
    EXPECT_EQ(saveLoadMenu()->slots()[1].fileName, "save000.ec6");
    EXPECT_EQ(saveLoadMenu()->slots()[1].header.name, "ng");
    game.pressGuiButton("LoadMenu_Slot1");
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(mapTape.back(), pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(positionTape.back(), savedPosition);

    game.pressKey(PlatformKey::KEY_UP);
    game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_NE(positionTape.back(), savedPosition); // The party walked forward.

    openSaveMenu(game);
    ASSERT_EQ(screenTape.back(), SCREEN_SAVEGAME);
    EXPECT_EQ(saveLoadMenu()->selectedSlot().fileName, "save000.ec6"); // The loaded slot is preselected.
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    const auto resaved = fileBytes(slot);
    EXPECT_NE(resaved, saved);
    EXPECT_EQ(fileBytes(backup), saved);
    game.runGameRoutine([&] {
        EXPECT_NO_THROW((void) mm6::stageV5FirstMapLoadPreflight(resaved, GameTitle::MM6));
        EXPECT_CONTAINS(positionTape, slotPosition(resaved));
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
