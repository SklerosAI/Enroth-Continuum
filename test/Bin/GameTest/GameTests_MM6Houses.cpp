#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6ShopStockIdentity.h"
#include "Engine/MM6/MM6ShopTitleState.h"
#include "Engine/Localization.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/HouseTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/ItemGrid.h"
#include "GUI/UI/MM6HouseServiceData.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"
#include "GUI/UI/Houses/MM6Shop.h"

#include "Library/Environment/Interface/Environment.h"
#include "Library/Image/Png.h"

#include "Media/MediaPlayer.h"

#include "Utility/Memory/Blob.h"
#include "Utility/ScopeGuard.h"
#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

static void startMM6NewGame(EngineController &game) {
    // MM6 has no new-game save yet, so the override seeds the map deltas and chest state from the game archives.
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        pMapDeltas.clear();
        for (const std::string &name : pGames_LOD->ls())
            if (name.ends_with(".dlv") || name.ends_with(".ddm"))
                pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
        mm6::resetChestTitleState(&mm6::chestTitleState());
        if (!mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(), 0x12345678u, {}))
            throw std::runtime_error("MM6 chest title state did not restore");
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    game.runGameRoutine([] {
        for (Character &character : pParty->pCharacters) {
            for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
                const Skill skill = character.GetSkillIdxByOrder(order);
                if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                    character.pActiveSkills[skill] = CombinedSkillValue::novice();
            }
        }
        for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
            for (Character &character : pParty->pCharacters)
                for (Attribute stat : allStatAttributes())
                    character.IncreaseAttribute(stat);
    });
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
}

static void enterThroughDoor(EngineController &game, Vec3f position, int viewYaw) {
    // Same map teleport, then Space at the door in front of the party.
    game.teleportTo(engine->_currentLoadedMapId, position, viewYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void waitUntilHour(EngineController &game, int hour) {
    // The party stands still while game time passes. A frame is 3 game seconds, so a day is 28800 frames.
    for (int frame = 0; frame < 28800; ++frame) {
        if (pParty->GetPlayingTime().toCivilTime().hour == hour)
            return;
        game.tick();
    }
    FAIL() << "Game time didn't reach " << hour << ":00 within a day";
}

static std::vector<int> setQuestBits() {
    // Every set quest bit, so that a tape shows a bit that changes and the ones that don't.
    std::vector<int> result;
    for (int bit = std::to_underlying(QBIT_FIRST); bit <= std::to_underlying(QBIT_LAST); ++bit)
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static std::vector<std::string> houseOptionLabels() {
    // The service and topic lines the house screen shows, without the hidden ones.
    std::vector<std::string> result;
    if (window_SpeakInHouse)
        for (GUIButton *button : window_SpeakInHouse->vButtons)
            if (button->id.contains("Dialogue_Option") && button->rect.h > 0)
                result.push_back(button->label);
    return result;
}

static void clickHouseOption(EngineController &game, std::string_view label) {
    // Left click in the middle of the shown option with this line.
    ASSERT_NE(window_SpeakInHouse, nullptr);
    std::optional<Pointi> center;
    for (GUIButton *button : window_SpeakInHouse->vButtons)
        if (button->id.contains("Dialogue_Option") && button->rect.h > 0 && button->label == label)
            center = button->rect.center();
    ASSERT_TRUE(center) << "No house option " << label;
    game.pressAndReleaseButton(BUTTON_LEFT, *center);
    game.tick();
}

static void typeAmount(EngineController &game, std::string_view digits) {
    // Types a number into the text input and confirms it with Enter.
    for (char digit : digits) {
        game.pressAndReleaseKey(static_cast<PlatformKey>(std::to_underlying(PlatformKey::KEY_DIGIT_0) + (digit - '0')));
        game.tick();
    }
    game.pressAndReleaseKey(PlatformKey::KEY_RETURN);
    game.tick();
}

static std::vector<int> shopStock(int house) {
    // Item ids in both stock banks of a shop.
    std::vector<int> result;
    for (mm6::ShopStockBank bank : {mm6::ShopStockBank::First, mm6::ShopStockBank::Second})
        for (int slot = 0; slot < 12; ++slot)
            result.push_back(mm6::shopTitleState().stock.records[mm6::shopStockRecordIndex(bank, house, slot, GameTitle::MM6)].itemNumberRaw());
    return result;
}

static std::vector<int> guildStock(int house) {
    // Item ids in a spell guild's stock.
    std::vector<int> result;
    for (int slot = 0; slot < 12; ++slot)
        result.push_back(mm6::shopTitleState().stock.records[mm6::guildStockRecordIndex(house, slot, GameTitle::MM6)].itemNumberRaw());
    return result;
}

static std::vector<int> backpackItems(int character) {
    // Item ids in a character's backpack, in inventory order.
    std::vector<int> result;
    for (InventoryConstEntry entry : pParty->pCharacters[character].inventory.entries())
        if (entry.zone() == INVENTORY_ZONE_GRID)
            result.push_back(std::to_underlying(entry->itemId));
    return result;
}

static InventoryEntry knightSword() {
    // The sword in the first character's backpack.
    for (InventoryEntry entry : pParty->pCharacters[0].inventory.entries())
        if (entry->itemId == static_cast<ItemId>(1))
            return entry;
    return {};
}

static void clickKnightSword(EngineController &game) {
    // Left click on the sword in the backpack a shop screen shows.
    ASSERT_TRUE(knightSword());
    game.pressAndReleaseButton(BUTTON_LEFT, mapFromInventoryGrid(knightSword().geometry().topLeft(), Pointi(14, 17)) + Pointi(16, 16));
    game.tick();
}

struct ScopedVendorRefresh {
    // Sets the MM6 vendor refresh on entry switch the way Atlas does, and clears it when the test is done with it.
    explicit ScopedVendorRefresh(bool on) {
        set(on);
    }
    ~ScopedVendorRefresh() {
        environment->setenv("ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY", "");
    }
    void set(bool on) {
        environment->setenv("ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY", on ? "1" : "0");
    }
    std::unique_ptr<Environment> environment = Environment::createStandardEnvironment();
};

static std::filesystem::path createTempProfile() {
    // An owner-only mm6-restart- folder in the system temp folder, the only kind of profile a test saves into.
    const std::filesystem::path tempRoot = std::filesystem::canonical(std::filesystem::temp_directory_path());
    std::string created = (tempRoot / "mm6-restart-houses-XXXXXX").string();
    if (!mkdtemp(created.data()))
        throw std::runtime_error("Can't create a temporary MM6 profile");
    return std::filesystem::canonical(created);
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

static void saveFromMenu(EngineController &game) {
    // Saves into a new slot from the Escape menu.
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
    game.doubleClickGuiButton("SaveMenu_Slot0"); // The new save slot.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_T);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.
}

static void loadFromMenu(EngineController &game) {
    // Loads the first numbered save from the Escape menu.
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_LoadGame");
    game.tick(3);
    const auto &slots = saveLoadMenu()->slots();
    const auto slot = std::ranges::find(slots, std::string("save000.ec6"), &SavegameSlot::fileName);
    ASSERT_NE(slot, slots.end());
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", slot - slots.begin()));
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
}

static void knockOut(Character *character) {
    character->health = 0;
    character->conditions.set(CONDITION_UNCONSCIOUS, pParty->GetPlayingTime());
}

static const Vec3f kTempleDoor(-9744, -1862, 700); // New Sorpigal Temple, facing north.
static const Vec3f kWeaponShopDoor(-8422, -7392, 300); // Weapon shop, facing east.
static const Vec3f kTownHallDoor(-11090, -6536, 300); // Town Hall, facing west.
static const Vec3f kGeneralStoreDoor(-10333, -8532, 300); // General store, facing south.
static const Vec3f kGuildDoor(-5384, -1276, 700); // Elemental guild.
static const Vec3f kTrainingDoor(-13874, -7032, 300); // Training Grounds, facing west.
static const Vec3f kTavernDoor(-8784, -8690, 300); // A Lonely Knight, facing east.
static const Vec3f kBankDoor(-10800, -7416, 300); // In front of the bank, facing west.
static constexpr int kBankDoorFace = 1928;
static const Vec3f kBufordHouseDoor(-7014, -5798, 300); // In front of Buford T. Allman's house, facing north.
static const Vec3f kStableDoor(-10346, -9472, 161); // In front of the stable, facing west.
static const Vec3f kSelfGuildDoor(-13952, -684, 576); // Initiate Guild of the Self, on the ground south of its door.
static const Vec3f kHaroldHessDoor(-5976, -8068, 160); // Harold Hess's house, facing south.
static const Vec3f kBladesEndDoor(-6504, -6090, 300); // Blades' End, a mercenary guild.
static const Vec3f kBuccaneersLairDoor(-12632, -5946, 300); // Buccaneers' Lair, a thieves' guild.
static const Vec3f kIsaoHouseDoor(-12528, -5814, 300); // In front of Isao Magistrus' house, facing north.
static const Vec3f kJezebelHouseDoor(4688, -17668, 192); // In front of Jezebel's house on the Misty Islands, facing south.
static constexpr AwardId kElementalGuildMember = static_cast<AwardId>(64);
static constexpr AwardId kSelfGuildMember = static_cast<AwardId>(65);
static constexpr AwardId kBuccaneersLairMember = static_cast<AwardId>(66);
static constexpr AwardId kBladesEndMember = static_cast<AwardId>(69);

static std::vector<std::pair<std::string, Recti>> houseOptions() {
    // The service and topic lines the house screen shows with their buttons, without the hidden ones.
    std::vector<std::pair<std::string, Recti>> result;
    if (window_SpeakInHouse)
        for (GUIButton *button : window_SpeakInHouse->vButtons)
            if (button->id.contains("Dialogue_Option") && button->rect.h > 0)
                result.emplace_back(button->label, button->rect);
    return result;
}

static std::vector<DrawnLine> panelLines(const AccessibleVector<DrawnLine> &frame) {
    // Lines drawn in the right panel below the resident's name.
    std::vector<DrawnLine> result;
    for (const DrawnLine &line : frame)
        if (line.position.x >= 483 && line.position.y >= 140)
            result.push_back(line);
    return result;
}

static bool isHousePanelQuad(const DrawnQuad &quad) {
    // The house's right panel, its portrait and the exit picture, without the HUD under them and the cursor over them.
    return quad.texture.starts_with("evpan") || quad.texture.starts_with("npc") || quad.texture == "buttesc1";
}

static DrawnLine panelLine(std::string_view text, int y, Color color) {
    // A line centred in the right panel's text column the way MM6 centres it.
    return DrawnLine{std::string(text), {483 + assets->pFontArrus->AlignText_Center(148, text), y}, color};
}

static std::vector<DrawnLine> noConditionLines() {
    // The first character's no condition refusal, wrapped in the right panel and centred on it the way MM6 places it.
    const Color yellow = colorTable.PaleCanary;
    return {panelLine("Zoltan is in no", 183, yellow), panelLine("condition to do", 199, yellow), panelLine("anything", 215, yellow)};
}

GAME_TEST(MM6Houses, EnterAndExit) {
    // Space at a New Sorpigal house door opens its house screen, and Exit returns the party to the world.
    auto screenTape = tapes.screen();
    auto houseTape = tapes.house();
    auto mapTape = tapes.map();
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kTempleDoor, 90);
    game.pressGuiButton("House_Exit");
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE, SCREEN_GAME));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(69), HOUSE_INVALID));
    EXPECT_EQ(mapTape, tape(pMapTable->GetMapInfo("oute3.odm")));
}

