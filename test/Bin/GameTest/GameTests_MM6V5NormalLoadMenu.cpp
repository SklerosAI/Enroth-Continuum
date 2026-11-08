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
#include "Engine/MM6/MM6CoreSaveFields.h"
#include "Engine/MM6/MM6V5FirstMapLoadAttempt.h"
#include "Engine/MM6/MM6V5FirstMapLoadPreflight.h"
#include "Engine/MM6/MM6V5FullSessionPreflight.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
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

static mm6::CorePartyFields slotParty(const std::vector<std::uint8_t> &bytes) {
    const auto fields = std::unique_ptr<mm6::V5SaveFields>(new mm6::V5SaveFields(mm6::decodeV5Save(bytes, GameTitle::MM6)));
    return fields->v4.v3.raw.core.party;
}

static void openMainMenuLoad(EngineController &game) {
    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
}

static void loadSelectedSlot(EngineController &game, int row) {
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", row));
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
}

GAME_TEST(MM6V5NormalLoadMenuHeadless, DISABLED_LoadMenuListsAndLoadsNumberedFirstMapSlot) {
    // The MM6 Load menu lists only loadable numbered slots, and the slot saved from the Save menu loads in a fresh
    // child through the production load, with its party and map intact before the first input and saving armed.
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
        savedParty = slotParty(saved);
        mm6::testing::setNativeProfileRootForTest(profile);
    });

    test.startTaping();
    auto screenTape = tapes.screen();
    auto mapTape = tapes.map();
    auto attemptTape = tapes.custom([] { return engine->_mm6FirstMapLoadAttempt != nullptr; });
    auto armedTape = tapes.custom([] { return mm6::isFirstMapManualSaveArmed(); });
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto yawTape = tapes.custom([] { return pParty->_viewYaw; });
    openMainMenuLoad(game);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 1);
    EXPECT_EQ(saveLoadMenu()->slots()[0].fileName, "save000.ec6");
    EXPECT_EQ(saveLoadMenu()->slots()[0].header.name, "ab");
    EXPECT_EQ(saveLoadMenu()->slots()[0].header.locationName, "oute3.odm");
    loadSelectedSlot(game, 0);
    const Vec3f savedPosition(savedParty.position[0], savedParty.position[1], savedParty.position[2]);
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(mapTape.back(), pMapTable->GetMapInfo("oute3.odm"));
    EXPECT_EQ(attemptTape, tape(false, true, false));
    EXPECT_EQ(armedTape, tape(false, true));
    EXPECT_EQ(positionTape.back(), savedPosition);
    EXPECT_EQ(yawTape.back(), savedParty.viewYaw);

    game.pressKey(PlatformKey::KEY_UP);
    game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
    EXPECT_NE(positionTape.back(), savedPosition); // The party walked forward.
    EXPECT_EQ(mapTape.back(), pMapTable->GetMapInfo("oute3.odm"));
    EXPECT_EQ(fileBytes(slot), saved);
    EXPECT_EQ(fileBytes(profile / "saves/save000.ec6.bak"), savedBackup);
    EXPECT_EQ(fileBytes(profile / "saves/save002.ec6"), malformed);
    EXPECT_EQ(window->nativeHandle(), nullptr);
    game.runGameRoutine([] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6V5NormalLoadMenuHeadless, DISABLED_LoadedSlotSavesAgainThroughSaveMenu) {
    // A numbered slot loaded through the MM6 Load menu in a fresh child saves again through the Save menu after one
    // input, over the same slot and with the party where the input left it.
    const char *enable = std::getenv("ENROTH_MM6_V5_NORMAL_RESAVE_ENABLE");
    if (!enable || std::string_view(enable) != "strict-v5-normal-resave")
        GTEST_SKIP() << "Run only after the normal Load menu child";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    const std::filesystem::path backup = profile / "saves/save000.ec6.bak";
    const auto saved = fileBytes(slot);
    ASSERT_FALSE(saved.empty());
    mm6::CorePartyFields savedParty;
    game.runGameRoutine([&] {
        savedParty = slotParty(saved);
        mm6::testing::setNativeProfileRootForTest(profile);
    });
    const Vec3f savedPosition(savedParty.position[0], savedParty.position[1], savedParty.position[2]);

    test.startTaping();
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto positionTape = tapes.custom([] { return pParty->pos; });
    openMainMenuLoad(game);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 1);
    ASSERT_EQ(saveLoadMenu()->slots()[0].fileName, "save000.ec6");
    loadSelectedSlot(game, 0);
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    ASSERT_EQ(positionTape.back(), savedPosition);

    game.pressKey(PlatformKey::KEY_UP);
    game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_NE(positionTape.back(), savedPosition); // The party walked forward.

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
    ASSERT_EQ(current_screen_type, SCREEN_SAVEGAME);
    EXPECT_EQ(saveLoadMenu()->selectedSlot().fileName, "save000.ec6"); // The loaded slot is preselected.
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    const auto resaved = fileBytes(slot);
    EXPECT_NE(resaved, saved);
    EXPECT_EQ(fileBytes(backup), saved);
    game.runGameRoutine([&] {
        const mm6::CorePartyFields resavedParty = slotParty(resaved);
        const Vec3f resavedPosition(resavedParty.position[0], resavedParty.position[1], resavedParty.position[2]);
        EXPECT_NE(resavedPosition, savedPosition);
        EXPECT_CONTAINS(positionTape, resavedPosition);
        EXPECT_NO_THROW((void) mm6::stageV5FirstMapLoadPreflight(resaved, GameTitle::MM6));
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
