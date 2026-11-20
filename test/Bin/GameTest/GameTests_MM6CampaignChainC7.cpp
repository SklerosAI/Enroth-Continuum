#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIBranchlessDialogue.h"

#include "MM6CampaignChain.h"

static std::vector<int> tombBits() {
    // The set ones of the control room chest, the four crystal quests, the Control Cube quest, the two the Oracle's
    // Kreegan topic sets, the cube carried, and the tomb's six chests and bookshelves.
    std::vector<int> result;
    for (int bit : {42, 162, 163, 164, 165, 166, 169, 195, 229, 230, 231, 232, 233, 234, 235})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static std::vector<int> oracleAwards() {
    // The six council votes, the traitor exposed, the letter delivered, then the Oracle's two awards.
    std::vector<int> result;
    for (int award : {2, 3, 4, 5, 6, 7, 32, 33, 34, 58})
        if (pParty->pCharacters[0]._achievedAwardsBits[static_cast<AwardId>(award)])
            result.push_back(award);
    return result;
}

static const BLVDoor &door(int doorId) {
    return *std::ranges::find(pIndoor->doors, static_cast<uint32_t>(doorId), &BLVDoor::doorId);
}

static void askOracle(EngineController &game, std::initializer_list<int> topics) {
    enterMM6CampaignChainOracle(game);
    useMM6CampaignChainIndoorEventFace(game, 3); // The Oracle. Her screen is still raised from C5.
    talkToMM6CampaignChainNpc(game, 8);
    for (int topic : topics)
        ASSERT_TRUE(selectMM6CampaignChainTopic(game, topic));
    leaveMM6CampaignChainHouse(game);
    useMM6CampaignChainIndoorEventFace(game, 50); // The way out.
    confirmMM6CampaignChainTransition(game);
}

static void selectKeyCarrier(EngineController &game) {
    // The first character carries every key and scroll, and a keyed check reads the active character's items. A
    // monster's hit before the check can still move the active character off the carrier, so callers try again.
    for (int frame = 0; frame < 20 && pParty->pCharacters[0].timeToRecovery > 0_ticks; frame++)
        game.tick(); // A monster's hit leaves a short recovery.
    if (!pParty->hasActiveCharacter() || pParty->activeCharacterIndex() != 0) {
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasActiveCharacter());
    ASSERT_EQ(pParty->activeCharacterIndex(), 0);
}

static void takeChestItems(EngineController &game, int chestEvent, int chestId, std::initializer_list<int> items) {
    for (int item : items) {
        for (int attempt = 0; attempt < 3 && current_screen_type != SCREEN_CHEST; attempt++) {
            selectKeyCarrier(game);
            clickMM6CampaignChainIndoorEventFace(game, chestEvent);
        }
        ASSERT_EQ(current_screen_type, SCREEN_CHEST);
        takeMM6CampaignChainChestItem(game, chestId, static_cast<ItemId>(item));
    }
}

static void useKeyedDoor(EngineController &game, int eventId, int keyId) {
    for (int attempt = 0; attempt < 3 && pParty->hasItem(static_cast<ItemId>(keyId)); attempt++) {
        selectKeyCarrier(game);
        useMM6CampaignChainIndoorEventFace(game, eventId);
    }
    ASSERT_FALSE(pParty->hasItem(static_cast<ItemId>(keyId)));
}

static void speakCode(EngineController &game, int plateEvent, std::string_view code) {
    for (int attempt = 0; attempt < 3 && !pGUIWindow_BranchlessDialogue; attempt++) {
        selectKeyCarrier(game);
        walkOntoMM6CampaignChainIndoorEventPlate(game, plateEvent);
    }
    answerMM6CampaignChainQuestion(game, code);
}

static void enterTomb(EngineController &game) {
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo("outb3.odm"), 90);
    confirmMM6CampaignChainTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("pyramid.blv"));
}