GAME_TEST(MM6Houses, TempleHeal) {
    // New Sorpigal Temple heals a poisoned character for gold, and doesn't offer to heal an eradicated one.
    auto poisonedTape = tapes.custom([] { return pParty->pCharacters[0].conditions.has(CONDITION_POISON_WEAK); });
    auto eradicatedTape = tapes.custom([] { return pParty->pCharacters[1].conditions.has(CONDITION_ERADICATED); });
    auto healthTape = tapes.custom([] { return std::vector<int>{pParty->pCharacters[0].health, pParty->pCharacters[1].health}; });
    auto goldTape = tapes.gold();
    auto screenTape = tapes.screen();
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto soundsTape = tapes.sounds();
    auto portraitTape = charTapes.portrait(0);
    startMM6NewGame(game);
    pParty->pCharacters[0].SetCondition(CONDITION_POISON_WEAK, 0);
    pParty->pCharacters[0].health = 1;
    pParty->pCharacters[1].SetCondition(CONDITION_ERADICATED, 0);
    pParty->pCharacters[1].health = 1;
    test.startTaping();
    enterThroughDoor(game, kTempleDoor, 90);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    game.tick();
    game.pressGuiButton("HouseDialogue_Option0"); // Heal.
    game.tick();
    game.pressGuiButton("House_Exit");
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE, SCREEN_GAME));
    EXPECT_EQ(poisonedTape, tape(true, false));
    EXPECT_EQ(eradicatedTape, tape(true));
    EXPECT_EQ(healthTape, tape(std::vector<int>{1, 1}, std::vector<int>{pParty->pCharacters[0].GetMaxHealth(), 1}));
    EXPECT_EQ(goldTape, tape(200, 190));
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_heal);
    EXPECT_CONTAINS(portraitTape, PORTRAIT_WIDE_SMILE); // The temple heal reaction.
    const std::string donate = localization->str(LSTR_DONATE);
    const std::string heal = fmt::format("{} 10 {}", localization->str(LSTR_HEAL), localization->str(LSTR_GOLD));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, std::vector<std::string>{heal, donate}, std::vector<std::string>{donate},
                                std::vector<std::string>{heal, donate}, std::vector<std::string>{donate}, std::vector<std::string>{}));
}

GAME_TEST(MM6Houses, HouseNpcTopic) {
    // Frank in the Town Hall can be picked, and his topic sets a quest bit, replaces itself and shows its text.
    auto questBitsTape = tapes.custom([] { return setQuestBits(); });
    auto topicsTape = tapes.custom([] {
        const NPCData &frank = pNPCStats->pNPCData[3];
        return std::vector<unsigned>{frank.dialogue_1_evt_id, frank.dialogue_2_evt_id, frank.dialogue_3_evt_id};
    });
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto goldTape = tapes.gold();
    auto screenTape = tapes.screen();
    startMM6NewGame(game);
    waitUntilHour(game, 10); // The Town Hall opens at 10 am.
    test.startTaping();
    enterThroughDoor(game, kTownHallDoor, 180);
    game.pressGuiButton("House_Npc0"); // Frank Fairchild.
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE));
    EXPECT_EQ(questBitsTape, tape(std::vector<int>{81, 181}, std::vector<int>{81, 138, 181}));
    EXPECT_EQ(topicsTape, tape(std::vector<unsigned>{324, 326, 0}, std::vector<unsigned>{325, 326, 0}));
    const EvtInstruction &message = engine->_globalEventMap.function(324).front();
    ASSERT_EQ(message.opcode, EVENT_ShowMessage);
    EXPECT_EQ(textTape, tape(std::string(), pNPCTopics[message.data.text_id - 1].pText));
    EXPECT_EQ(goldTape.size(), 1);
}

GAME_TEST(MM6Houses, TempleHealWithoutGold) {
    // New Sorpigal Temple refuses a heal the party can't pay for, with the not enough gold line and the priest's sound.
    auto poisonedTape = tapes.custom([] { return pParty->pCharacters[0].conditions.has(CONDITION_POISON_WEAK); });
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    auto soundsTape = tapes.sounds();
    startMM6NewGame(game);
    pParty->pCharacters[0].SetCondition(CONDITION_POISON_WEAK, 0);
    pParty->SetGold(5);
    test.startTaping();
    enterThroughDoor(game, kTempleDoor, 90);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    game.tick();
    game.pressGuiButton("HouseDialogue_Option0"); // Heal for 10 gold.
    game.tick();
    EXPECT_EQ(poisonedTape, tape(true));
    EXPECT_EQ(goldTape, tape(5));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD));
    EXPECT_CONTAINS(soundsTape.flatten(), static_cast<SoundId>(31702));
    EXPECT_MISSES(soundsTape.flatten(), SOUND_heal);
}

GAME_TEST(MM6Houses, ClosedHouse) {
    // The Town Hall turns the party away at 9 am with its opening hours, and lets it in once it opens at 10 am.
    auto screenTape = tapes.screen();
    auto houseTape = tapes.house();
    auto statusTape = tapes.statusBar();
    auto portraitTape = charTapes.portrait(0);
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kTownHallDoor, 180);
    ASSERT_EQ(screenTape, tape(SCREEN_GAME));
    waitUntilHour(game, 10);
    enterThroughDoor(game, kTownHallDoor, 180);
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(89)));
    EXPECT_CONTAINS(statusTape, localization->format(LSTR_THIS_PLACE_IS_OPEN_FROM_DS_TO_DS, 10, localization->amPm(false), 2, localization->amPm(true)));
    EXPECT_CONTAINS(portraitTape, PORTRAIT_NO); // The store closed reaction.
}

GAME_TEST(MM6Houses, GeneralStoreSell) {
    // The New Sorpigal general store buys an item from the active character's backpack and pays half the sell price.
    auto goldTape = tapes.gold();
    auto backpackTape = tapes.custom([] { return backpackItems(0); });
    auto otherBackpackTape = tapes.custom([] { return backpackItems(1); });
    auto screenTape = tapes.screen();
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kGeneralStoreDoor, 1536);
    game.pressGuiButton("HouseDialogue_Option1"); // Sell Items.
    game.tick();
    InventoryEntry sold;
    for (InventoryEntry entry : pParty->pCharacters[0].inventory.entries())
        if (entry.zone() == INVENTORY_ZONE_GRID && !sold)
            sold = entry;
    ASSERT_TRUE(sold);
    game.pressAndReleaseButton(BUTTON_LEFT, mapFromInventoryGrid(sold.geometry().topLeft(), Pointi(14, 17)) + Pointi(16, 16));
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE));
    EXPECT_EQ(backpackTape, tape(std::vector<int>{124, 1, 15, 23, 66, 505}, std::vector<int>{1, 15, 23, 66, 505}));
    EXPECT_EQ(otherBackpackTape.size(), 1);
    EXPECT_EQ(goldTape, tape(200, 312));
}

GAME_TEST(MM6Houses, WeaponShopBuyAndSellBack) {
    // The New Sorpigal weapon shop sells an item from its generated stock for gold, and buys it back for less.
    auto goldTape = tapes.gold();
    auto stockTape = tapes.custom([] { return shopStock(1); });
    auto backpackTape = tapes.custom([] { return backpackItems(0); });
    auto otherBackpackTape = tapes.custom([] { return backpackItems(1); });
    auto clockTape = tapes.custom([] { return mm6::shopTitleState().clocks.deadlineWords[mm6::shopClockSlot(1)] != 0; });
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kWeaponShopDoor, 0);
    game.pressGuiButton("HouseDialogue_Option0"); // Buy Items.
    game.tick();
    GUIWindow_MM6Shop *shop = dynamic_cast<GUIWindow_MM6Shop *>(window_SpeakInHouse.get());
    ASSERT_NE(shop, nullptr);
    game.pressAndReleaseButton(BUTTON_LEFT, shop->stockItemRect(0).center());
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton("HouseDialogue_Option1"); // Sell Items.
    game.tick();
    InventoryEntry bought;
    for (InventoryEntry entry : pParty->pCharacters[0].inventory.entries())
        if (entry.zone() == INVENTORY_ZONE_GRID && entry->mm6RawRecord)
            bought = entry;
    ASSERT_TRUE(bought);
    game.pressAndReleaseButton(BUTTON_LEFT, mapFromInventoryGrid(bought.geometry().topLeft(), Pointi(14, 17)) + Pointi(16, 16));
    game.tick();
    const std::vector<int> generated = {50, 55, 31, 1, 20, 24, 0, 0, 0, 0, 0, 0, 20, 20, 16, 20, 17, 15, 0, 0, 0, 0, 0, 0};
    std::vector<int> afterSale = generated;
    afterSale[0] = 0;
    EXPECT_EQ(stockTape, tape(std::vector<int>(24, 0), generated, afterSale));
    EXPECT_EQ(backpackTape, tape(std::vector<int>{124, 1, 15, 23, 66, 505}, std::vector<int>{124, 1, 15, 23, 66, 505, 50},
                                 std::vector<int>{124, 1, 15, 23, 66, 505}));
    EXPECT_EQ(otherBackpackTape.size(), 1);
    EXPECT_EQ(goldTape, tape(200, 125, 139));
    EXPECT_EQ(clockTape, tape(false, true));
}

GAME_TEST(MM6Houses, WeaponShopRefusesBlaster) {
    // The New Sorpigal weapon shop bought a blaster for 1 gold.
    auto goldTape = tapes.gold();
    auto backpackTape = tapes.custom([] { return backpackItems(0); });
    startMM6NewGame(game);
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(64)));
    test.startTaping();
    enterThroughDoor(game, kWeaponShopDoor, 0);
    game.pressGuiButton("HouseDialogue_Option1"); // Sell Items.
    game.tick();
    for (ItemId item : {static_cast<ItemId>(64), static_cast<ItemId>(1)}) { // The blaster, then the knight's sword.
        InventoryEntry entry;
        for (InventoryEntry candidate : pParty->pCharacters[0].inventory.entries())
            if (candidate->itemId == item)
                entry = candidate;
        ASSERT_TRUE(entry);
        game.pressAndReleaseButton(BUTTON_LEFT, mapFromInventoryGrid(entry.geometry().topLeft(), Pointi(14, 17)) + Pointi(16, 16));
        game.tick();
    }
    EXPECT_EQ(backpackTape, tape(std::vector<int>{124, 1, 15, 23, 66, 505, 64}, std::vector<int>{124, 15, 23, 66, 505, 64}));
    EXPECT_EQ(goldTape, tape(200, 214)); // The sword alone.
}

