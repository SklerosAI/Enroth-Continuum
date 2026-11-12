#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ShopStockIdentity.h"
#include "Engine/MM6/MM6ShopTitleState.h"
#include "Engine/Localization.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/HouseTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/ItemGrid.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/Houses/MM6Shop.h"

#include "Utility/Memory/Blob.h"

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

static const Vec3f kTempleDoor(-9744, -1862, 700); // New Sorpigal Temple, facing north.
static const Vec3f kWeaponShopDoor(-8422, -7392, 300); // Weapon shop, facing east.
static const Vec3f kTownHallDoor(-11090, -6536, 300); // Town Hall, facing west.
static const Vec3f kGeneralStoreDoor(-10333, -8532, 300); // General store, facing south.
static const Vec3f kGuildDoor(-5384, -1276, 700); // Elemental guild.
static const Vec3f kTrainingDoor(-13874, -7032, 300); // Training Grounds, facing west.
static const Vec3f kTavernDoor(-8784, -8690, 300); // A Lonely Knight, facing east.

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

GAME_TEST(MM6Houses, UnimplementedServiceRefuses) {
    // A service the MM6 house screen doesn't implement is listed, and choosing it refuses without touching the party.
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto goldTape = tapes.gold();
    auto itemsTape = tapes.totalItemCount();
    startMM6NewGame(game);
    test.startTaping();
    enterThroughDoor(game, kGuildDoor, 1536);
    game.pressGuiButton("HouseDialogue_Option1"); // Learn.
    game.tick();
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE));
    EXPECT_CONTAINS(statusTape, "This service is not available yet.");
    EXPECT_EQ(goldTape.size(), 1);
    EXPECT_EQ(itemsTape.size(), 1);
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
    EXPECT_EQ(questBitsTape, tape(std::vector<int>{1, 2, 3, 4, 5, 6}, std::vector<int>{1, 2, 3, 4, 5, 6, 138}));
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

GAME_TEST(MM6Houses, GuildBuySpellbook) {
    // A New Sorpigal spell guild sells a spellbook from its generated stock for gold.
    auto goldTape = tapes.gold();
    auto stockTape = tapes.custom([] { return guildStock(137); });
    auto backpackTape = tapes.custom([] { return backpackItems(0); });
    startMM6NewGame(game);
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
