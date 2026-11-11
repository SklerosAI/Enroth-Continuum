#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6SessionSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/Evt/EvtInstruction.h"
#include "Engine/Party.h"
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
        throw std::runtime_error("MM6 save anywhere test needs an owner-only system-temp profile");
    return root;
}

static bool enabled(std::string_view value) {
    const char *flag = std::getenv("ENROTH_MM6_SAVE_ANYWHERE_ENABLE");
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

static void openSaveMenu(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
}

static std::string chestContents(int chestId) {
    // The chest's flags, then every item with its grid cell.
    std::string result = fmt::format("{}", std::to_underlying(vChests[chestId].flags));
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        result += fmt::format(" {}@{},{}", std::to_underlying(entry->itemId), entry.geometry().x, entry.geometry().y);
    return result;
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

static void killDummy(EngineController &game, MapId map, Vec3f partyPos, Vec3f monsterPos) {
    // The party faces east and hits a dummy in front of it until it's dead.
    game.teleportTo(map, partyPos, 0);
    const Actor *monster = game.spawnMonster(monsterPos, static_cast<MonsterId>(121), SPAWN_DUMMY);
    game.pointMouseAtActor(monster->id);
    for (int attack = 0; attack < 1000 && monster->aiState != Dead; attack++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick(); // A chest opens only for a character who has recovered from the fight.
}

static void lootOute3Chest2(EngineController &game) {
    // The party opens an untrapped chest on the first map, takes its first item that isn't gold and puts it into the
    // first character's bag.
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-19696, -9686, 0), 270); // In front of the chest, facing it.
    game.pointMouseAtFace(897); // The chest's near side.
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    for (InventoryEntry entry : vChests[2].inventory.entries()) {
        if (entry->isGold())
            continue;
        game.pressAndReleaseButton(BUTTON_LEFT, Pointi(42, 34) + entry.geometry().topLeft() * 32 + Pointi(16, 16)); // The chest grid of type 0.
        game.tick();
        break;
    }
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.moveMouse(600, 400); // Off the viewport. Hovering an interactive decoration without a variable of its own indexes the decoration variables at -75.
    game.tick();
    game.pressGuiButton("Game_Character1"); // A held item goes into the bag of the clicked portrait.
    game.tick();
}

static void walkOverEdge(EngineController &game, MapId map, Vec3f pos, int yaw) {
    game.teleportTo(map, pos, yaw); // Next to the edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    confirmTransition(game);
}

static void disarmAndOpenOutd3Chest4(EngineController &game) {
    // The active character, good enough at disarming, opens a trapped chest on outd3 and closes it again.
    pParty->activeCharacter().setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(6, MASTERY_NOVICE)); // Beats outd3's lock even on the worst roll.
    game.teleportTo(pMapTable->GetMapInfo("outd3.odm"), Vec3f(21632, -17200, 0), 90); // South of the crate, facing it.
    game.pointMouseAtFace(2308); // The crate's near side.
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.moveMouse(600, 400); // Off the viewport.
    game.tick();
}

static void enterD06(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("outd3.odm"), Vec3f(16512, -14550, 100), 90); // South of the d06 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}

static std::vector<int> deadActors() {
    std::vector<int> result;
    for (const Actor &actor : pActors)
        if (actor.aiState == Dead)
            result.push_back(actor.id);
    return result;
}