GAME_TEST(MM6Houses, WeaponShopStockGivesEveryCandidateItsWeight) {
    // The roll for a shop item of a given kind started at 0, so the first candidate got one chance more than its weight
    // and the last one one less. On a new game, the draw for the sixth weapon of New Sorpigal's weapon shop is the Dark
    // Hammer's last number, and the fixed roll, one higher, is the Club's first. The sixth special item moves from the
    // Dagger to the Blood Dagger the same way.
    for (bool fix : {false, true}) {
        SCOPED_TRACE(fmt::format("fix={}", fix));
        test.prepareForNextTest();
        engine->config->gameplay.FixItemGeneration.setValue(fix);
        auto stockTape = tapes.custom([] { return shopStock(1); });
        auto quadsTape = tapes.hudQuads();
        startNewGameThroughInput(game);
        test.startTaping();
        enterThroughDoor(game, kWeaponShopDoor, 0);
        clickHouseOption(game, localization->str(static_cast<LstrId>(539))); // Buy Items.

        const int sixthWeapon = fix ? 58 : 56; // Club or Dark Hammer.
        const int sixthSpecial = fix ? 16 : 15; // Blood Dagger or Dagger.
        EXPECT_EQ(stockTape, tape(std::vector<int>(24, 0),
                                  std::vector<int>{51, 15, 58, 31, 31, sixthWeapon, 0, 0, 0, 0, 0, 0, 21, 16, 15, 15, 15, sixthSpecial, 0, 0, 0, 0, 0, 0}));
        const auto shelfCount = [&](std::string_view icon) {
            return quadsTape.back().count([icon](const DrawnQuad &quad) { return quad.texture == icon; });
        };
        EXPECT_EQ(shelfCount("hammer2"), fix ? 0 : 1);
        EXPECT_EQ(shelfCount("club1"), fix ? 2 : 1);
    }
}

GAME_TEST(MM6Houses, GuildBuySpellbook) {
    // A New Sorpigal spell guild sells a spellbook from its generated stock for gold.
    auto goldTape = tapes.gold();
    auto stockTape = tapes.custom([] { return guildStock(137); });
    auto backpackTape = tapes.custom([] { return backpackItems(0); });
    startMM6NewGame(game);
    pParty->pCharacters[0]._achievedAwardsBits.set(kElementalGuildMember);
    test.startTaping();
    enterThroughDoor(game, kGuildDoor, 1536);
    game.pressGuiButton("HouseDialogue_Option0"); // Buy Spells.
    game.tick();
    GUIWindow_MM6Shop *guild = dynamic_cast<GUIWindow_MM6Shop *>(window_SpeakInHouse.get());
    ASSERT_NE(guild, nullptr);
    game.pressAndReleaseButton(BUTTON_LEFT, guild->stockItemRect(0).center());
    game.tick();
    const std::vector<int> generated = {322, 322, 301, 336, 333, 301, 313, 333, 314, 300, 325, 314};
    std::vector<int> afterSale = generated;
    afterSale[0] = 0;
    EXPECT_EQ(stockTape, tape(std::vector<int>(12, 0), generated, afterSale));
    EXPECT_EQ(backpackTape, tape(std::vector<int>{124, 1, 15, 23, 66, 505}, std::vector<int>{124, 1, 15, 23, 66, 505, 322}));
    EXPECT_EQ(goldTape, tape(200, 50));
}

GAME_TEST(MM6Houses, WeaponShopVendorRefreshOnEntry) {
    // With vendor refresh on entry the New Sorpigal weapon shop restocks once per visit, so the slot the party bought
    // out is filled again on its next visit. Without it the slot stays empty until the stock deadline.
    for (bool refresh : {false, true}) {
        SCOPED_TRACE(fmt::format("refresh={}", refresh));
        test.prepareForNextTest();
        ScopedVendorRefresh vendorRefresh(refresh);
        auto goldTape = tapes.gold();
        auto stockTape = tapes.custom([] { return shopStock(1); });
        auto backpackTape = tapes.custom([] { return backpackItems(0); });
        auto deadlineTape = tapes.custom([] { return mm6::shopTitleState().clocks.deadlineWords[mm6::shopClockSlot(1)]; });
        auto houseTape = tapes.house();
        startMM6NewGame(game);
        test.startTaping();
        enterThroughDoor(game, kWeaponShopDoor, 0);
        clickHouseOption(game, localization->str(static_cast<LstrId>(539))); // Buy Items.
        GUIWindow_MM6Shop *shop = dynamic_cast<GUIWindow_MM6Shop *>(window_SpeakInHouse.get());
        ASSERT_NE(shop, nullptr);
        game.pressAndReleaseButton(BUTTON_LEFT, shop->stockItemRect(0).center());
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressGuiButton("House_Exit");
        game.tick();
        enterThroughDoor(game, kWeaponShopDoor, 0);
        clickHouseOption(game, localization->str(static_cast<LstrId>(539))); // Buy Items.
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        clickHouseOption(game, localization->str(static_cast<LstrId>(543))); // Special Items.

        const std::vector<int> generated = {50, 55, 31, 1, 20, 24, 0, 0, 0, 0, 0, 0, 20, 20, 16, 20, 17, 15, 0, 0, 0, 0, 0, 0};
        std::vector<int> afterSale = generated;
        afterSale[0] = 0;
        EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(1), HOUSE_INVALID, static_cast<HouseId>(1)));
        EXPECT_EQ(goldTape, tape(200, 125));
        EXPECT_EQ(backpackTape, tape(std::vector<int>{124, 1, 15, 23, 66, 505}, std::vector<int>{124, 1, 15, 23, 66, 505, 50}));
        if (refresh) {
            const std::vector<int> restocked = {12, 12, 15, 50, 31, 51, 0, 0, 0, 0, 0, 0, 20, 21, 17, 16, 20, 16, 0, 0, 0, 0, 0, 0};
            EXPECT_EQ(stockTape, tape(std::vector<int>(24, 0), generated, afterSale, restocked));
            ASSERT_EQ(deadlineTape.size(), 3);
            EXPECT_GT(deadlineTape.back(), deadlineTape[1]);
        } else {
            EXPECT_EQ(stockTape, tape(std::vector<int>(24, 0), generated, afterSale));
            EXPECT_EQ(deadlineTape.size(), 2);
        }
    }
}

GAME_TEST(MM6Houses, GuildVendorRefreshOnEntry) {
    // With vendor refresh on entry a New Sorpigal spell guild restocks once per visit, so the book the party bought is
    // replaced on its next visit. Without it the slot stays empty until the stock deadline.
    for (bool refresh : {false, true}) {
        SCOPED_TRACE(fmt::format("refresh={}", refresh));
        test.prepareForNextTest();
        ScopedVendorRefresh vendorRefresh(refresh);
        auto goldTape = tapes.gold();
        auto stockTape = tapes.custom([] { return guildStock(137); });
        auto backpackTape = tapes.custom([] { return backpackItems(0); });
        auto houseTape = tapes.house();
        startMM6NewGame(game);
        pParty->pCharacters[0]._achievedAwardsBits.set(kElementalGuildMember);
        test.startTaping();
        enterThroughDoor(game, kGuildDoor, 1536);
        clickHouseOption(game, localization->str(static_cast<LstrId>(400))); // Buy Spells.
        GUIWindow_MM6Shop *guild = dynamic_cast<GUIWindow_MM6Shop *>(window_SpeakInHouse.get());
        ASSERT_NE(guild, nullptr);
        game.pressAndReleaseButton(BUTTON_LEFT, guild->stockItemRect(0).center());
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressGuiButton("House_Exit");
        game.tick();
        enterThroughDoor(game, kGuildDoor, 1536);
        clickHouseOption(game, localization->str(static_cast<LstrId>(400))); // Buy Spells.
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        clickHouseOption(game, localization->str(static_cast<LstrId>(400))); // Buy Spells.

        const std::vector<int> generated = {322, 322, 301, 336, 333, 301, 313, 333, 314, 300, 325, 314};
        std::vector<int> afterSale = generated;
        afterSale[0] = 0;
        EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(137), HOUSE_INVALID, static_cast<HouseId>(137)));
        EXPECT_EQ(goldTape, tape(200, 50));
        EXPECT_EQ(backpackTape, tape(std::vector<int>{124, 1, 15, 23, 66, 505}, std::vector<int>{124, 1, 15, 23, 66, 505, 322}));
        if (refresh) {
            const std::vector<int> restocked = {323, 335, 312, 301, 324, 334, 314, 314, 322, 325, 314, 336};
            EXPECT_EQ(stockTape, tape(std::vector<int>(12, 0), generated, afterSale, restocked));
        } else {
            EXPECT_EQ(stockTape, tape(std::vector<int>(12, 0), generated, afterSale));
        }
    }
}

GAME_TEST(MM6Houses, VendorRefreshStockSavesAsStock) {
    // Stock that vendor refresh on entry rolled is saved and loaded like any other stock, and with the switch off the
    // shop keeps it until its deadline.
    const std::filesystem::path profile = createTempProfile();
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });
    MM_AT_SCOPE_EXIT(game.runGameRoutine([] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    }));
    ScopedVendorRefresh vendorRefresh(true);
    auto goldTape = tapes.gold();
    auto stockTape = tapes.custom([] { return shopStock(1); });
    auto statusTape = tapes.statusBar();
    startNewGameThroughInput(game);
    test.startTaping();
    const auto visitWeaponShop = [&](bool buy) {
        enterThroughDoor(game, kWeaponShopDoor, 0);
        clickHouseOption(game, localization->str(static_cast<LstrId>(539))); // Buy Items.
        if (buy) {
            GUIWindow_MM6Shop *shop = dynamic_cast<GUIWindow_MM6Shop *>(window_SpeakInHouse.get());
            ASSERT_NE(shop, nullptr);
            game.pressAndReleaseButton(BUTTON_LEFT, shop->stockItemRect(0).center());
            game.tick();
        }
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressGuiButton("House_Exit");
        game.tick();
    };
    visitWeaponShop(false);
    visitWeaponShop(false);
    saveFromMenu(game);
    vendorRefresh.set(false);
    visitWeaponShop(true);
    loadFromMenu(game);
    visitWeaponShop(false);

    const std::vector<int> generated = {51, 15, 58, 31, 31, 58, 0, 0, 0, 0, 0, 0, 21, 16, 15, 15, 15, 16, 0, 0, 0, 0, 0, 0};
    const std::vector<int> restocked = {15, 50, 15, 12, 58, 51, 0, 0, 0, 0, 0, 0, 15, 20, 16, 15, 21, 16, 0, 0, 0, 0, 0, 0};
    std::vector<int> restockedAfterSale = restocked;
    restockedAfterSale[0] = 0;
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_EQ(stockTape, tape(std::vector<int>(24, 0), generated, restocked, restockedAfterSale, restocked));
    EXPECT_EQ(goldTape, tape(200, 188, 200));
}

