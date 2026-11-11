#include <unistd.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numbers>
#include <optional>
#include <sstream>
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
#include "Engine/MM6/MM6SaveContainer.h"
#include "Engine/MM6/MM6SaveMetadata.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/ScopeGuard.h"

static std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path root = std::filesystem::canonical(std::filesystem::path(value));
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 autosave test needs an owner-only system-temp profile");
    return root;
}

static bool enabled(std::string_view value) {
    const char *flag = std::getenv("ENROTH_MM6_AUTOSAVE_ENABLE");
    return flag && std::string_view(flag) == value;
}

static std::vector<std::uint8_t> fileBytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

static std::string fileText(const std::filesystem::path &path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

static std::string slotSummary(const std::filesystem::path &path) {
    // The EC6 version of a slot and the map it was saved on, or empty while there is no slot.
    if (!std::filesystem::exists(path))
        return {};
    const mm6::SaveContainer container = mm6::decodeSaveContainer(fileBytes(path));
    return fmt::format("v{} {}", container.version, mm6::decodeMetadataSections(container).header.mapFileName);
}

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
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
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static void confirmTransition(EngineController &game) {
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void walkOverWestEdge(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-22450, -10000, 100), 180); // Next to the west edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    confirmTransition(game);
}

static void standAtD06Door(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("outd3.odm"), Vec3f(16512, -14550, 100), 90); // South of the d06 door, facing it.
}

static void enterD06(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}

static void leaveD06(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("d06.blv"), pParty->pos, 0); // Turned around to the way out behind the arrival.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}

static void killDummy(EngineController &game, Vec3f monsterPos) {
    // The party hits a dummy in front of it until it's dead.
    const Actor *monster = game.spawnMonster(monsterPos, static_cast<MonsterId>(121), SPAWN_DUMMY);
    game.pointMouseAtActor(monster->id);
    for (int attack = 0; attack < 1000 && monster->aiState != Dead; attack++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick(); // A door opens only for a character who has recovered from the fight.
}

static void loadFromMenu(EngineController &game, int slot) {
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", slot));
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
}

static std::string partySummary() {
    // Where the party stands, its gold and food, and every character's hit points, spell points and bag.
    std::string result = fmt::format("{} {} {} {} gold {} food {}", pParty->pos.x, pParty->pos.y, pParty->pos.z, pParty->_viewYaw,
                                     pParty->GetGold(), pParty->GetFood());
    for (const Character &character : pParty->pCharacters) {
        result += fmt::format(" | hp {} sp {}", character.health, character.mana);
        for (InventoryConstEntry entry : character.inventory.entries())
            result += fmt::format(" {}@{},{}", std::to_underlying(entry->itemId), entry.geometry().x, entry.geometry().y);
    }
    return result;
}

static std::vector<int> deadActors() {
    std::vector<int> result;
    for (const Actor &actor : pActors)
        if (actor.aiState == Dead)
            result.push_back(actor.id);
    return result;
}

GAME_TEST(MM6AutosaveHeadless, DISABLED_EdgeAndDoorWriteTheAutosave) {
    // MM6 never wrote an autosave, the original writes one for a new party and on every trip to another map.
    if (!enabled("strict-autosave-write"))
        GTEST_SKIP() << "Run only as the first autosave child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path autosave = profile / "saves/autosave.ec6";
    ASSERT_FALSE(std::filesystem::exists(autosave));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId d06 = pMapTable->GetMapInfo("d06.blv");
    test.startTaping();
    auto mapTape = tapes.map();
    auto autosaveTape = tapes.custom([&] { return slotSummary(autosave); });
    auto backupTape = tapes.custom([&] { return slotSummary(profile / "saves/autosave.ec6.bak"); });

    walkOverWestEdge(game);
    standAtD06Door(game);
    const std::string party = partySummary();
    enterD06(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3, d06));
    EXPECT_EQ(autosaveTape, tape("v5 oute3.odm", "v7 oute3.odm", "v7 outd3.odm")); // A new party on the first map, then each map the party leaves.
    EXPECT_EQ(backupTape, tape("", "v5 oute3.odm", "v7 oute3.odm"));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    std::ofstream(profile / "expected-autosave.txt") << party << "\n";
}