GAME_TEST(MM6SaveAnywhereHeadless, DISABLED_DungeonSaveAfterTravel) {
    // MM6 saved only on the first map of a new game, the Save menu refused once the party had left it. A save on the
    // first map, then one in a dungeon over it, keeps the first one as the backup.
    if (!enabled("strict-save-anywhere-write"))
        GTEST_SKIP() << "Run only as the first save-anywhere child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    ASSERT_FALSE(std::filesystem::exists(slot));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId d06 = pMapTable->GetMapInfo("d06.blv");
    test.startTaping();
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    auto oute3DeadTape = tapes.custom([=] { return engine->_currentLoadedMapId == oute3 ? std::optional(deadActors()) : std::nullopt; });
    auto outd3DeadTape = tapes.custom([=] { return engine->_currentLoadedMapId == outd3 ? std::optional(deadActors()) : std::nullopt; });
    auto chestTape = tapes.custom([=] { return engine->_currentLoadedMapId == oute3 ? std::optional(chestContents(2)) : std::nullopt; });
    auto trappedChestTape = tapes.custom([=] { return engine->_currentLoadedMapId == outd3 ? std::optional(chestContents(4)) : std::nullopt; });

    openSaveMenu(game);
    game.doubleClickGuiButton("SaveMenu_Slot0"); // The new save slot.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_N);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_G);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.
    ASSERT_TRUE(std::filesystem::is_regular_file(slot));
    const auto firstMapSave = fileBytes(slot);

    killDummy(game, oute3, Vec3f(-21000, -1500, 160), Vec3f(-20850, -1500, 160)); // By the d02 door, on open ground.
    lootOute3Chest2(game);
    walkOverEdge(game, oute3, Vec3f(-22450, -10000, 100), 180); // The west edge.
    killDummy(game, outd3, Vec3f(21632, -16880, 0), Vec3f(21800, -16880, 0)); // By a crate south of the arrival.
    disarmAndOpenOutd3Chest4(game);
    enterD06(game);
    const std::string party = partySummary();

    openSaveMenu(game);
    ASSERT_EQ(screenTape.back(), SCREEN_SAVEGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 2);
    ASSERT_EQ(saveLoadMenu()->slots()[1].fileName, "save000.ec6");
    game.pressGuiButton("SaveMenu_Slot1");
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.

    EXPECT_EQ(mapTape, tape(oute3, outd3, d06));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_EQ(heldTape.size(), 3); // Empty, the chest item on the cursor, empty again.
    ASSERT_EQ(oute3DeadTape.size(), 3);
    EXPECT_EQ(oute3DeadTape[0], std::vector<int>());
    ASSERT_EQ(oute3DeadTape[1]->size(), 1);
    ASSERT_EQ(outd3DeadTape.size(), 4);
    EXPECT_EQ(outd3DeadTape[1], std::vector<int>());
    ASSERT_EQ(outd3DeadTape[2]->size(), 1);
    ASSERT_EQ(chestTape.size(), 4);
    EXPECT_LT(chestTape[2]->size(), chestTape[1]->size()); // The item left the chest.
    ASSERT_EQ(trappedChestTape.size(), 5); // The chest's random items are made while outd3 loads.
    EXPECT_TRUE(trappedChestTape[2]->starts_with(fmt::format("{} ", std::to_underlying(CHEST_TRAPPED))));
    EXPECT_TRUE(trappedChestTape[3]->starts_with(fmt::format("{} ", std::to_underlying(CHEST_ITEMS_PLACED)))); // Disarmed and opened.
    const auto saved = fileBytes(slot);
    EXPECT_EQ(fileBytes(profile / "saves/save000.ec6.bak"), firstMapSave);
    game.runGameRoutine([&] {
        const mm6::SessionSaveFields fields = mm6::decodeSessionSave(saved, GameTitle::MM6);
        EXPECT_EQ(fields.raw.metadata.header.mapFileName, "d06.blv");
        EXPECT_EQ(fields.raw.metadata.header.displayName, "ng");
        EXPECT_NO_THROW((void) mm6::stageSessionSave(saved, GameTitle::MM6));
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    std::ofstream(profile / "expected.txt") << party << "\n" << oute3DeadTape[1]->front() << "\n" << outd3DeadTape[2]->front() << "\n"
                                            << *chestTape[2] << "\n" << *trappedChestTape[3] << "\n";
}

GAME_TEST(MM6SaveAnywhereHeadless, DISABLED_DungeonSlotLoadsWithItsOutdoorMaps) {
    // A fresh MM6 process loads the dungeon slot with the party where it saved, and the outdoor maps the party came
    // through still have their dead monsters, the looted chest and the disarmed one when it walks back. Saving again
    // keeps the dungeon save as the backup.
    if (!enabled("strict-save-anywhere-reload"))
        GTEST_SKIP() << "Run only after the save-anywhere write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    const std::filesystem::path backup = profile / "saves/save000.ec6.bak";
    const auto saved = fileBytes(slot);
    ASSERT_FALSE(saved.empty());
    ASSERT_TRUE(std::filesystem::exists(backup));
    std::istringstream expected(fileText(profile / "expected.txt"));
    std::string savedParty, oute3Line, outd3Line, lootedChest, disarmedChest;
    ASSERT_TRUE(std::getline(expected, savedParty) && std::getline(expected, oute3Line) && std::getline(expected, outd3Line) &&
                std::getline(expected, lootedChest) && std::getline(expected, disarmedChest));
    const int oute3Killed = std::stoi(oute3Line);
    const int outd3Killed = std::stoi(outd3Line);
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId d06 = pMapTable->GetMapInfo("d06.blv");
    test.startTaping();
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto partyTape = tapes.custom([] { return partySummary(); });
    auto oute3KilledTape = tapes.custom([=] {
        return engine->_currentLoadedMapId == oute3 && oute3Killed < std::ssize(pActors) ? std::optional(pActors[oute3Killed].aiState) : std::nullopt;
    });
    auto outd3KilledTape = tapes.custom([=] {
        return engine->_currentLoadedMapId == outd3 && outd3Killed < std::ssize(pActors) ? std::optional(pActors[outd3Killed].aiState) : std::nullopt;
    });
    auto chestTape = tapes.custom([=] { return engine->_currentLoadedMapId == oute3 ? std::optional(chestContents(2)) : std::nullopt; });
    auto trappedChestTape = tapes.custom([=] { return engine->_currentLoadedMapId == outd3 ? std::optional(chestContents(4)) : std::nullopt; });

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 2); // The autosave from the d06 door, then the dungeon slot.
    EXPECT_EQ(saveLoadMenu()->slots()[0].fileName, "autosave.ec6");
    EXPECT_EQ(saveLoadMenu()->slots()[1].header.name, "ng");
    EXPECT_EQ(saveLoadMenu()->slots()[1].header.locationName, "d06.blv");
    game.pressGuiButton("LoadMenu_Slot1");
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    ASSERT_EQ(mapTape.back(), d06);
    EXPECT_EQ(partyTape.back(), savedParty);

    game.teleportTo(d06, pParty->pos, 0); // Turned around to the way out behind the arrival.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
    walkOverEdge(game, outd3, Vec3f(22450, -10000, 164), 0); // The east edge.

    EXPECT_EQ(mapTape, tape(MAP_INVALID, d06, outd3, oute3));
    EXPECT_EQ(outd3KilledTape, tape(std::optional<AIState>(), std::optional(Dead), std::optional<AIState>()));
    EXPECT_EQ(oute3KilledTape.back(), Dead);
    EXPECT_EQ(chestTape.back(), lootedChest);
    EXPECT_EQ(trappedChestTape, tape(std::optional<std::string>(), std::optional(disarmedChest), std::optional<std::string>()));

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
        EXPECT_EQ(mm6::decodeSessionSave(resaved, GameTitle::MM6).raw.metadata.header.mapFileName, "oute3.odm");
        EXPECT_NO_THROW((void) mm6::stageSessionSave(resaved, GameTitle::MM6));
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