GAME_TEST(MM6Houses, Training) {
    // New Sorpigal Training Grounds trains two characters for gold, and only the first level of the visit moves time.
    auto levelsTape = charTapes.levels();
    auto experienceTape = charTapes.experiences();
    auto skillPointsTape = charTapes.custom([](const Character &character) { return character.uSkillPoints; });
    auto goldTape = tapes.gold();
    auto dayHourTape = tapes.custom([] {
        const CivilTime time = pParty->GetPlayingTime().toCivilTime();
        return std::pair(time.day, time.hour);
    });
    auto soundsTape = tapes.sounds();
    auto statusTape = tapes.statusBar();
    auto screenTape = tapes.screen();
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    startMM6NewGame(game);
    pParty->pCharacters[0].experience = 1000;
    pParty->pCharacters[1].experience = 1000;
    test.startTaping();
    enterThroughDoor(game, kTrainingDoor, 180);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    game.tick();
    game.pressGuiButton("HouseDialogue_Option0"); // Train.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();
    game.pressGuiButton("HouseDialogue_Option0");
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE));
    EXPECT_EQ(levelsTape, tape({1, 1, 1, 1}, {2, 1, 1, 1}, {2, 2, 1, 1}));
    EXPECT_EQ(experienceTape.size(), 1);
    EXPECT_EQ(skillPointsTape.delta(), tape(5, 5, 0, 0));
    EXPECT_EQ(goldTape, tape(200, 190, 180));
    EXPECT_EQ(dayHourTape, tape(std::pair(1, 9), std::pair(9, 9)));
    const std::string &first = pParty->pCharacters[0].name;
    const std::string &second = pParty->pCharacters[1].name;
    EXPECT_CONTAINS(statusTape, localization->format(LSTR_S_IS_NOW_LEVEL_LU_AND_HAS_EARNED_LU, first, 2, 5));
    EXPECT_CONTAINS(statusTape, localization->format(LSTR_S_IS_NOW_LEVEL_LU_AND_HAS_EARNED_LU, second, 2, 5));
    const std::string train = localization->format(LSTR_TRAIN_TO_LEVEL_D_FOR_D_GOLD, 2, 10);
    const std::string needMore = localization->format(LSTR_YOU_NEED_D_MORE_EXPERIENCE_TO_TRAIN_TO, 2000, 3);
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, std::vector<std::string>{train}, std::vector<std::string>{needMore},
                                std::vector<std::string>{train}, std::vector<std::string>{needMore}));
    EXPECT_EQ(soundsTape.flatten(), tape(SOUND_gold01, static_cast<SoundId>(30702), SOUND_gold01, static_cast<SoundId>(30702))); // Paying, then the trainer.
}

GAME_TEST(MM6Houses, TavernMenu) {
    // A Lonely Knight lists a room and packs with their prices, a drink and a tip, in the original order.
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto screenTape = tapes.screen();
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kTavernDoor, 0);
    game.pressGuiButton("House_Npc0"); // The proprietor, the tavern has a resident NPC too.
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE));
    const std::vector<std::string> menu = {localization->format(LSTR_RENT_ROOM_FOR_D_GOLD, 3), localization->format(LSTR_FILL_PACKS_TO_D_DAYS_FOR_D_GOLD, 6, 2),
                                           localization->str(LSTR_HAVE_A_DRINK), localization->str(static_cast<LstrId>(399))};
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, menu));
}

GAME_TEST(MM6Houses, ExitPicture) {
    // The exit picture under the right panel didn't leave the house, the exit button sat over the HUD buttons instead.
    auto screenTape = tapes.screen();
    auto houseTape = tapes.house();
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kTempleDoor, 90);
    game.pressAndReleaseButton(BUTTON_LEFT, {556, 329});
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE, SCREEN_GAME));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(69), HOUSE_INVALID));
}

GAME_TEST(MM6Houses, TempleHealAtItsLine) {
    // Clicking Heal where MM6 draws it hit Donate, the options sat on a fixed 30-pixel grid.
    auto poisonedTape = tapes.custom([] { return pParty->pCharacters[0].conditions.has(CONDITION_POISON_WEAK); });
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    startMM6NewGame(game);
    pParty->pCharacters[0].SetCondition(CONDITION_POISON_WEAK, 0);
    test.startTaping();
    enterThroughDoor(game, kTempleDoor, 90);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {556, 200});
    game.tick();
    EXPECT_EQ(poisonedTape, tape(true, false));
    EXPECT_EQ(goldTape, tape(200, 190));
    EXPECT_MISSES(statusTape, "This service is not available yet.");
}

GAME_TEST(MM6Houses, GeneralStoreShelf) {
    // General store items floated half way up the wall, so a click on the shelf they stand on in MM6 bought nothing.
    auto goldTape = tapes.gold();
    auto backpackTape = tapes.custom([] { return backpackItems(0); });
    auto otherBackpackTape = tapes.custom([] { return backpackItems(1); });
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kGeneralStoreDoor, 1536);
    game.pressGuiButton("HouseDialogue_Option0"); // Buy Items.
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT, {40, 303}); // Just above the shelf, in the middle of the first item.
    game.tick();
    EXPECT_EQ(backpackTape, tape(std::vector<int>{124, 1, 15, 23, 66, 505}, std::vector<int>{124, 1, 15, 23, 66, 505, 115}));
    EXPECT_EQ(otherBackpackTape.size(), 1);
    EXPECT_EQ(goldTape, tape(200, 100));
}

GAME_TEST(MM6Houses, GuildMenu) {
    // MM6 spell guilds listed Buy Spells and Learn on the generic menu grid and served anyone. A guild turns away a
    // non-member, and lists a member the skills it teaches that the member's class can learn and doesn't know yet,
    // under their price. Choosing one teaches it.
    auto optionsTape = tapes.custom([] { return houseOptions(); });
    auto linesTape = tapes.titleLines();
    auto quadsTape = tapes.hudQuads();
    auto skillsTape = tapes.custom([] {
        // The levels of the four skills the guild teaches, character by character.
        std::vector<int> result;
        for (const Character &character : pParty->pCharacters)
            for (Skill skill : {SKILL_FIRE, SKILL_AIR, SKILL_WATER, SKILL_EARTH})
                result.push_back(character.pActiveSkills[skill].level());
        return result;
    });
    auto goldTape = tapes.gold();
    startMM6NewGame(game);
    pParty->pCharacters[2]._achievedAwardsBits.set(kElementalGuildMember); // Serena, a sorcerer who knows Fire Magic.
    pParty->SetGold(1000);
    test.startTaping();
    enterThroughDoor(game, kGuildDoor, 1536);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    game.tick();
    const std::vector<DrawnLine> refusal = panelLines(linesTape.back());
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_3);
    game.tick();
    const std::vector<DrawnLine> menu = panelLines(linesTape.back());
    game.pressGuiButton("HouseDialogue_Option2"); // Air Magic.
    game.tick();

    const Color white = colorTable.White;
    const Color yellow = colorTable.PaleCanary;
    EXPECT_EQ(refusal, std::vector<DrawnLine>({panelLine("You must be a", 175, yellow), panelLine("member of this", 191, yellow),
                                               panelLine("guild to study", 207, yellow), panelLine("here", 223, yellow)}));
    EXPECT_EQ(menu, std::vector<DrawnLine>({panelLine("Buy Spells", 173, yellow), panelLine("Air Magic", 209, white), panelLine("Water Magic", 245, white),
                                            panelLine("Earth Magic", 281, white), panelLine("Skill Cost: 750", 146, white)}));
    EXPECT_EQ(panelLines(linesTape.back()), std::vector<DrawnLine>({panelLine("Buy Spells", 180, white), panelLine("Water Magic", 227, white),
                                                                    panelLine("Earth Magic", 274, white), panelLine("Skill Cost: 750", 146, white)}));
    using Options = std::vector<std::pair<std::string, Recti>>;
    EXPECT_EQ(optionsTape, tape(Options(),
                                Options({{"Buy Spells", Recti(480, 173, 140, 16)}, {"Air Magic", Recti(480, 209, 140, 16)},
                                         {"Water Magic", Recti(480, 245, 140, 16)}, {"Earth Magic", Recti(480, 281, 140, 16)}}),
                                Options({{"Buy Spells", Recti(480, 180, 140, 16)}, {"Water Magic", Recti(480, 227, 140, 16)},
                                         {"Earth Magic", Recti(480, 274, 140, 16)}})));
    EXPECT_EQ(skillsTape, tape(std::vector<int>{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1}, std::vector<int>{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1}));
    EXPECT_EQ(goldTape, tape(1000, 250));
    EXPECT_EQ(quadsTape.back().filter(isHousePanelQuad),
              tape(DrawnQuad{"evpan043", {481, 0}}, DrawnQuad{"npc512", {525, 34}}, DrawnQuad{"buttesc1", {526, 313}}));
}

GAME_TEST(MM6Houses, BankMenu) {
    // The bank listed Deposit and Withdraw on the generic menu grid and didn't show the party's balance.
    auto optionsTape = tapes.custom([] { return houseOptions(); });
    auto linesTape = tapes.titleLines();
    auto quadsTape = tapes.hudQuads();
    startMM6NewGame(game);
    test.startTaping();
    game.teleportTo(engine->_currentLoadedMapId, kBankDoor, 180);
    game.pointMouseAtFace(kBankDoorFace);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();

    EXPECT_EQ(panelLines(linesTape.back()), std::vector<DrawnLine>({panelLine("Deposit", 146, colorTable.PaleCanary), panelLine("Withdraw", 176, colorTable.White),
                                                                    panelLine("Balance: 0", 220, colorTable.PaleCanary)}));
    using Options = std::vector<std::pair<std::string, Recti>>;
    EXPECT_EQ(optionsTape, tape(Options(), Options({{"Deposit", Recti(480, 146, 140, 30)}, {"Withdraw", Recti(480, 176, 140, 30)}})));
    EXPECT_EQ(quadsTape.back().filter(isHousePanelQuad),
              tape(DrawnQuad{"evpan006", {481, 0}}, DrawnQuad{"npc504", {525, 34}}, DrawnQuad{"buttesc1", {526, 313}}));
}

GAME_TEST(MM6Houses, StableMenu) {
    // The stable drew its price above MM6's line and listed its routes on the generic menu grid. The price heads one
    // block of lines with a blank line under each, and a route's button covers its lines.
    auto optionsTape = tapes.custom([] { return houseOptions(); });
    auto linesTape = tapes.titleLines();
    auto quadsTape = tapes.hudQuads();
    startMM6NewGame(game);
    test.startTaping();
    game.teleportTo(engine->_currentLoadedMapId, kStableDoor, 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();

    const Color white = colorTable.White;
    const Color yellow = colorTable.PaleCanary;
    EXPECT_EQ(panelLines(linesTape.back()), std::vector<DrawnLine>({panelLine("2 days to Castle", 194, yellow), panelLine("Ironfist", 210, yellow),
                                                                    panelLine("Travel Cost 25", 146, white), panelLine("gold", 162, white)}));
    using Options = std::vector<std::pair<std::string, Recti>>;
    EXPECT_EQ(optionsTape, tape(Options(), Options({{"2 days to Castle Ironfist", Recti(480, 194, 140, 32)}})));
    EXPECT_EQ(quadsTape.back().filter(isHousePanelQuad),
              tape(DrawnQuad{"evpan033", {481, 0}}, DrawnQuad{"npc385", {525, 34}}, DrawnQuad{"buttesc1", {526, 313}}));
}

GAME_TEST(MM6Houses, NpcHouseProfession) {
    // Buford T. Allman's house named him "the Time in Enroth", MM7's id for his profession read MM6's calendar string.
    // MM6 names him by his profession, a Barrister.
    auto linesTape = tapes.titleLines();
    auto houseTape = tapes.house();
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kBufordHouseDoor, 90);

    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(470)));
    auto nameLines = linesTape.back().filter([](const DrawnLine &line) { return line.position.y >= 100 && line.position.y < 140; });
    auto nameLine = [](std::string_view text, int y) {
        return DrawnLine{std::string(text), {483 + assets->pFontCreate->AlignText_Center(147, text), y}, colorTable.EasternBlue};
    };
    EXPECT_EQ(nameLines, tape(nameLine("Buford T. Allman", 108), nameLine("the Barrister", 123)));
}