GAME_TEST(MM6AutosaveHeadless, DISABLED_AutosaveLoadsAndQuickSaveComesBack) {
    // MM6 refused quick save and quick load. A fresh process loads the autosave from the d06 door with the party in
    // front of it, and a quick save there comes back with F9 after the party has moved and killed a monster.
    if (!enabled("strict-autosave-reload"))
        GTEST_SKIP() << "Run only after the autosave write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    std::istringstream expected(fileText(profile / "expected-autosave.txt"));
    std::string savedParty;
    ASSERT_TRUE(std::getline(expected, savedParty));
    const std::filesystem::path quickSave = profile / "saves/quicksave0.ec6";
    ASSERT_FALSE(std::filesystem::exists(quickSave));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    test.startTaping();
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto partyTape = tapes.custom([] { return partySummary(); });
    auto deadTape = tapes.custom([=] { return engine->_currentLoadedMapId == outd3 ? std::optional(deadActors()) : std::nullopt; });
    auto quickSaveTape = tapes.custom([&] { return slotSummary(quickSave); });

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 1);
    EXPECT_EQ(saveLoadMenu()->slots()[0].header.name, localization->str(LSTR_AUTOSAVE));
    EXPECT_EQ(saveLoadMenu()->slots()[0].header.locationName, "outd3.odm");
    loadFromMenu(game, 0);
    ASSERT_EQ(mapTape.back(), outd3);
    EXPECT_EQ(partyTape.back(), savedParty);

    game.pressAndReleaseKey(PlatformKey::KEY_F5);
    game.tick();
    const std::string quickSavedParty = partyTape.back();
    game.teleportTo(outd3, Vec3f(21632, -16880, 0), 0); // By a crate south of the arrival, facing east.
    killDummy(game, Vec3f(21800, -16880, 0));
    game.pressAndReleaseKey(PlatformKey::KEY_F9);
    game.skipLoadingScreen();
    game.tick();

    EXPECT_EQ(mapTape, tape(MAP_INVALID, outd3));
    EXPECT_EQ(quickSaveTape, tape("", "v7 outd3.odm"));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    ASSERT_EQ(deadTape.size(), 4);
    EXPECT_EQ(deadTape[1], std::vector<int>());
    EXPECT_EQ(deadTape[2]->size(), 1); // The dummy fell after the quick save.
    EXPECT_EQ(deadTape[3], std::vector<int>()); // And is gone after the quick load.
    EXPECT_EQ(partyTape.back(), quickSavedParty);
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    std::ofstream(profile / "expected-quicksave.txt") << quickSavedParty << "\n";
}

GAME_TEST(MM6AutosaveHeadless, DISABLED_QuickSaveLoadsInFreshProcess) {
    // A fresh MM6 process lists the quick save next to the autosave and loads it with the party where it quick saved.
    if (!enabled("strict-quicksave-reload"))
        GTEST_SKIP() << "Run only after the quick save child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    std::istringstream expected(fileText(profile / "expected-quicksave.txt"));
    std::string savedParty;
    ASSERT_TRUE(std::getline(expected, savedParty));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    test.startTaping();
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto partyTape = tapes.custom([] { return partySummary(); });

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 2);
    EXPECT_EQ(saveLoadMenu()->slots()[0].header.name, localization->str(LSTR_AUTOSAVE));
    EXPECT_EQ(saveLoadMenu()->slots()[1].header.name, localization->str(LSTR_QUICKSAVE));
    EXPECT_EQ(saveLoadMenu()->slots()[1].fileName, "quicksave0.ec6");
    loadFromMenu(game, 1);

    EXPECT_EQ(mapTape, tape(MAP_INVALID, outd3));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(partyTape.back(), savedParty);
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameLoad, EscapeMenuLoadReplacesTheSession) {
    // MM6 refused Load from the Escape menu. Loading a first-map slot from the map next to a dungeon puts the party back
    // where it saved, and the dungeon it visited after the save is as it was.
    char profileTemplate[] = "mm6-restart-ingame-XXXXXX";
    const std::filesystem::path tempRoot = std::filesystem::canonical(std::filesystem::temp_directory_path());
    std::string created = (tempRoot / profileTemplate).string();
    ASSERT_NE(mkdtemp(created.data()), nullptr);
    const std::filesystem::path profile = std::filesystem::canonical(created);
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId d06 = pMapTable->GetMapInfo("d06.blv");
    test.startTaping();
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto partyTape = tapes.custom([] { return partySummary(); });
    auto d06DeadTape = tapes.custom([=] { return engine->_currentLoadedMapId == d06 ? std::optional(deadActors()) : std::nullopt; });

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
    game.doubleClickGuiButton("SaveMenu_Slot0"); // The new save slot.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_N);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    const std::string savedParty = partyTape.back();

    walkOverWestEdge(game);
    standAtD06Door(game);
    enterD06(game);
    const float arrivalYaw = pParty->_viewYaw * std::numbers::pi / 1024;
    killDummy(game, pParty->pos + Vec3f(std::cos(arrivalYaw), std::sin(arrivalYaw), 0) * 150); // In front of the arrival.
    leaveD06(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots()[1].fileName, "save000.ec6");
    loadFromMenu(game, 1);
    ASSERT_EQ(mapTape.back(), oute3);
    const std::string loadedParty = partyTape.back();
    walkOverWestEdge(game);
    standAtD06Door(game);
    enterD06(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3, d06, outd3, oute3, outd3, d06));
    EXPECT_EQ(loadedParty, savedParty);
    ASSERT_EQ(d06DeadTape.size(), 5);
    EXPECT_EQ(d06DeadTape[1], std::vector<int>());
    EXPECT_EQ(d06DeadTape[2]->size(), 1); // The dummy fell.
    EXPECT_EQ(d06DeadTape[4], std::vector<int>()); // The dungeon is as it was before the save.
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}
