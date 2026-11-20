#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numbers>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineCallObserver.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6SaveContainer.h"
#include "Engine/MM6/MM6SaveMetadata.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/MM6HouseServiceData.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/ScopeGuard.h"

#include "MM6CampaignChain.h"

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

static std::filesystem::path createTempProfile() {
    // An owner-only mm6-restart- folder in the system temp folder, the only kind of profile a test saves into.
    const std::filesystem::path tempRoot = std::filesystem::canonical(std::filesystem::temp_directory_path());
    std::string created = (tempRoot / "mm6-restart-save-XXXXXX").string();
    if (!mkdtemp(created.data()))
        throw std::runtime_error("Can't create a temporary MM6 profile");
    return std::filesystem::canonical(created);
}

static void saveFromMenu(EngineController &game) {
    // Saves into the new slot from the Escape menu.
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
    game.doubleClickGuiButton("SaveMenu_Slot0"); // The new save slot.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.
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

static const Vec3f kTavernDoor(-8784, -8690, 300); // A Lonely Knight in New Sorpigal, facing east.
static const Vec3f kJanineSpot(-7896, -5464, 300); // Janine, who wants a bribe from this party, stands to the east.

static void clickOption(EngineController &game, GUIWindow *window, std::string_view label) {
    // Left click in the middle of the shown house or conversation option with this line.
    ASSERT_NE(window, nullptr);
    std::optional<Pointi> center;
    for (GUIButton *button : window->vButtons)
        if (button->id.contains("Dialogue_Option") && button->rect.h > 0 && button->label == label)
            center = button->rect.center();
    ASSERT_TRUE(center) << "No option " << label;
    game.pressAndReleaseButton(BUTTON_LEFT, *center);
    game.tick();
}

static std::vector<std::string> conversationOptions() {
    // Topics of the street NPC conversation, empty while none is open.
    std::vector<std::string> result;
    if (pDialogueWindow)
        for (GUIButton *button : pDialogueWindow->vButtons)
            if (button->id.starts_with("NpcDialogue_Option"))
                result.push_back(button->label);
    return result;
}

static void pressSpaceAt(EngineController &game, Vec3f position, int viewYaw) {
    // Same map teleport, then Space at the door or the villager in front of the party.
    game.teleportTo(engine->_currentLoadedMapId, position, viewYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static const Vec3f kTownHallDoor(-11090, -6536, 300); // New Sorpigal Town Hall, facing west.

static void waitUntilTownHallOpens(EngineController &game) {
    // Waits in the rest screen an hour at a time until 10 am, when the Town Hall opens.
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    while (pParty->GetPlayingTime().toCivilTime().hour < 10) {
        game.pressGuiButton("Rest_Wait1Hour");
        game.tick();
        while (currentRestType != REST_NONE)
            game.tick(); // The hour passes.
    }
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

static void askJaniceForBounty(EngineController &game) {
    // Janice in the Town Hall answers the Bounty Hunt topic, then the party steps back out, facing east.
    pressSpaceAt(game, kTownHallDoor, 180);
    ASSERT_NE(window_SpeakInHouse, nullptr);
    std::optional<Pointi> portrait;
    for (GUIButton *button : window_SpeakInHouse->vButtons)
        if (button->id.starts_with("House_Npc") && button->label.contains("Janice"))
            portrait = button->rect.center();
    ASSERT_TRUE(portrait);
    game.pressAndReleaseButton(BUTTON_LEFT, *portrait);
    game.tick();
    clickOption(game, window_SpeakInHouse.get(), "Bounty Hunt");
    for (int i = 0; i < 3 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    game.teleportTo(engine->_currentLoadedMapId, kTownHallDoor, 0);
}

static void killBountyMonster(EngineController &game) {
    // The party hits a dummy of this month's New Sorpigal bounty monster in front of it until it's dead.
    Actor *monster = game.spawnMonster(kTownHallDoor + Vec3f(200, 0, 0), pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL], SPAWN_DUMMY);
    monster->hp = 1; // One hit kills it.
    game.pointMouseAtActor(monster->id);
    for (int attack = 0; attack < 1000 && monster->aiState != Dead; attack++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick(); // A door opens only for a character who has recovered from the fight.
}

static void loadFromEscapeMenu(EngineController &game, int slot) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_LoadGame");
    game.tick(3);
    loadFromMenu(game, slot);
}

static constexpr std::uint32_t kHiveTreasureLuck = 84; // Chest luck that rolls artifact 414, relic 423 and three items with special bonuses into the hive's chest 4.
static constexpr ItemId kMorgan = static_cast<ItemId>(414); // An artifact amulet.
static constexpr ItemId kAegis = static_cast<ItemId>(423); // A relic shield.
static constexpr ItemId kAcidSpear = static_cast<ItemId>(35); // Sacred Spear of Acid, special bonus 15.

static void takeFromHiveChest4(EngineController &game, ItemId item, int characterIndex = 0) {
    // The party opens the hive's chest 4 and puts the item into a character's backpack.
    game.teleportTo(pMapTable->GetMapInfo("hive.blv"), Vec3f(-3000, 4182, -1777), 0, -20); // West of chest 4, looking down at it.
    game.pointMouseAtFace(2960);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    InventoryEntry entry = vChests[4].inventory.find(item);
    ASSERT_TRUE(entry);
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[4].chestTypeId].inventoryOffset + entry.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton(fmt::format("Game_Character{}", characterIndex + 1));
    game.tick();
}

static void wearFromBackpack(EngineController &game, int characterIndex, ItemId item) {
    // The character picks the item up in the backpack and drops it on the paperdoll.
    game.goToInventory(characterIndex);
    InventoryEntry entry = pParty->pCharacters[characterIndex].inventory.find(item);
    ASSERT_TRUE(entry);
    game.pressAndReleaseButton(BUTTON_LEFT, Pointi(14, 17) + entry.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {520, 150}); // The paperdoll.
    game.tick();
}

static std::string carriedRecords() {
    // Every item the party carries, where it sits and its native record.
    std::string result;
    for (const Character &character : pParty->pCharacters) {
        result += " |";
        for (InventoryConstEntry entry : character.inventory.entries()) {
            const std::string place = entry.zone() == INVENTORY_ZONE_EQUIPMENT ? fmt::format("slot{}", std::to_underlying(entry.slot())) :
                                      fmt::format("{},{}", entry.geometry().x, entry.geometry().y);
            result += fmt::format(" {}@{}:", std::to_underlying(entry->itemId), place);
            if (entry->mm6RawRecord)
                result += fmt::format("{:02x}", fmt::join(*entry->mm6RawRecord, ""));
        }
    }
    return result;
}

static std::string drawnItems(const AccessibleVector<DrawnQuad> &quads, int characterIndex) {
    // Where the character screen drew the icons of the character's items, in the backpack and on the paperdoll.
    std::set<std::string> icons;
    for (InventoryConstEntry entry : pParty->pCharacters[characterIndex].inventory.entries())
        icons.insert(entry->GetIconName());
    std::string result;
    for (const DrawnQuad &quad : quads)
        if (icons.contains(quad.texture))
            result += fmt::format(" {}@{},{}", quad.texture, quad.position.x, quad.position.y);
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

GAME_TEST(MM6ItemSaveHeadless, DISABLED_ArtifactsSaveWithTheParty) {
    // A party carrying an artifact, a relic or an item with a special bonus couldn't save. A new party takes artifact
    // 414, relic 423 and a spear with a special bonus from the hive, the second character wears the relic, and the party
    // saves, the first half of the round trip.
    if (!enabled("strict-item-save-write"))
        GTEST_SKIP() << "Run only as the item save write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    ASSERT_FALSE(std::filesystem::exists(slot));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    auto statusTape = tapes.statusBar();
    auto quadsTape = tapes.hudQuads();
    auto textTape = tapes.allGUIWindowsText();
    auto shieldTape = charTapes.custom(1, [](const Character &character) {
        InventoryConstEntry entry = character.inventory.entry(ITEM_SLOT_OFF_HAND);
        return entry ? entry->itemId : ITEM_NULL;
    });
    test.startTaping();

    mm6::chestTitleState().lcgState = kHiveTreasureLuck;
    takeFromHiveChest4(game, kMorgan);
    takeFromHiveChest4(game, kAegis, 1); // The second character has the Shield skill.
    takeFromHiveChest4(game, kAcidSpear, 1);
    wearFromBackpack(game, 1, kAegis);
    const std::string items = drawnItems(quadsTape.back(), 1);
    game.pressGuiButton("CharacterWindow_Stats");
    game.tick();
    const std::string stats = fmt::format("{}", fmt::join(textTape.back(), "|"));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    const std::string records = carriedRecords();
    saveFromMenu(game);

    EXPECT_EQ(shieldTape, tape(ITEM_NULL, kAegis));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_TRUE(std::filesystem::is_regular_file(slot));
    EXPECT_NE(items.find("lshield5"), std::string::npos);
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    std::ofstream(profile / "expected-items.txt") << records << "\n" << items << "\n";
    std::ofstream(profile / "expected-stats.txt") << stats; // The character screen's text has line breaks of its own.
}

GAME_TEST(MM6ItemSaveHeadless, DISABLED_ArtifactsComeBackInFreshProcess) {
    // A fresh MM6 process loads the slot, and the artifact, the relic on the paperdoll and the spear come back with
    // their native records, drawn where they were and with the same stats on the character screen.
    if (!enabled("strict-item-save-reload"))
        GTEST_SKIP() << "Run only after the item save write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    std::istringstream expected(fileText(profile / "expected-items.txt"));
    std::string savedRecords, savedItems;
    ASSERT_TRUE(std::getline(expected, savedRecords));
    ASSERT_TRUE(std::getline(expected, savedItems));
    const std::string savedStats = fileText(profile / "expected-stats.txt");
    ASSERT_FALSE(savedStats.empty());
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    auto screenTape = tapes.screen();
    auto quadsTape = tapes.hudQuads();
    auto textTape = tapes.allGUIWindowsText();
    auto recordsTape = tapes.custom([] { return carriedRecords(); });
    test.startTaping();

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto saved = std::ranges::find(slots, "save000.ec6", [](const auto &entry) { return entry.fileName; });
    ASSERT_NE(saved, slots.end());
    loadFromMenu(game, saved - slots.begin());
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    game.goToInventory(1);
    const std::string items = drawnItems(quadsTape.back(), 1);
    game.pressGuiButton("CharacterWindow_Stats");
    game.tick();

    EXPECT_EQ(recordsTape.back(), savedRecords);
    EXPECT_EQ(items, savedItems);
    EXPECT_EQ(fmt::format("{}", fmt::join(textTape.back(), "|")), savedStats);
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

GAME_TEST(MM6InGameSave, SavesWithTheCloakOfBaa) {
    // The game couldn't be saved once Wilbur Humphrey had given the Cloak of Baa, an item an MM6 event gives had no
    // record to save.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    pNPCStats->pNPCData[4].dialogue_1_evt_id = 101; // Slicker Silvertongue's refusal gives Wilbur Humphrey this topic.
    auto cloakTape = tapes.hasItem(static_cast<ItemId>(485));
    auto slotTape = tapes.custom([&] { return std::filesystem::exists(profile / "saves/save000.ec6"); });
    test.startTaping();

    enterMM6CampaignChainCastle(game, pMapTable->GetMapInfo("outd3.odm"), 43); // Castle Ironfist.
    talkToMM6CampaignChainNpc(game, 4);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 101));
    leaveMM6CampaignChainHouse(game);
    game.pressGuiButton("Game_Character1"); // The cloak comes in hand and goes into the first character's backpack.
    game.tick();
    saveFromMenu(game);

    EXPECT_EQ(cloakTape, tape(false, true));
    EXPECT_EQ(slotTape, tape(false, true));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameSave, SavesWithDevilPlans) {
    // The game couldn't be saved while the party carried the Devil Plans, the item a monster drops had no record to
    // save.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    const ItemId plans = static_cast<ItemId>(506);
    game.teleportTo(pMapTable->GetMapInfo("zdwj02.blv"), Vec3f(-128, -300, 1), 90);
    pActors[0].hp = 1; // The Devil Warrior, who carries the plans.
    engine->config->debug.NoDamage.setValue(true); // The other devils of the outpost attack the party.
    auto plansTape = tapes.hasItem(plans);
    auto slotTape = tapes.custom([&] { return std::filesystem::exists(profile / "saves/save000.ec6"); });
    test.startTaping();

    killMM6CampaignChainActor(game, 0);
    for (int frame = 0; frame < 100 && pActors[0].aiState != Removed; frame++)
        game.tick(); // The warrior falls and vanishes, and the plans drop to the floor.
    pickUpMM6CampaignChainFloorItem(game, plans);
    saveFromMenu(game);

    EXPECT_EQ(plansTape, tape(false, true));
    EXPECT_EQ(slotTape, tape(false, true));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameSave, SavesWithHighClericsKey) {
    // The game couldn't be saved while the party carried the High Cleric's key, the item looted from a monster had no
    // record to save.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    const ItemId key = static_cast<ItemId>(567);
    game.teleportTo(pMapTable->GetMapInfo("t7.blv"), Vec3f(2094, -19, 177), 337); // The Temple of Baa's entrance.
    pActors[0].hp = 1; // The priest of Baa who carries the key.
    engine->config->debug.NoDamage.setValue(true); // The temple's priests and acolytes attack the party.
    auto keyTape = tapes.hasItem(key);
    auto slotTape = tapes.custom([&] { return std::filesystem::exists(profile / "saves/save000.ec6"); });
    test.startTaping();

    killMM6CampaignChainActor(game, 0);
    lootMM6CampaignChainCorpse(game, 0);
    saveFromMenu(game);

    EXPECT_EQ(keyTape, tape(false, true));
    EXPECT_EQ(slotTape, tape(false, true));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameSave, StreetNpcBribesAndTavernTipsSurviveLoad) {
    // A load forgot the party's bribes, so the next bribe cost as much as the first, and its tavern drinks and tips, so
    // the barkeep wanted another drink before telling his news again.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    auto tipTape = tapes.custom([] { return current_npc_text; });
    auto optionsTape = tapes.custom([] { return conversationOptions(); });
    const std::string tip = localization->str(mm6::houseServiceOptionLabel(mm6::HouseServiceOption::TipBarkeep));
    test.startTaping();

    pressSpaceAt(game, kTavernDoor, 0);
    game.pressGuiButton("House_Npc0"); // The proprietor.
    game.tick();
    clickOption(game, window_SpeakInHouse.get(), localization->str(LSTR_HAVE_A_DRINK));
    clickOption(game, window_SpeakInHouse.get(), tip);
    game.pressGuiButton("House_Exit"); // Back to the tavern's residents.
    game.tick();
    game.pressGuiButton("House_Exit");
    game.tick();
    pressSpaceAt(game, kJanineSpot, 0);
    clickOption(game, pDialogueWindow.get(), "Bribe 50 Gold");
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    saveFromMenu(game);
    game.goToMainMenu();
    startNewGameThroughInput(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_LoadGame");
    game.tick(3);
    loadFromMenu(game, 1);
    pressSpaceAt(game, kTavernDoor, 0);
    game.pressGuiButton("House_Npc0");
    game.tick();
    clickOption(game, window_SpeakInHouse.get(), tip);
    game.pressGuiButton("House_Exit");
    game.tick();
    game.pressGuiButton("House_Exit");
    game.tick();
    pressSpaceAt(game, kJanineSpot, 0);

    EXPECT_EQ(goldTape, tape(200, 199, 198, 148, 200, 148, 147)); // A drink, a tip, the bribe, the new game, the load and the second tip.
    EXPECT_MISSES(statusTape, localization->str(LSTR_HAVE_A_DRINK_FIRST));
    ASSERT_EQ(tipTape.size(), 5);
    EXPECT_TRUE(tipTape[1].starts_with("An evil guild of mages has been searching the southeast end of our peninsula"));
    EXPECT_EQ(tipTape, tape(std::string(), tipTape[1], std::string(), tipTape[1], std::string()));
    EXPECT_EQ(optionsTape.back(), (std::vector<std::string>{"Beg", "Threat", "Bribe 100 Gold"}));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameSave, BountySurvivesLoad) {
    // The game couldn't be saved once Janice in the New Sorpigal Town Hall had named this month's bounty. Loaded after a
    // new game, she names the same monster, and killing it pays.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    waitUntilTownHallOpens(game);
    auto goldTape = tapes.gold();
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto bountyTape = tapes.custom([] { return std::pair(pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL], pParty->monster_for_hunting_killed[HOUSE_FIRST_TOWN_HALL]); });
    auto countTape = tapes.custom([] { return pParty->uNumBountiesCollected; });
    auto slotTape = tapes.custom([&] { return std::filesystem::exists(profile / "saves/save000.ec6"); });
    test.startTaping();

    askJaniceForBounty(game);
    saveFromMenu(game);
    ASSERT_TRUE(slotTape.back());
    game.goToMainMenu();
    startNewGameThroughInput(game);
    loadFromEscapeMenu(game, 1);
    askJaniceForBounty(game);
    killBountyMonster(game);
    askJaniceForBounty(game);

    const MonsterId druid = static_cast<MonsterId>(45);
    const std::string name = fmt::format("{::}Grand Druid{::}", colorTable.PaleCanary.tag(), colorTable.White.tag());
    const std::string offer = "This month's bounty is on a " + name + ".  Kill it and return before the end of the month to collect the 2800 gold reward.";
    EXPECT_EQ(slotTape, tape(false, true));
    EXPECT_EQ(bountyTape, tape(std::pair(MONSTER_INVALID, false), std::pair(druid, false), std::pair(MONSTER_INVALID, false), std::pair(druid, false),
                               std::pair(druid, true), std::pair(MONSTER_INVALID, false))); // Named, a new game, the load, the kill and the reward.
    EXPECT_EQ(textTape, tape(std::string(), offer, std::string(), offer, std::string(),
                             "Congratulations on defeating the " + name + "!  Here is the 2800 gold reward.  Come back next month for a new bounty.",
                             std::string()));
    EXPECT_EQ(goldTape, tape(200, 3000)); // A level 28 monster.
    EXPECT_EQ(countTape, tape(0, 1));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameSave, BountyKillSurvivesAutosave) {
    // A load forgot the town hall bounties. The autosave written as the party left New Sorpigal with this month's
    // bounty monster dead came back with no bounty, and Janice named a new monster instead of paying. Once she has
    // paid, the game saves.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    waitUntilTownHallOpens(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto mapTape = tapes.map();
    auto goldTape = tapes.gold();
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto bountyTape = tapes.custom([] { return std::pair(pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL], pParty->monster_for_hunting_killed[HOUSE_FIRST_TOWN_HALL]); });
    auto autosaveTape = tapes.custom([&] { return slotSummary(profile / "saves/autosave.ec6"); });
    auto slotTape = tapes.custom([&] { return slotSummary(profile / "saves/save000.ec6"); });
    test.startTaping();

    askJaniceForBounty(game);
    killBountyMonster(game);
    walkOverWestEdge(game);
    loadFromEscapeMenu(game, 0);
    game.pressGuiButton("Transition_No"); // The autosave has the party at the edge, so the travel dialogue is up again.
    game.tick();
    askJaniceForBounty(game);
    saveFromMenu(game);

    const MonsterId druid = static_cast<MonsterId>(45);
    const std::string name = fmt::format("{::}Grand Druid{::}", colorTable.PaleCanary.tag(), colorTable.White.tag());
    EXPECT_EQ(mapTape, tape(oute3, outd3, oute3));
    EXPECT_EQ(autosaveTape, tape("v5 oute3.odm", "v7 oute3.odm"));
    EXPECT_EQ(bountyTape, tape(std::pair(MONSTER_INVALID, false), std::pair(druid, false), std::pair(druid, true), std::pair(MONSTER_INVALID, false)));
    EXPECT_EQ(textTape, tape(std::string(),
                             "This month's bounty is on a " + name + ".  Kill it and return before the end of the month to collect the 2800 gold reward.",
                             std::string(),
                             "Congratulations on defeating the " + name + "!  Here is the 2800 gold reward.  Come back next month for a new bounty.",
                             std::string()));
    EXPECT_EQ(goldTape, tape(200, 3000)); // A level 28 monster.
    EXPECT_EQ(slotTape, tape("", "v7 oute3.odm"));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameSave, SavesWithAnArtifactAndARelic) {
    // The game couldn't be saved while the party carried an artifact or a relic, the save took only common items.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    auto morganTape = tapes.hasItem(kMorgan);
    auto aegisTape = tapes.hasItem(kAegis);
    auto statusTape = tapes.statusBar();
    auto slotTape = tapes.custom([&] { return std::filesystem::exists(profile / "saves/save000.ec6"); });
    test.startTaping();

    mm6::chestTitleState().lcgState = kHiveTreasureLuck;
    takeFromHiveChest4(game, kMorgan);
    takeFromHiveChest4(game, kAegis);
    saveFromMenu(game);

    EXPECT_EQ(morganTape, tape(false, true));
    EXPECT_EQ(aegisTape, tape(false, true));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_EQ(slotTape, tape(false, true));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6InGameSave, SavesWithASpecialBonus) {
    // The game couldn't be saved while the party carried an item with a special bonus other than Antique.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    auto spearTape = charTapes.custom(0, [](const Character &character) {
        InventoryConstEntry entry = character.inventory.find(kAcidSpear);
        return entry ? entry->specialEnchantment : ITEM_ENCHANTMENT_NULL;
    });
    auto statusTape = tapes.statusBar();
    auto slotTape = tapes.custom([&] { return std::filesystem::exists(profile / "saves/save000.ec6"); });
    test.startTaping();

    mm6::chestTitleState().lcgState = kHiveTreasureLuck;
    takeFromHiveChest4(game, kAcidSpear);
    saveFromMenu(game);

    EXPECT_EQ(spearTape, tape(ITEM_ENCHANTMENT_NULL, ITEM_ENCHANTMENT_OF_ACID));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_EQ(slotTape, tape(false, true));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}