GAME_TEST(MM6Houses, MercenaryGuildMenu) {
    // Blades' End, a mercenary guild, showed its proprietor with an empty menu. It turns away a non-member, and lists
    // a member the skills it teaches that the member's class can learn and doesn't know yet, under their price.
    // Choosing one teaches it, and once the member knows them all the guild sends the member elsewhere.
    auto optionsTape = tapes.custom([] { return houseOptions(); });
    auto linesTape = tapes.titleLines();
    auto skillsTape = tapes.custom([] {
        // The levels of the five skills the guild teaches, character by character.
        std::vector<int> result;
        for (const Character &character : pParty->pCharacters)
            for (Skill skill : {SKILL_SWORD, SKILL_AXE, SKILL_SPEAR, SKILL_STAFF, SKILL_LEATHER})
                result.push_back(character.pActiveSkills[skill].level());
        return result;
    });
    auto goldTape = tapes.gold();
    auto soundsTape = tapes.sounds();
    startMM6NewGame(game);
    pParty->pCharacters[0]._achievedAwardsBits.set(kBladesEndMember); // Zoltan, a knight who knows Sword, Axe and Leather.
    pParty->SetGold(1000);
    test.startTaping();
    enterThroughDoor(game, kBladesEndDoor, 270);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();
    const std::vector<DrawnLine> refusal = panelLines(linesTape.back());
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    game.tick();
    const std::vector<DrawnLine> menu = panelLines(linesTape.back());
    clickHouseOption(game, localization->skillName(SKILL_SPEAR));
    const std::vector<DrawnLine> menuAfterSpear = panelLines(linesTape.back());
    clickHouseOption(game, localization->skillName(SKILL_STAFF));

    const Color white = colorTable.White;
    const Color yellow = colorTable.PaleCanary;
    EXPECT_EQ(refusal, std::vector<DrawnLine>({panelLine("You must be a", 175, yellow), panelLine("member of this", 191, yellow),
                                               panelLine("guild to study", 207, yellow), panelLine("here", 223, yellow)}));
    EXPECT_EQ(menu, std::vector<DrawnLine>({panelLine("Spear", 204, white), panelLine("Staff", 251, white), panelLine("Skill Cost: 375", 146, white)}));
    EXPECT_EQ(menuAfterSpear, std::vector<DrawnLine>({panelLine("Staff", 228, white), panelLine("Skill Cost: 375", 146, white)}));
    EXPECT_EQ(panelLines(linesTape.back()), std::vector<DrawnLine>({panelLine("Seek knowledge", 169, yellow), panelLine("elsewhere", 185, yellow),
                                                                    panelLine("Zoltan the", 201, yellow), panelLine("Knight", 217, yellow),
                                                                    panelLine(" ", 233, yellow), panelLine("I can offer you", 249, yellow),
                                                                    panelLine("nothing further.", 265, yellow)}));
    using Options = std::vector<std::pair<std::string, Recti>>;
    const Options both({{"Spear", Recti(480, 204, 140, 16)}, {"Staff", Recti(480, 251, 140, 16)}});
    EXPECT_EQ(optionsTape, tape(Options(), both, Options(), both, Options({{"Staff", Recti(480, 228, 140, 16)}}), Options()));
    EXPECT_EQ(skillsTape, tape(std::vector<int>{1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1},
                               std::vector<int>{1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1},
                               std::vector<int>{1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1}));
    EXPECT_EQ(goldTape, tape(1000, 625, 250));
    EXPECT_EQ(soundsTape.flatten(), tape(SOUND_gold01, static_cast<SoundId>(32902))); // The room's learn sound.
}

GAME_TEST(MM6Houses, ThievesGuildMenu) {
    // Buccaneers' Lair, a thieves' guild, showed its proprietor with an empty menu. Once it opens at 6 pm it lists a
    // member the skills it teaches that the member's class can learn and doesn't know yet, at a thieves' guild price.
    // Choosing one teaches it, and a second one the party can no longer pay for is refused.
    auto optionsTape = tapes.custom([] { return houseOptions(); });
    auto linesTape = tapes.titleLines();
    auto skillsTape = tapes.custom([] {
        // The levels of the five skills the guild teaches, character by character.
        std::vector<int> result;
        for (const Character &character : pParty->pCharacters)
            for (Skill skill : {SKILL_DAGGER, SKILL_MERCHANT, SKILL_ITEM_ID, SKILL_PERCEPTION, SKILL_TRAP_DISARM})
                result.push_back(character.pActiveSkills[skill].level());
        return result;
    });
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    auto soundsTape = tapes.sounds();
    startMM6NewGame(game);
    pParty->pCharacters[0]._achievedAwardsBits.set(kBuccaneersLairMember); // Zoltan, who knows Dagger.
    pParty->SetGold(200);
    waitUntilHour(game, 18);
    test.startTaping();
    enterThroughDoor(game, kBuccaneersLairDoor, 270);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    game.tick();
    const std::vector<DrawnLine> menu = panelLines(linesTape.back());
    clickHouseOption(game, localization->skillName(SKILL_MERCHANT));
    clickHouseOption(game, localization->skillName(SKILL_ITEM_ID));

    const Color white = colorTable.White;
    const Color yellow = colorTable.PaleCanary;
    EXPECT_EQ(menu, std::vector<DrawnLine>({panelLine("Merchant", 173, white), panelLine("Identify Item", 209, white), panelLine("Perception", 245, white),
                                            panelLine("Disarm Trap", 281, white), panelLine("Skill Cost: 150", 146, white)}));
    EXPECT_EQ(panelLines(linesTape.back()), std::vector<DrawnLine>({panelLine("Identify Item", 180, yellow), panelLine("Perception", 227, white),
                                                                    panelLine("Disarm Trap", 274, white), panelLine("Skill Cost: 135", 146, white)}));
    using Options = std::vector<std::pair<std::string, Recti>>;
    EXPECT_EQ(optionsTape, tape(Options(),
                                Options({{"Merchant", Recti(480, 173, 140, 16)}, {"Identify Item", Recti(480, 209, 140, 16)},
                                         {"Perception", Recti(480, 245, 140, 16)}, {"Disarm Trap", Recti(480, 281, 140, 16)}}),
                                Options({{"Identify Item", Recti(480, 180, 140, 16)}, {"Perception", Recti(480, 227, 140, 16)},
                                         {"Disarm Trap", Recti(480, 274, 140, 16)}})));
    EXPECT_EQ(skillsTape, tape(std::vector<int>{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               std::vector<int>{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
    EXPECT_EQ(goldTape, tape(200, 50)); // Zoltan's new Merchant skill takes the price down to 135.
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD));
    EXPECT_EQ(soundsTape.flatten(), tape(SOUND_gold01, static_cast<SoundId>(32602), static_cast<SoundId>(32604))); // The room's learn and no gold sounds.
}

GAME_TEST(MM6Houses, SelfGuildDoor) {
    // Space at the Initiate Guild of the Self's door, from the ground south of it, opens the guild. It lists a member
    // Buy Spells and the Self skills the member's class can learn and doesn't know yet.
    auto houseTape = tapes.house();
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    startMM6NewGame(game);
    pParty->pCharacters[1]._achievedAwardsBits.set(kSelfGuildMember);
    test.startTaping();
    enterThroughDoor(game, kSelfGuildDoor, 90);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(139)));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, std::vector<std::string>{"Buy Spells", "Spirit Magic", "Mind Magic"})); // The cleric knows Body Magic already.
}

GAME_TEST(MM6Houses, OptionFocusColor) {
    // House menus drew every line white. MM6 draws the line the mouse last pointed at, the first one until then, in a
    // focus colour: gold at a shop, pale yellow at a temple.
    auto linesTape = tapes.titleLines();
    startMM6NewGame(game);
    pParty->pCharacters[0].SetCondition(CONDITION_POISON_WEAK, 0); // So that the temple offers to heal.
    test.startTaping();
    enterThroughDoor(game, kWeaponShopDoor, 0);
    const std::vector<DrawnLine> shop = panelLines(linesTape.back());
    game.hoverGuiButton("HouseDialogue_Option2");
    game.tick();
    const std::vector<DrawnLine> shopHovered = panelLines(linesTape.back());
    game.moveMouse(240, 180); // Over the room, away from the menu.
    game.tick();
    const std::vector<DrawnLine> shopAway = panelLines(linesTape.back());
    game.pressGuiButton("House_Exit");
    game.tick();
    enterThroughDoor(game, kTempleDoor, 90);
    const std::vector<DrawnLine> temple = panelLines(linesTape.back());

    const Color white = colorTable.White;
    const Color gold = colorTable.Sunflower;
    EXPECT_EQ(shop, std::vector<DrawnLine>({panelLine("Buy Items", 149, gold), panelLine("Sell Items", 182, white), panelLine("Identify Items", 215, white),
                                            panelLine("Repair Items", 248, white), panelLine("Special Items", 281, white)}));
    EXPECT_EQ(shopHovered, std::vector<DrawnLine>({panelLine("Buy Items", 149, white), panelLine("Sell Items", 182, white), panelLine("Identify Items", 215, gold),
                                                   panelLine("Repair Items", 248, white), panelLine("Special Items", 281, white)}));
    EXPECT_EQ(shopAway, shopHovered);
    EXPECT_EQ(temple, std::vector<DrawnLine>({panelLine("Heal 10 Gold", 193, colorTable.PaleCanary), panelLine("Donate", 240, white)}));
}

GAME_TEST(MM6Houses, GuildMembership) {
    // Joining Blades' End through Harold Hess made the party members silently and left his membership topic up. He
    // tells what joining costs and offers to join, joining makes every character a member for 25 gold with the award
    // chime and reaction, and he drops the topic.
    auto goldTape = tapes.gold();
    auto membersTape = tapes.custom([] {
        std::vector<bool> result;
        for (const Character &character : pParty->pCharacters)
            result.push_back(character._achievedAwardsBits[kBladesEndMember]);
        return result;
    });
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto textTape = tapes.custom([] { return current_npc_text.substr(0, 20); }); // The start of what the NPC says.
    auto topicsTape = tapes.custom([] {
        const NPCData &harold = pNPCStats->pNPCData[33];
        return std::vector<unsigned>{harold.dialogue_1_evt_id, harold.dialogue_2_evt_id, harold.dialogue_3_evt_id};
    });
    auto soundsTape = tapes.sounds();
    auto portraitsTape = tapes.custom([] {
        std::vector<PortraitId> result;
        for (const Character &character : pParty->pCharacters)
            result.push_back(character.portrait);
        return result;
    });
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kHaroldHessDoor, 270);
    clickHouseOption(game, "Blade's End Membership");
    clickHouseOption(game, "Join Blades' End for 25 gold");

    EXPECT_EQ(goldTape, tape(200, 175));
    EXPECT_EQ(membersTape, tape(std::vector<bool>(4, false), std::vector<bool>(4, true)));
    using Lines = std::vector<std::string>;
    EXPECT_EQ(optionsTape, tape(Lines(), Lines{"Blade's End Membership"}, Lines{"Join Blades' End for 25 gold"}, Lines()));
    EXPECT_EQ(textTape, tape(std::string(), std::string("HEY!  The Blades' En"), std::string()));
    EXPECT_EQ(topicsTape, tape(std::vector<unsigned>{0, 0, 386}, std::vector<unsigned>{0, 0, 0}));
    EXPECT_EQ(std::ranges::count(soundsTape.flatten(), SOUND_quest), 4); // Each new member's award chime.
    EXPECT_EQ(portraitsTape.back(), std::vector<PortraitId>(4, PORTRAIT_WIDE_SMILE)); // The award reaction, the portraits idle before it.
}

