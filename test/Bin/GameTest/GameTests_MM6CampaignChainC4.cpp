#include <initializer_list>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"

#include "MM6CampaignChain.h"

static std::vector<int> councilBits() {
    // The set ones of Temper's quest, Oracle admission, the traitor exposed, the plans carried, the letter carried,
    // Humphrey's cure quest and the letter for the council.
    std::vector<int> result;
    for (int bit : {113, 167, 168, 184, 190, 200, 201})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static std::vector<int> councilAwards() {
    // The six council votes, the traitor exposed, then the letter delivered to Wilbur Humphrey.
    std::vector<int> result;
    for (int award : {2, 3, 4, 5, 6, 7, 32, 58})
        if (pParty->pCharacters[0]._achievedAwardsBits[static_cast<AwardId>(award)])
            result.push_back(award);
    return result;
}

static void confirmTransition(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void talkInCastle(EngineController &game, MapId map, int doorEvent, int npcId, std::initializer_list<int> topics) {
    enterMM6CampaignChainCastle(game, map, doorEvent);
    talkToMM6CampaignChainNpc(game, npcId);
    for (int topic : topics)
        ASSERT_TRUE(selectMM6CampaignChainTopic(game, topic));
    leaveMM6CampaignChainHouse(game);
}

static void talkToSilvertongue(EngineController &game) {
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo("outc2.odm"), 49); // The High Council.
    talkToMM6CampaignChainNpc(game, 304);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 380));
    leaveMM6CampaignChainHouse(game);
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter4) {
    // Fixtures: C1's party strength, and the party takes no damage inside the Devil Outpost and the Temple of Baa.
    // Teleports to Castle Temper, the Devil Outpost and the Temple of Baa doors, the High Council and Castle Ironfist,
    // and inside both dungeons next to the monsters, their corpses and the plans, to the letter's chest and the ways
    // out.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 3);
    const MapId outb1 = pMapTable->GetMapInfo("outb1.odm");
    const MapId outc1 = pMapTable->GetMapInfo("outc1.odm");
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId zdwj02 = pMapTable->GetMapInfo("zdwj02.blv");
    const MapId t7 = pMapTable->GetMapInfo("t7.blv");
    ASSERT_EQ(engine->_currentLoadedMapId, outc1);
    ASSERT_EQ(councilAwards(), std::vector<int>({2, 3, 5, 6, 7, 58}));

    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return councilBits(); });
    auto awardsTape = tapes.custom([] { return councilAwards(); });
    auto plansTape = tapes.hasItem(static_cast<ItemId>(506));
    auto cloakTape = tapes.hasItem(static_cast<ItemId>(485));
    auto clericKeyTape = tapes.hasItem(static_cast<ItemId>(567));
    auto sorcererKeyTape = tapes.hasItem(static_cast<ItemId>(492));
    auto letterTape = tapes.hasItem(static_cast<ItemId>(502));
    auto silvertongueTape = tapes.custom([] { return pNPCStats->pNPCData[304].house; });
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    talkInCastle(game, outc2, 211, 6, {61}); // Osric Temper wants the Devil Outpost destroyed.

    useMM6CampaignChainEventFace(game, outb1, 94); // The Devil Outpost.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, zdwj02);
    engine->config->debug.NoDamage.setValue(true);
    killMM6CampaignChainActor(game, 0); // The Devil Warrior, who carries the plans.
    for (int frame = 0; frame < 100 && pActors[0].aiState != Removed; frame++)
        game.tick(); // The warrior falls and vanishes, and the plans drop to the floor.
    pickUpMM6CampaignChainFloorItem(game, static_cast<ItemId>(506));
    useMM6CampaignChainIndoorEventFace(game, 1); // The way out.
    confirmTransition(game);
    engine->config->debug.NoDamage.setValue(false);

    talkInCastle(game, outc2, 211, 6, {62}); // Osric Temper takes the plans.
    talkToSilvertongue(game); // Slicker Silvertongue refuses his vote, and Wilbur Humphrey hears of it.
    talkInCastle(game, outd3, 43, 4, {101}); // Wilbur Humphrey gives the Cloak of Baa to find a cure.
    game.pressGuiButton("Game_Character1"); // The cloak comes in hand and goes into the first character's backpack.
    game.tick();

    useMM6CampaignChainEventFace(game, outb1, 90); // The Temple of Baa lets in followers of Baa.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, t7);
    engine->config->debug.NoDamage.setValue(true);
    for (int priest : {0, 1}) { // The priests of Baa with the High Cleric's and the High Sorcerer's keys.
        killMM6CampaignChainActor(game, priest);
        lootMM6CampaignChainCorpse(game, priest);
    }
    useMM6CampaignChainIndoorEventFace(game, 40); // The chest the two keys open.
    takeMM6CampaignChainChestItem(game, 7, static_cast<ItemId>(502));
    useMM6CampaignChainIndoorEventFace(game, 60); // The way out.
    confirmTransition(game);
    engine->config->debug.NoDamage.setValue(false);

    talkToSilvertongue(game); // The letter from Zenofex exposes him.

    EXPECT_EQ(mapTape, tape(outc1, outc2, outb1, zdwj02, outb1, outc2, outd3, outb1, t7, outb1, outc2));
    EXPECT_EQ(bitsTape, tape(std::vector<int>(), std::vector<int>{113}, std::vector<int>{113, 184}, std::vector<int>(),
                             std::vector<int>{200}, std::vector<int>{190, 200}, std::vector<int>{167, 168}));
    EXPECT_EQ(awardsTape, tape(std::vector<int>{2, 3, 5, 6, 7, 58}, std::vector<int>{2, 3, 4, 5, 6, 7, 58},
                               std::vector<int>{2, 3, 4, 5, 6, 7, 32, 58}));
    EXPECT_EQ(plansTape, tape(false, true, false));
    EXPECT_EQ(cloakTape, tape(false, true));
    EXPECT_EQ(clericKeyTape, tape(false, true, false));
    EXPECT_EQ(sorcererKeyTape, tape(false, true, false));
    EXPECT_EQ(letterTape, tape(false, true, false));
    EXPECT_EQ(silvertongueTape, tape(static_cast<HouseId>(165), HOUSE_INVALID)); // He leaves the council for good.
    EXPECT_EQ(reputationTape, tape(600, 800, 1600)); // Temper's thanks, then four times 200 for exposing the traitor.
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 4);
}
