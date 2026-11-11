#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM6CampaignChain.h"

static std::vector<int> councilBits() {
    // The set ones of Andover Potbello's pay, Wilbur Humphrey's shield quest, Loretta Fleise's price fixing (taken on,
    // all done and one per stable), and Kilburn's shield found.
    std::vector<int> result;
    for (int bit : {82, 86, 116, 117, 147, 148, 149, 150, 151, 152, 153, 154, 155, 182, 304})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static std::vector<int> councilAwards() {
    // The six council votes, then the letter delivered to Wilbur Humphrey.
    std::vector<int> result;
    for (int award : {2, 3, 4, 5, 6, 7, 58})
        if (pParty->pCharacters[0]._achievedAwardsBits[static_cast<AwardId>(award)])
            result.push_back(award);
    return result;
}

static void talkInCastle(EngineController &game, MapId map, int doorEvent, int npcId, std::initializer_list<int> topics) {
    enterMM6CampaignChainCastle(game, map, doorEvent);
    talkToMM6CampaignChainNpc(game, npcId);
    for (int topic : topics)
        ASSERT_TRUE(selectMM6CampaignChainTopic(game, topic));
    leaveMM6CampaignChainHouse(game);
}

static void fixStablePrices(EngineController &game, MapId map, int doorEvent) {
    // Space at the stable door, then the price fixing line that Loretta Fleise's quest adds under the routes.
    useMM6CampaignChainEventFace(game, map, doorEvent);
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    auto option = std::ranges::find_if(window_SpeakInHouse->vButtons, [](GUIButton *button) {
        return button->id.starts_with("HouseDialogue_Option") && button->label == pNPCTopics[99].pTopic;
    });
    ASSERT_NE(option, window_SpeakInHouse->vButtons.end());
    game.pressGuiButton((*option)->id);
    game.tick();
    leaveMM6CampaignChainHouse(game);
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter1) {
    // Fixtures: party strength, level 100, might, accuracy, speed and endurance 200, Disarm Trap 30 at master and full
    // health, kept by the later saves. Teleports to Castle Fleise, to the stables of outd1, outc1, outb1, outb2, outc2,
    // outc3 and oute3, to Kilburn's chest on outb2 and back to Castle Ironfist, plus doors on the current map.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 0);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId outd1 = pMapTable->GetMapInfo("outd1.odm");
    const MapId outc1 = pMapTable->GetMapInfo("outc1.odm");
    const MapId outb1 = pMapTable->GetMapInfo("outb1.odm");
    const MapId outb2 = pMapTable->GetMapInfo("outb2.odm");
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId outc3 = pMapTable->GetMapInfo("outc3.odm");
    ASSERT_EQ(engine->_currentLoadedMapId, oute3);
    ASSERT_TRUE(pParty->hasItem(static_cast<ItemId>(505)));
    ASSERT_EQ(councilAwards(), std::vector<int>());
    for (Character &character : pParty->pCharacters) {
        character.uLevel = 100;
        for (Attribute attribute : {ATTRIBUTE_MIGHT, ATTRIBUTE_ACCURACY, ATTRIBUTE_SPEED, ATTRIBUTE_ENDURANCE})
            character._stats[attribute] = 200;
        character.setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(30, MASTERY_MASTER));
        character.health = character.GetMaxHealth();
    }

    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return councilBits(); });
    auto awardsTape = tapes.custom([] { return councilAwards(); });
    auto letterTape = tapes.hasItem(static_cast<ItemId>(505));
    auto shieldTape = tapes.hasItem(static_cast<ItemId>(499));
    auto goldTape = tapes.gold();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    game.teleportTo(oute3, Vec3f(-8784, -8690, 300), 0); // A Lonely Knight, facing its door.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    talkToMM6CampaignChainNpc(game, 1); // Andover Potbello.
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 1));
    leaveMM6CampaignChainHouse(game);

    game.teleportTo(oute3, Vec3f(-10346, -9472, 161), 180); // The New Sorpigal stable, facing its door.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("HouseDialogue_Option0"); // The coach to Castle Ironfist.
    game.tick(17); // The coach leaves a second and a half after the party pays, then the house closes.
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, outd3);
    talkInCastle(game, outd3, 43, 4, {9, 10}); // Wilbur Humphrey takes the letter and asks for Kilburn's shield.

    talkInCastle(game, outd1, 34, 14, {79}); // Loretta Fleise wants the stables to raise their prices.
    fixStablePrices(game, outd1, 10);
    fixStablePrices(game, outc1, 31);
    fixStablePrices(game, outb1, 8);
    fixStablePrices(game, outb2, 10);

    useMM6CampaignChainEventFace(game, outb2, 68); // Kilburn's chest.
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    InventoryEntry shield = vChests[1].inventory.find(static_cast<ItemId>(499));
    ASSERT_TRUE(shield);
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[1].chestTypeId].inventoryOffset + shield.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton("Game_Character1"); // The held shield goes into the first character's backpack.
    game.tick();

    fixStablePrices(game, outc2, 14);
    fixStablePrices(game, outc2, 16);
    fixStablePrices(game, outc3, 8);
    fixStablePrices(game, outd3, 8);
    talkInCastle(game, outd3, 43, 4, {11}); // Wilbur Humphrey takes the shield.
    fixStablePrices(game, oute3, 15);
    talkInCastle(game, outd1, 34, 14, {80}); // Loretta Fleise hears that every stable has raised its prices.

    EXPECT_EQ(mapTape, tape(oute3, outd3, outd1, outc1, outb1, outb2, outc2, outc3, outd3, oute3, outd1));
    EXPECT_EQ(bitsTape.front(), std::vector<int>());
    EXPECT_CONTAINS(bitsTape, std::vector<int>({82})); // Andover Potbello paid, Wilbur Humphrey clears it.
    EXPECT_CONTAINS(bitsTape, std::vector<int>({86, 116, 152, 153, 154, 155, 182, 304})); // Four stables, then the shield.
    EXPECT_EQ(bitsTape.back(), std::vector<int>({117, 147, 148, 149, 150, 151, 152, 153, 154, 155, 304}));
    EXPECT_EQ(awardsTape, tape(std::vector<int>(), std::vector<int>{58}, std::vector<int>{2, 58}, std::vector<int>{2, 6, 58}));
    EXPECT_EQ(letterTape, tape(true, false));
    EXPECT_EQ(shieldTape, tape(false, true, false));
    EXPECT_CONTAINS(goldTape.adjacentDeltas(), 1000); // Andover Potbello's pay.
    EXPECT_EQ(reputationTape, tape(0, 200, 0)); // Wilbur Humphrey's thanks, then Loretta Fleise's price rise.
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 1);
}