GAME_TEST(MM6OffscreenFrame, DISABLED_HousesCapture) {
    // Hidden capture of New Sorpigal house screens: each house's first view, then the screens behind its options.
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePrefix = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePrefix, nullptr);
    ASSERT_TRUE(std::string_view(capturePrefix).starts_with("/private/tmp/"));
    auto capture = [&](std::string_view name) {
        RgbaImage frame;
        game.runGameRoutine([&frame] {
            render->ClearTarget(Color(0, 0, 0));
            engine->drawWorld();
            pMediaPlayer->HouseMovieLoop();
            engine->drawHUD();
            render->flushAndScale();
            engine->drawOverlay();
            frame = render->MakeFullScreenshot();
        });
        FileOutputStream output{Path(fmt::format("{}-{}.png", capturePrefix, name))};
        output.write(png::encode(frame));
        output.close();
    };

    auto captureHouse = [&](std::string_view name, int screens) {
        game.tick();
        capture(fmt::format("{}-0", name));
        if (current_screen_type != SCREEN_HOUSE)
            return;
        const bool listsResidents = std::ranges::any_of(window_SpeakInHouse->vButtons, [](GUIButton *button) { return button->id == "House_Npc0"; });
        if (listsResidents && name != "npchouse465" && name != "npchouse470") {
            game.pressGuiButton("House_Npc0");
            game.tick();
            capture(fmt::format("{}-p", name));
        }
        for (int option = 0; option < screens; ++option) {
            game.pressGuiButton(fmt::format("HouseDialogue_Option{}", option));
            game.tick();
            capture(fmt::format("{}-{}", name, option + 1));
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
        for (int i = 0; i < 3 && current_screen_type != SCREEN_GAME; ++i) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick(2);
        }
    };

    engine->config->debug.NoVideo.setValue(false);
    startMM6NewGame(game);
    for (auto [door, yaw, name, screens] : std::array<std::tuple<Vec3f, int, std::string_view, int>, 13>{{
             {kWeaponShopDoor, 0, "weapons", 5}, {kGeneralStoreDoor, 1536, "general", 2}, {kTempleDoor, 90, "temple", 0},
             {kTrainingDoor, 180, "training", 0}, {kGuildDoor, 1536, "guild", 0}, {kTavernDoor, 0, "tavern", 0},
             {kStableDoor, 180, "stable", 0}, {Vec3f(-8678, -6352, 300), 0, "armor", 5}, {Vec3f(-6376, -7606, 300), 90, "magic", 5},
             {kBladesEndDoor, 270, "merc", 0}, {Vec3f(-12528, -5814, 300), 90, "npchouse465", 0},
             {kBufordHouseDoor, 90, "npchouse470", 0}, {kSelfGuildDoor, 90, "self", 0}}}) {
        enterThroughDoor(game, door, yaw);
        captureHouse(name, screens);
    }

    game.teleportTo(engine->_currentLoadedMapId, kBankDoor, 180);
    game.pointMouseAtFace(kBankDoorFace);
    game.pressAndReleaseButton(BUTTON_LEFT);
    captureHouse("bank", 0);

    game.teleportTo(engine->_currentLoadedMapId, Vec3f(1800, -10224, 100), 0); // On the dock, facing the ship.
    game.pointMouseAtDecoration(477);
    game.pressAndReleaseButton(BUTTON_LEFT);
    captureHouse("boat", 0);

    pParty->pCharacters[2]._achievedAwardsBits.set(kElementalGuildMember);
    enterThroughDoor(game, kGuildDoor, 1536);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_3);
    captureHouse("guild-member", 1);

    pParty->pCharacters[0]._achievedAwardsBits.set(kBladesEndMember);
    enterThroughDoor(game, kBladesEndDoor, 270);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    captureHouse("merc-member", 0);

    waitUntilHour(game, 10); // The Town Hall opens at 10 am.
    enterThroughDoor(game, kTownHallDoor, 180);
    captureHouse("townhall", 0);

    waitUntilHour(game, 18); // The thieves' guild opens at 6 pm.
    pParty->pCharacters[0]._achievedAwardsBits.set(kBuccaneersLairMember);
    enterThroughDoor(game, kBuccaneersLairDoor, 270);
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
    captureHouse("thieves-member", 0);
}

GAME_TEST(MM6Houses, WeaponShopIdentify) {
    // The New Sorpigal weapon shop identifies a weapon from the active character's backpack for gold.
    auto goldTape = tapes.gold();
    auto identifiedTape = tapes.custom([] {
        for (InventoryConstEntry entry : pParty->pCharacters[0].inventory.entries())
            if (entry->itemId == static_cast<ItemId>(1))
                return entry->IsIdentified();
        return false;
    });
    auto statusTape = tapes.statusBar();
    startMM6NewGame(game);
    for (InventoryEntry entry : pParty->pCharacters[0].inventory.entries())
        if (entry->itemId == static_cast<ItemId>(1))
            entry->flags &= ~ITEM_IDENTIFIED;
    test.startTaping();
    enterThroughDoor(game, kWeaponShopDoor, 0);
    clickHouseOption(game, localization->str(static_cast<LstrId>(541))); // Identify Items.
    InventoryEntry sword;
    for (InventoryEntry entry : pParty->pCharacters[0].inventory.entries())
        if (entry->itemId == static_cast<ItemId>(1))
            sword = entry;
    ASSERT_TRUE(sword);
    game.pressAndReleaseButton(BUTTON_LEFT, mapFromInventoryGrid(sword.geometry().topLeft(), Pointi(14, 17)) + Pointi(16, 16));
    game.tick();
    EXPECT_EQ(goldTape, tape(200, 125));
    EXPECT_EQ(identifiedTape, tape(false, true));
    EXPECT_CONTAINS(statusTape, localization->str(static_cast<LstrId>(569))); // "Done!"
}

GAME_TEST(MM6Houses, WeaponShopRepair) {
    // The New Sorpigal weapon shop repairs a broken weapon from the active character's backpack for gold.
    auto goldTape = tapes.gold();
    auto brokenTape = tapes.custom([] {
        for (InventoryConstEntry entry : pParty->pCharacters[0].inventory.entries())
            if (entry->itemId == static_cast<ItemId>(1))
                return entry->IsBroken();
        return false;
    });
    auto statusTape = tapes.statusBar();
    startMM6NewGame(game);
    for (InventoryEntry entry : pParty->pCharacters[0].inventory.entries())
        if (entry->itemId == static_cast<ItemId>(1))
            entry->flags |= ITEM_BROKEN;
    test.startTaping();
    enterThroughDoor(game, kWeaponShopDoor, 0);
    clickHouseOption(game, localization->str(static_cast<LstrId>(542))); // Repair Items.
    InventoryEntry sword;
    for (InventoryEntry entry : pParty->pCharacters[0].inventory.entries())
        if (entry->itemId == static_cast<ItemId>(1))
            sword = entry;
    ASSERT_TRUE(sword);
    game.pressAndReleaseButton(BUTTON_LEFT, mapFromInventoryGrid(sword.geometry().topLeft(), Pointi(14, 17)) + Pointi(16, 16));
    game.tick();
    EXPECT_EQ(goldTape, tape(200, 189));
    EXPECT_EQ(brokenTape, tape(true, false));
    EXPECT_CONTAINS(statusTape, localization->str(static_cast<LstrId>(570))); // "Good as New!"
}

GAME_TEST(MM6Houses, WeaponShopIdentifyUnconscious) {
    // An unconscious character had a weapon identified at the New Sorpigal weapon shop. The shop turns him away with
    // the no condition line and the error sound, and a conscious one still pays and gets it identified.
    for (bool conscious : {false, true}) {
        SCOPED_TRACE(fmt::format("conscious={}", conscious));
        test.prepareForNextTest();
        auto goldTape = tapes.gold();
        auto identifiedTape = tapes.custom([] { return knightSword() && knightSword()->IsIdentified(); });
        auto unconsciousTape = tapes.custom([] { return pParty->pCharacters[0].conditions.has(CONDITION_UNCONSCIOUS); });
        auto activeTape = tapes.activeCharacterIndex();
        auto statusTape = tapes.statusBar();
        auto soundsTape = tapes.sounds();
        auto linesTape = tapes.titleLines();
        startMM6NewGame(game);
        knightSword()->flags &= ~ITEM_IDENTIFIED;
        if (!conscious)
            knockOut(&pParty->pCharacters[0]);
        test.startTaping();
        enterThroughDoor(game, kWeaponShopDoor, 0);
        clickHouseOption(game, localization->str(static_cast<LstrId>(541))); // Identify Items.
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
        game.tick();
        clickKnightSword(game);

        EXPECT_EQ(unconsciousTape, tape(!conscious));
        EXPECT_EQ(activeTape.back(), 0);
        EXPECT_EQ(goldTape, conscious ? tape(200, 125) : tape(200));
        EXPECT_EQ(identifiedTape, conscious ? tape(false, true) : tape(false));
        if (conscious) {
            EXPECT_CONTAINS(statusTape, localization->str(static_cast<LstrId>(569))); // "Done!"
            EXPECT_MISSES(soundsTape.flatten(), SOUND_error);
            EXPECT_EQ(panelLines(linesTape.back()), std::vector<DrawnLine>());
        } else {
            EXPECT_MISSES(statusTape, localization->str(static_cast<LstrId>(569)));
            EXPECT_CONTAINS(soundsTape.flatten(), SOUND_error);
            EXPECT_EQ(panelLines(linesTape.back()), noConditionLines());
        }
    }
}

