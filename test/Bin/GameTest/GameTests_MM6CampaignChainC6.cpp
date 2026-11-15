#include <algorithm>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/MM6MapByteState.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM6CampaignChain.h"

static std::vector<int> crystalBits() {
    // The set ones of Beta taken from Castle Alamos, the four crystals in their modules, Epsilon taken from Castle
    // Kriegspire, the four crystal quests, Oracle admission, the traitor exposed, Beta, Delta and Epsilon carried, and
    // the four modules full.
    std::vector<int> result;
    for (int bit : {54, 100, 101, 102, 103, 104, 162, 163, 164, 165, 167, 168, 192, 193, 194, 280, 281, 282, 283})
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

static void enterDungeon(EngineController &game, const char *outdoorMap, int doorEvent, const char *dungeon) {
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo(outdoorMap), doorEvent);
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo(dungeon));
}

static void leaveDungeon(EngineController &game, int exitEvent) {
    useMM6CampaignChainIndoorEventFace(game, exitEvent);
    confirmTransition(game);
}

static void takeCrystal(EngineController &game, int decorationId) {
    useMM6CampaignChainDecoration(game, decorationId);
    game.pressGuiButton("Game_Character1"); // The crystal comes in hand and goes into the first character's backpack.
    game.tick();
}

static void enterOracle(EngineController &game) {
    // The High Council's way down, which its approval opens.
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo("outc2.odm"), 49);
    auto button = std::ranges::find(window_SpeakInHouse->vButtons, localization->format(LSTR_ENTER_S, pMapTable->pInfos[oracle].name), &GUIButton::label);
    ASSERT_NE(button, window_SpeakInHouse->vButtons.end());
    game.pressGuiButton((*button)->id);
    game.tick();
    game.pressGuiButton("HouseExit_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, oracle);
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter6) {
    // Fixtures: C1's party strength. Teleports to the castle doors on oute1, outc3 and outb1 and to the High Council
    // door, inside the castles next to the password plate, the crystals and the ways out, and inside the Oracle to the
    // modules and the way out.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 5);
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId oute1 = pMapTable->GetMapInfo("oute1.odm");
    const MapId outc3 = pMapTable->GetMapInfo("outc3.odm");
    const MapId outb1 = pMapTable->GetMapInfo("outb1.odm");
    const MapId cd1 = pMapTable->GetMapInfo("cd1.blv");
    const MapId cd2 = pMapTable->GetMapInfo("cd2.blv");
    const MapId cd3 = pMapTable->GetMapInfo("cd3.blv");
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    ASSERT_EQ(engine->_currentLoadedMapId, outc2);

    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return crystalBits(); });
    auto awardsTape = tapes.custom([] { return councilAwards(); });
    auto betaTape = tapes.hasItem(static_cast<ItemId>(551));
    auto deltaTape = tapes.hasItem(static_cast<ItemId>(552));
    auto epsilonTape = tapes.hasItem(static_cast<ItemId>(553));
    auto statusTape = tapes.statusBar();
    auto powerTape = tapes.custom([oracle] { // The Oracle's power panel, a byte of its map.
        return engine->_currentLoadedMapId == oracle && !pIndoor->mm6DeltaBytes.empty() && MM6MapByteState(pIndoor->mm6DeltaBytes).compare(111, 1);
    });
    auto oracleTopicsTape = tapes.custom([] {
        return std::vector<unsigned>{pNPCStats->pNPCData[8].dialogue_1_evt_id, pNPCStats->pNPCData[8].dialogue_2_evt_id};
    });
    auto experienceTape = tapes.totalExperience();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    enterDungeon(game, "oute1.odm", 90, "cd1.blv"); // Castle Alamos.
    walkOntoMM6CampaignChainIndoorEventPlate(game, 69); // The plate before the inner rooms asks for the password.
    answerMM6CampaignChainQuestion(game, "jbard");
    takeCrystal(game, 394);
    leaveDungeon(game, 61);

    enterDungeon(game, "outc3.odm", 91, "cd2.blv"); // Castle Darkmoor.
    takeCrystal(game, 329);
    leaveDungeon(game, 60);

    enterDungeon(game, "outb1.odm", 93, "cd3.blv"); // Castle Kriegspire.
    takeCrystal(game, 290);
    leaveDungeon(game, 19);

    enterOracle(game);
    for (int module : {5, 15, 17})
        useMM6CampaignChainIndoorEventFace(game, module);
    leaveDungeon(game, 50);

    EXPECT_EQ(mapTape, tape(outc2, oute1, cd1, oute1, outc3, cd2, outc3, outb1, cd3, outb1, outc2, oracle, outc2));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{100, 162, 163, 164, 165, 167, 168, 282},
                             std::vector<int>{54, 100, 162, 163, 164, 165, 167, 168, 192, 282},
                             std::vector<int>{54, 100, 162, 163, 164, 165, 167, 168, 192, 193, 282},
                             std::vector<int>{54, 100, 104, 162, 163, 164, 165, 167, 168, 192, 193, 194, 282},
                             std::vector<int>{54, 100, 101, 104, 162, 163, 164, 165, 167, 168, 193, 194, 280, 282},
                             std::vector<int>{54, 100, 101, 102, 104, 162, 163, 164, 165, 167, 168, 194, 280, 281, 282},
                             std::vector<int>{54, 100, 101, 102, 103, 104, 162, 163, 164, 165, 167, 168, 280, 281, 282, 283}));
    EXPECT_EQ(awardsTape, tape(std::vector<int>{2, 3, 4, 5, 6, 7, 32, 58}));
    EXPECT_EQ(betaTape, tape(false, true, false));
    EXPECT_EQ(deltaTape, tape(false, true, false));
    EXPECT_EQ(epsilonTape, tape(false, true, false));
    EXPECT_CONTAINS(statusTape, std::string("Who told you!  Alright, you may pass!"));
    EXPECT_EQ(powerTape, tape(false, true, false));
    EXPECT_EQ(oracleTopicsTape, tape(std::vector<unsigned>{73, 74}, std::vector<unsigned>{76, 0}));
    EXPECT_EQ(experienceTape.delta(), 1200000); // Each module gives every character 100,000.
    EXPECT_EQ(reputationTape, tape(1600));
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 6);
}