static void takeControlCube(EngineController &game) {
    // The keys, scrolls and codes of the tomb in the order they open each other, then out with the cube.
    clickMM6CampaignChainIndoorEventFace(game, 29); // The switch by the chest, where a Djinn and two Genies stand guard.
    for (int frame = 0; frame < 50 && door(38).state != DOOR_OPEN; frame++)
        game.tick(); // The cover over the chest slides away.
    takeChestItems(game, 44, 4, {574, 537});
    useKeyedDoor(game, 31, 574); // The Flame Door.
    takeChestItems(game, 42, 2, {573, 538});
    useKeyedDoor(game, 11, 573); // The Water Temple door.
    takeChestItems(game, 45, 5, {576, 539});
    takeChestItems(game, 43, 3, {577, 542});
    for (int bookshelf : {26, 27}) {
        clickMM6CampaignChainIndoorEventFace(game, bookshelf);
        game.pressGuiButton("Game_Character1"); // The scroll comes in hand and goes into the first character's backpack.
        game.tick();
    }
    speakCode(game, 21, "kcops");
    speakCode(game, 22, "ulus");
    speakCode(game, 23, "aruhu");
    speakCode(game, 24, "yttocs");
    speakCode(game, 25, "yoccm");
    speakCode(game, 1, "krik"); // The Well of VARN.
    useMM6CampaignChainIndoorEventFace(game, 35); // The control room entry.
    takeChestItems(game, 41, 1, {456});
    useMM6CampaignChainIndoorEventFace(game, 17); // The way out.
    confirmMM6CampaignChainTransition(game);
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter7) {
    // Fixtures: C1's party strength. Teleports to the High Council door and the tomb's door on outb3, inside the Oracle
    // to the Oracle and the way out, and inside the tomb in front of the switch, doors, chests, bookshelves, pools and
    // the well, into the control room and to the way out.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 6);
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId outb3 = pMapTable->GetMapInfo("outb3.odm");
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    const MapId pyramid = pMapTable->GetMapInfo("pyramid.blv");
    ASSERT_EQ(engine->_currentLoadedMapId, outc2);

    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return tombBits(); });
    auto awardsTape = tapes.custom([] { return oracleAwards(); });
    auto flameKeyTape = tapes.hasItem(static_cast<ItemId>(574));
    auto waterKeyTape = tapes.hasItem(static_cast<ItemId>(573));
    auto libraryKeyTape = tapes.hasItem(static_cast<ItemId>(576));
    auto controlKeyTape = tapes.hasItem(static_cast<ItemId>(577));
    auto cubeTape = tapes.hasItem(static_cast<ItemId>(456));
    auto statusTape = tapes.statusBar();
    auto oracleTopicsTape = tapes.custom([] {
        return std::vector<unsigned>{pNPCStats->pNPCData[8].dialogue_1_evt_id, pNPCStats->pNPCData[8].dialogue_2_evt_id};
    });
    auto nicolaiTopicTape = tapes.custom([] { return pNPCStats->pNPCData[13].dialogue_2_evt_id; });
    auto experienceTape = tapes.totalExperience();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    askOracle(game, {76}); // The Control Cube.
    enterTomb(game);
    takeControlCube(game);
    askOracle(game, {76, 77}); // The cube, then the Kreegan.

    EXPECT_EQ(mapTape, tape(outc2, oracle, outc2, outb3, pyramid, outb3, outc2, oracle, outc2));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{162, 163, 164, 165}, std::vector<int>{166}, std::vector<int>{166, 231}, std::vector<int>{166, 229, 231},
                             std::vector<int>{166, 229, 231, 232}, std::vector<int>{166, 229, 230, 231, 232},
                             std::vector<int>{166, 229, 230, 231, 232, 234}, std::vector<int>{166, 229, 230, 231, 232, 233, 234},
                             std::vector<int>{166, 230, 231, 232, 233, 234}, std::vector<int>{166, 230, 231, 233, 234},
                             std::vector<int>{166, 230, 231, 233}, std::vector<int>{166, 230, 231}, std::vector<int>{166, 231},
                             std::vector<int>{166}, std::vector<int>{42, 166, 195}, std::vector<int>{42}, std::vector<int>{42, 169, 235}));
    EXPECT_EQ(awardsTape, tape(std::vector<int>{2, 3, 4, 5, 6, 7, 32, 58}, std::vector<int>{2, 3, 4, 5, 6, 7, 32, 33, 58},
                               std::vector<int>{2, 3, 4, 5, 6, 7, 32, 33, 34, 58}));
    EXPECT_EQ(flameKeyTape, tape(false, true, false)); // The Flame Door takes its key.
    EXPECT_EQ(waterKeyTape, tape(false, true, false)); // So does the Water Temple door.
    EXPECT_EQ(libraryKeyTape, tape(false, true, false)); // The library chest takes its key.
    EXPECT_EQ(controlKeyTape, tape(false, true, false)); // So does the control room chest.
    EXPECT_EQ(cubeTape, tape(false, true, false));
    EXPECT_CONTAINS(statusTape, std::string("The waters part."));
    EXPECT_EQ(oracleTopicsTape, tape(std::vector<unsigned>{76, 0}, std::vector<unsigned>{77, 78}));
    EXPECT_EQ(nicolaiTopicTape, tape(25u, 26u));
    EXPECT_EQ(experienceTape.delta(), 2000000); // The Oracle gives every character 500,000 for the cube.
    EXPECT_EQ(reputationTape, tape(1600));
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 7);
}