GAME_TEST(MM6Houses, WeaponShopRepairUnconscious) {
    // An unconscious character had a broken weapon repaired at the New Sorpigal weapon shop. The shop turns him away
    // with the no condition line and the error sound, and a conscious one still pays and gets it repaired.
    for (bool conscious : {false, true}) {
        SCOPED_TRACE(fmt::format("conscious={}", conscious));
        test.prepareForNextTest();
        auto goldTape = tapes.gold();
        auto brokenTape = tapes.custom([] { return knightSword() && knightSword()->IsBroken(); });
        auto unconsciousTape = tapes.custom([] { return pParty->pCharacters[0].conditions.has(CONDITION_UNCONSCIOUS); });
        auto activeTape = tapes.activeCharacterIndex();
        auto statusTape = tapes.statusBar();
        auto soundsTape = tapes.sounds();
        auto linesTape = tapes.titleLines();
        startMM6NewGame(game);
        knightSword()->flags |= ITEM_BROKEN;
        if (!conscious)
            knockOut(&pParty->pCharacters[0]);
        test.startTaping();
        enterThroughDoor(game, kWeaponShopDoor, 0);
        clickHouseOption(game, localization->str(static_cast<LstrId>(542))); // Repair Items.
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
        game.tick();
        clickKnightSword(game);

        EXPECT_EQ(unconsciousTape, tape(!conscious));
        EXPECT_EQ(activeTape.back(), 0);
        EXPECT_EQ(goldTape, conscious ? tape(200, 189) : tape(200));
        EXPECT_EQ(brokenTape, conscious ? tape(true, false) : tape(true));
        if (conscious) {
            EXPECT_CONTAINS(statusTape, localization->str(static_cast<LstrId>(570))); // "Good as New!"
            EXPECT_MISSES(soundsTape.flatten(), SOUND_error);
            EXPECT_EQ(panelLines(linesTape.back()), std::vector<DrawnLine>());
        } else {
            EXPECT_MISSES(statusTape, localization->str(static_cast<LstrId>(570)));
            EXPECT_CONTAINS(soundsTape.flatten(), SOUND_error);
            EXPECT_EQ(panelLines(linesTape.back()), noConditionLines());
        }
    }
}

GAME_TEST(MM6Houses, WeaponShopMenuUnconscious) {
    // The New Sorpigal weapon shop listed its services to an unconscious character, and Identify Items opened. The shop
    // hides its menu behind the no condition line until a conscious character is selected.
    for (bool conscious : {false, true}) {
        SCOPED_TRACE(fmt::format("conscious={}", conscious));
        test.prepareForNextTest();
        auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
        auto activeTape = tapes.activeCharacterIndex();
        auto linesTape = tapes.titleLines();
        auto screenTape = tapes.custom([] {
            GUIWindow_MM6Shop *shop = dynamic_cast<GUIWindow_MM6Shop *>(window_SpeakInHouse.get());
            return shop ? shop->screen() : std::nullopt;
        });
        startMM6NewGame(game);
        if (!conscious)
            knockOut(&pParty->pCharacters[0]);
        test.startTaping();
        enterThroughDoor(game, kWeaponShopDoor, 0);
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
        game.tick();
        std::optional<Pointi> identify;
        for (const auto &[label, rect] : houseOptions())
            if (label == localization->str(static_cast<LstrId>(541))) // Identify Items.
                identify = rect.center();
        ASSERT_TRUE(identify);
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
        game.tick();
        const std::vector<DrawnLine> zoltanPanel = panelLines(linesTape.back());
        game.pressAndReleaseButton(BUTTON_LEFT, *identify);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
        game.tick();

        const std::vector<std::string> menu = {"Buy Items", "Sell Items", "Identify Items", "Repair Items", "Special Items"};
        using Labels = std::vector<std::string>;
        if (conscious) {
            EXPECT_EQ(optionsTape, tape(Labels(), menu, Labels()));
            EXPECT_EQ(screenTape, tape(std::optional<mm6::HouseServiceOption>(), std::optional(mm6::HouseServiceOption::IdentifyItems)));
            EXPECT_EQ(zoltanPanel.size(), menu.size());
        } else {
            EXPECT_EQ(optionsTape, tape(Labels(), menu, Labels(), menu));
            EXPECT_EQ(screenTape, tape(std::optional<mm6::HouseServiceOption>()));
            EXPECT_EQ(zoltanPanel, noConditionLines());
        }
        EXPECT_EQ(activeTape, conscious ? tape(0, 1, 0, 1) : tape(1, 0, 1)); // An unconscious Zoltan loses the focus on the street.
    }
}

GAME_TEST(MM6Houses, TrainingUnconscious) {
    // An unconscious character trained to the next level at the New Sorpigal Training Grounds, and came out with full
    // hit points. The trainer hides the offer behind the no condition line.
    for (bool conscious : {false, true}) {
        SCOPED_TRACE(fmt::format("conscious={}", conscious));
        test.prepareForNextTest();
        auto levelsTape = charTapes.levels();
        auto hpTape = charTapes.hps();
        auto goldTape = tapes.gold();
        auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
        auto linesTape = tapes.titleLines();
        startMM6NewGame(game);
        pParty->pCharacters[0].experience = 1000;
        pParty->pCharacters[1].experience = 1000;
        if (!conscious)
            knockOut(&pParty->pCharacters[0]);
        test.startTaping();
        enterThroughDoor(game, kTrainingDoor, 180);
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
        game.tick();
        ASSERT_EQ(houseOptions().size(), 1);
        const Pointi train = houseOptions().front().second.center();
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
        game.tick();
        game.pressAndReleaseButton(BUTTON_LEFT, train);
        game.tick();

        using Labels = std::vector<std::string>;
        const Labels offer = {localization->format(LSTR_TRAIN_TO_LEVEL_D_FOR_D_GOLD, 2, 10)};
        const Labels needMore = {localization->format(LSTR_YOU_NEED_D_MORE_EXPERIENCE_TO_TRAIN_TO, 2000, 3)};
        if (conscious) {
            EXPECT_EQ(levelsTape, tape({1, 1, 1, 1}, {2, 1, 1, 1}));
            EXPECT_EQ(goldTape, tape(200, 190));
            EXPECT_EQ(optionsTape.back(), needMore);
        } else {
            EXPECT_EQ(levelsTape, tape({1, 1, 1, 1}));
            EXPECT_EQ(hpTape.size(), 1);
            EXPECT_EQ(goldTape, tape(200));
            EXPECT_EQ(optionsTape.back(), Labels());
            EXPECT_EQ(panelLines(linesTape.back()), noConditionLines());
        }
    }
}

GAME_TEST(MM6Houses, BankDepositAndWithdraw) {
    // The New Sorpigal bank answered Deposit and Withdraw with a not available line instead of moving gold.
    auto goldTape = tapes.gold();
    auto bankTape = tapes.custom([] { return pParty->GetBankGold(); });
    auto soundsTape = tapes.sounds();
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    startMM6NewGame(game);
    test.startTaping();
    game.teleportTo(engine->_currentLoadedMapId, kBankDoor, 180);
    game.pointMouseAtFace(kBankDoorFace);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    clickHouseOption(game, localization->str(LSTR_DEPOSIT));
    typeAmount(game, "150");
    clickHouseOption(game, localization->str(LSTR_WITHDRAW));
    typeAmount(game, "999"); // More than the bank holds.
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE));
    EXPECT_EQ(goldTape, tape(200, 50, 200));
    EXPECT_EQ(bankTape, tape(0, 150, 0));
    EXPECT_MISSES(statusTape, "This service is not available yet.");
    EXPECT_CONTAINS(soundsTape.flatten(), static_cast<SoundId>(31502)); // The banker when the party asks for more than the bank holds.
}

GAME_TEST(MM6Houses, TavernRentRoom) {
    // A Lonely Knight's room said "not available yet". Renting it takes the price, leaves the house and sleeps the
    // party until 6 am the next day, which restores HP and SP without eating food.
    auto goldTape = tapes.gold();
    auto foodTape = tapes.food();
    auto screenTape = tapes.screen();
    auto clockTape = tapes.custom([] {
        const CivilTime time = pParty->GetPlayingTime().toCivilTime();
        return std::tuple(time.day, time.hour, time.minute);
    });
    auto hpTape = charTapes.hp(0);
    auto spTape = charTapes.mp(2);
    auto statusTape = tapes.statusBar();
    auto soundsTape = tapes.sounds();
    startMM6NewGame(game);
    pParty->pCharacters[0].health = 1;
    pParty->pCharacters[2].mana = 0;
    test.startTaping();
    enterThroughDoor(game, kTavernDoor, 0);
    game.pressGuiButton("House_Npc0"); // The proprietor.
    game.tick();
    clickHouseOption(game, localization->format(LSTR_RENT_ROOM_FOR_D_GOLD, 3));
    for (int frame = 0; frame < 1000 && current_screen_type != SCREEN_GAME; ++frame)
        game.tick(); // The party sleeps through the night.

    EXPECT_EQ(goldTape, tape(200, 197));
    EXPECT_EQ(foodTape, tape(7));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE, SCREEN_REST, SCREEN_GAME));
    EXPECT_EQ(clockTape.front(), std::tuple(1, 9, 0));
    EXPECT_EQ(clockTape.back(), std::tuple(2, 6, 0));
    EXPECT_EQ(hpTape, tape(1, pParty->pCharacters[0].GetMaxHealth()));
    EXPECT_EQ(spTape, tape(0, pParty->pCharacters[2].GetMaxMana()));
    EXPECT_MISSES(statusTape, "This service is not available yet.");
    EXPECT_CONTAINS(soundsTape.flatten(), static_cast<SoundId>(32002)); // The tavern keeper renting the room.
}

GAME_TEST(MM6Houses, TavernFillPacks) {
    // A Lonely Knight's packs said "not available yet". They fill up to the tavern's six days for the price, and packs
    // that already hold six days are refused with the packs full line.
    auto goldTape = tapes.gold();
    auto foodTape = tapes.food();
    auto statusTape = tapes.statusBar();
    auto portraitTape = charTapes.portrait(0);
    auto soundsTape = tapes.sounds();
    startMM6NewGame(game);
    pParty->SetFood(2);
    test.startTaping();
    enterThroughDoor(game, kTavernDoor, 0);
    game.pressGuiButton("House_Npc0"); // The proprietor.
    game.tick();
    const std::string fill = localization->format(LSTR_FILL_PACKS_TO_D_DAYS_FOR_D_GOLD, 6, 2);
    clickHouseOption(game, fill);
    clickHouseOption(game, fill);

    EXPECT_EQ(goldTape, tape(200, 198));
    EXPECT_EQ(foodTape, tape(2, 6));
    EXPECT_EQ(std::ranges::count(statusTape, localization->str(LSTR_YOUR_PACKS_ARE_ALREADY_FULL)), 1);
    EXPECT_MISSES(statusTape, "This service is not available yet.");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_NO)); // The packs full reaction.
    EXPECT_CONTAINS(soundsTape.flatten(), static_cast<SoundId>(32003)); // The tavern keeper selling food.
}

GAME_TEST(MM6Houses, ExpertTeacher) {
    // Isao Magistrus' Fire Magic Expertise topic did nothing, so no character could become an expert. His offer turns
    // away a character who doesn't know the skill, and makes one with rank 4 an expert for 1000 gold.
    auto goldTape = tapes.gold();
    auto fireTape = tapes.custom([] {
        const CombinedSkillValue fire = pParty->pCharacters[2].pActiveSkills[SKILL_FIRE];
        return std::pair(fire.level(), fire.mastery());
    });
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto portraitTape = charTapes.portrait(2);
    auto houseTape = tapes.house();
    startMM6NewGame(game);
    pParty->SetGold(2000);
    pParty->pCharacters[2].pActiveSkills[SKILL_FIRE] = CombinedSkillValue(4, MASTERY_NOVICE); // Serena, a sorcerer.
    test.startTaping();
    enterThroughDoor(game, kIsaoHouseDoor, 90);
    clickHouseOption(game, "Fire Magic Expertise");
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_3);
    game.tick();
    clickHouseOption(game, "Become Expert in Fire Magic for 1000 gold");

    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(465)));
    EXPECT_EQ(goldTape, tape(2000, 1000));
    EXPECT_EQ(fireTape, tape(std::pair(4, MASTERY_NOVICE), std::pair(4, MASTERY_EXPERT)));
    EXPECT_EQ(textTape, tape(std::string(), std::string(pNPCTopics[223].pText), std::string())); // The offer, text 224 of npctext.txt.
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, std::vector<std::string>{"Fire Magic Expertise", "Jezebel"},
                                std::vector<std::string>{pNPCTopics[261].pText}, // "You must know the skill before you can become an expert in it!"
                                std::vector<std::string>{"Become Expert in Fire Magic for 1000 gold"},
                                std::vector<std::string>{"Fire Magic Expertise", "Jezebel"}));
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_SMILE));
}

GAME_TEST(MM6Houses, MasterTeacher) {
    // Jezebel's Master of Fire topic on the Misty Islands did nothing, so no character could become a master. Her offer
    // turns away a character who doesn't know Fire Magic and an expert of rank 11, and makes an expert of rank 12 a
    // master for 4000 gold.
    auto goldTape = tapes.gold();
    auto fireTape = tapes.custom([] {
        const CombinedSkillValue fire = pParty->pCharacters[2].pActiveSkills[SKILL_FIRE];
        return std::pair(fire.level(), fire.mastery());
    });
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto portraitTape = charTapes.portrait(2);
    auto houseTape = tapes.house();
    startMM6NewGame(game);
    pParty->SetGold(5000);
    pParty->pCharacters[2].pActiveSkills[SKILL_FIRE] = CombinedSkillValue(12, MASTERY_EXPERT); // Serena, a sorcerer.
    pParty->pCharacters[3].pActiveSkills[SKILL_FIRE] = CombinedSkillValue(11, MASTERY_EXPERT);
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.teleportTo(pMapTable->GetMapInfo("oute2.odm"), kJezebelHouseDoor, 270);
    game.runGameRoutine([&] { chestOptIn.reset(); });
    test.startTaping();
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    clickHouseOption(game, "Master of Fire "); // npctopic.txt ends the topic with a space.
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_4);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_3);
    game.tick();
    clickHouseOption(game, "Become Master in Fire Magic for 4000 gold");

    EXPECT_EQ(houseTape, tape(static_cast<HouseId>(455)));
    EXPECT_EQ(goldTape, tape(5000, 1000));
    EXPECT_EQ(fireTape, tape(std::pair(12, MASTERY_EXPERT), std::pair(12, MASTERY_MASTER)));
    EXPECT_EQ(textTape, tape(std::string(), std::string(pNPCTopics[224].pText), std::string())); // The offer, text 225 of npctext.txt.
    using Lines = std::vector<std::string>;
    EXPECT_EQ(optionsTape, tape(Lines{"Master of Fire ", "Fire Guild Membership"}, Lines{pNPCTopics[261].pText}, // "You must know the skill before you can become an expert in it!"
                                Lines{pNPCTopics[260].pText}, // "You don't meet the requirements, and cannot be taught until you do."
                                Lines{"Become Master in Fire Magic for 4000 gold"}, Lines{"Master of Fire ", "Fire Guild Membership"}));
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_WIDE_SMILE)); // The mastery reaction.
}

static void talkTo(EngineController &game, std::string_view name) {
    // Clicks the portrait of the person in the house with this name.
    ASSERT_NE(window_SpeakInHouse, nullptr);
    std::optional<Pointi> center;
    for (GUIButton *button : window_SpeakInHouse->vButtons)
        if (button->id.starts_with("House_Npc") && button->label.contains(name))
            center = button->rect.center();
    ASSERT_TRUE(center) << "No one called " << name;
    game.pressAndReleaseButton(BUTTON_LEFT, *center);
    game.tick();
}

GAME_TEST(MM6Houses, BountyHunt) {
    // Janice's Bounty Hunt topic in the New Sorpigal Town Hall did nothing. She names this month's monster, and once
    // the party has killed one she pays 100 gold per level of the monster, every character gets the bounties
    // collected award with its chime, and the reputation grows by the level. Asked again that month, she says the
    // bounty is claimed.
    auto goldTape = tapes.gold();
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto bountyTape = tapes.custom([] { return std::pair(pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL], pParty->monster_for_hunting_killed[HOUSE_FIRST_TOWN_HALL]); });
    auto awardsTape = charTapes.custom([](const Character &character) { return character._achievedAwardsBits[static_cast<AwardId>(81)]; });
    auto soundsTape = tapes.sounds();
    startMM6NewGame(game);
    waitUntilHour(game, 10); // The Town Hall opens at 10 am.
    test.startTaping();
    enterThroughDoor(game, kTownHallDoor, 180);
    talkTo(game, "Janice");
    clickHouseOption(game, "Bounty Hunt");
    for (int i = 0; i < 3 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    game.teleportTo(engine->_currentLoadedMapId, kTownHallDoor, 0); // Back to the street, facing east.
    ASSERT_NE(pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL], MONSTER_INVALID);
    Actor *target = game.spawnMonster(kTownHallDoor + Vec3f(200, 0, 0), pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL], SPAWN_DUMMY);
    target->hp = 1; // One hit kills it.
    game.pointMouseAtActor(target->id);
    for (int attack = 0; attack < 1000 && target->aiState != Dead; ++attack) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick(); // A door opens only for a character who has recovered from the fight.
    enterThroughDoor(game, kTownHallDoor, 180);
    talkTo(game, "Janice");
    clickHouseOption(game, "Bounty Hunt");
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    clickHouseOption(game, "Bounty Hunt");

    const MonsterId ooze = static_cast<MonsterId>(116);
    const std::string name = fmt::format("{::}Acidic Ooze{::}", colorTable.PaleCanary.tag(), colorTable.White.tag());
    EXPECT_EQ(bountyTape, tape(std::pair(MONSTER_INVALID, false), std::pair(ooze, false), std::pair(ooze, true), std::pair(MONSTER_INVALID, false)));
    EXPECT_EQ(textTape, tape(std::string(), "This month's bounty is on a " + name + ".  Kill it and return before the end of the month to collect the 1800 gold reward.",
                             std::string(), "Congratulations on defeating the " + name + "!  Here is the 1800 gold reward.  Come back next month for a new bounty.",
                             std::string(), "Someone has already claimed the bounty this month.  Come back next month for a new bounty."));
    using Lines = std::vector<std::string>;
    const Lines topics = {"Goblinwatch", "Bounty Hunt"};
    EXPECT_EQ(optionsTape, tape(Lines(), topics, Lines(), topics, Lines(), topics, Lines(), topics, Lines())); // The bounty takes the place of her topics, escape brings them back.
    EXPECT_EQ(goldTape, tape(200, 2000)); // A level 18 monster.
    EXPECT_EQ(reputationTape, tape(0, 18));
    EXPECT_EQ(awardsTape, tape({false, false, false, false}, {true, true, true, true}));
    EXPECT_EQ(std::ranges::count(soundsTape.flatten(), SOUND_quest), 4); // Each character's award chime.
}

GAME_TEST(MM6Houses, TempleDonate) {
    // New Sorpigal Temple's Donate said "not available yet". A donation takes the temple's 10 gold, thanks the party
    // and raises its reputation by 200, but not past 200.
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto statusTape = tapes.statusBar();
    auto portraitTape = charTapes.portrait(0);
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kTempleDoor, 90);
    clickHouseOption(game, localization->str(LSTR_DONATE));
    clickHouseOption(game, localization->str(LSTR_DONATE));

    EXPECT_EQ(goldTape, tape(200, 190, 180));
    EXPECT_EQ(reputationTape, tape(0, 200));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_THANK_YOU));
    EXPECT_MISSES(statusTape, "This service is not available yet.");
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_PURSE_LIPS_3)); // The donation reaction.
}

GAME_TEST(MM6Houses, TavernDrink) {
    // A Lonely Knight's Have a Drink said "not available yet". A drink costs 1 gold, hiccups on the status line and
    // plays the tavern keeper's sound, and by chance the drinker gets drunk or a stat bonus. These four get the
    // character drunk.
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    auto soundsTape = tapes.sounds();
    auto portraitTape = charTapes.portrait(0);
    auto drunkTape = charTapes.custom(0, [](const Character &character) { return character.conditions.has(CONDITION_DRUNK); });
    auto bonusesTape = charTapes.custom(0, [](const Character &character) {
        std::vector<int> result;
        for (Attribute stat : allStatAttributes())
            result.push_back(character._statBonuses[stat]);
        return result;
    });
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kTavernDoor, 0);
    game.pressGuiButton("House_Npc0"); // The proprietor.
    game.tick();
    for (int drink = 0; drink < 4; ++drink)
        clickHouseOption(game, localization->str(LSTR_HAVE_A_DRINK));

    EXPECT_EQ(goldTape, tape(200, 199, 198, 197, 196));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_HIC));
    EXPECT_CONTAINS(soundsTape.flatten(), static_cast<SoundId>(32003)); // The tavern keeper serving.
    EXPECT_EQ(drunkTape, tape(false, true));
    EXPECT_EQ(portraitTape, tape(PORTRAIT_NORMAL, PORTRAIT_DRUNK));
    EXPECT_EQ(bonusesTape.size(), 1); // These drinks rolled no stat bonus.
}

GAME_TEST(MM6Houses, TavernTip) {
    // A Lonely Knight's Tip the Bartender said "not available yet". The barkeep wants the party to have a drink first.
    // After one, a tip of 1 gold gets a piece of New Sorpigal's news, and the next tip gets the same news again. The
    // drinker gets drunk, so a sober character tips.
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto drawnTape = tapes.allGUIWindowsText();
    auto portraitTape = charTapes.portrait(1);
    auto tipsTape = tapes.custom([] { return pParty->mm6TavernTips[0]; });
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kTavernDoor, 0);
    game.pressGuiButton("House_Npc0"); // The proprietor.
    game.tick();
    const std::string tip = localization->str(mm6::houseServiceOptionLabel(mm6::HouseServiceOption::TipBarkeep));
    clickHouseOption(game, tip);
    clickHouseOption(game, localization->str(LSTR_HAVE_A_DRINK));
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();
    clickHouseOption(game, tip);
    clickHouseOption(game, tip);

    const std::string news = pNPCStats->pCatchPhrases[30]; // The last of New Sorpigal's news.
    EXPECT_EQ(goldTape, tape(200, 199, 198, 197));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_HAVE_A_DRINK_FIRST));
    EXPECT_MISSES(statusTape, "This service is not available yet.");
    EXPECT_EQ(textTape, tape(std::string(), news));
    EXPECT_EQ(tipsTape, tape(0, 29));
    EXPECT_CONTAINS(portraitTape, PORTRAIT_45); // The tip reaction.
    EXPECT_CONTAINS(drawnTape.flatten(), "Berserker's Fury in Castle Ironfist can teach you\nbow, shield, chain, plate, and repair item.");
}
