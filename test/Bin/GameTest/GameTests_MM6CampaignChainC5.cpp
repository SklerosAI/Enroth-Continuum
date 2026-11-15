#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM6CampaignChain.h"

static std::vector<int> oracleBits() {
    // The set ones of the crystal taken from its altar, Alpha in its module, the four crystal quests, Oracle admission,
    // the traitor exposed, the cylinder to the sewer, Alpha carried, and the module Alpha fills.
    std::vector<int> result;
    for (int bit : {53, 100, 162, 163, 164, 165, 167, 168, 176, 191, 282})
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

static const BLVDoor &door(int doorId) {
    return *std::ranges::find(pIndoor->doors, static_cast<uint32_t>(doorId), &BLVDoor::doorId);
}

static void confirmTransition(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void pressHouseOption(EngineController &game, const std::string &label) {
    auto button = std::ranges::find(window_SpeakInHouse->vButtons, label, &GUIButton::label);
    ASSERT_NE(button, window_SpeakInHouse->vButtons.end());
    game.pressGuiButton((*button)->id);
    game.tick();
}

static void enterOracle(EngineController &game) {
    // The High Council's way down, which its approval opens.
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo("outc2.odm"), 49);
    pressHouseOption(game, localization->format(LSTR_ENTER_S, pMapTable->pInfos[oracle].name));
    game.pressGuiButton("HouseExit_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, oracle);
}

static void walkForwardUntil(EngineController &game, const std::function<bool()> &condition) {
    game.pressKey(PlatformKey::KEY_UP);
    for (int frame = 0; frame < 50 && !condition(); frame++)
        game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_TRUE(condition());
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter5) {
    // Fixtures: C1's party strength. Teleports to the outd3 dock, to the Temple of Baa door on the Hermit's Isle and
    // to the High Council door, inside the Oracle to the floor plate before its chamber, to the Oracle, a crystal
    // module and the way out, inside the temple next to the crystal and the cylinder, and in the sewer to its ladder.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 4);
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId outa3 = pMapTable->GetMapInfo("outa3.odm");
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    const MapId t6 = pMapTable->GetMapInfo("t6.blv");
    const MapId sewer = pMapTable->GetMapInfo("sewer.blv");
    ASSERT_EQ(engine->_currentLoadedMapId, outc2);

    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return oracleBits(); });
    auto awardsTape = tapes.custom([] { return councilAwards(); });
    auto crystalTape = tapes.hasItem(static_cast<ItemId>(550));
    auto oracleTopicsTape = tapes.custom([] {
        return std::vector<unsigned>{pNPCStats->pNPCData[8].dialogue_1_evt_id, pNPCStats->pNPCData[8].dialogue_2_evt_id};
    });
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto experienceTape = tapes.totalExperience();
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    enterOracle(game);
    game.teleportTo(oracle, Vec3f(-1408, 900, -512), 90);
    walkForwardUntil(game, [] { return door(2).state != DOOR_OPEN; }); // The floor plate before the chamber.
    for (int frame = 0; frame < 50 && door(2).state != DOOR_CLOSED; frame++)
        game.tick(); // The wall in front of the Oracle slides away.
    useMM6CampaignChainIndoorEventFace(game, 3); // The Oracle.
    talkToMM6CampaignChainNpc(game, 8);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 73)); // Crystal Locations.
    leaveMM6CampaignChainHouse(game);
    useMM6CampaignChainIndoorEventFace(game, 50); // The way out.
    confirmTransition(game);

    game.teleportTo(outd3, Vec3f(21141, 4570, 192), 0); // The dock.
    game.pointMouseAtDecoration(128); // The ship.
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    pressHouseOption(game, localization->format(LSTR_D_DAYS_TO_S, 21, pMapTable->pInfos[outa3].name));
    game.tick(27); // The ship leaves two and a half seconds after the party pays, then the house closes.
    game.skipLoadingScreen();
    game.tick();

    useMM6CampaignChainEventFace(game, outa3, 90); // The Supreme Temple of Baa.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, t6);
    useMM6CampaignChainDecoration(game, 132); // Memory Crystal Alpha on its altar.
    game.pressGuiButton("Game_Character1"); // The crystal comes in hand and goes into the first character's backpack.
    game.tick();
    useMM6CampaignChainIndoorEventFace(game, 30); // The cylinder's wall, which lowers.
    for (int frame = 0; frame < 100 && door(21).state != DOOR_OPEN; frame++)
        game.tick();
    walkForwardUntil(game, [] { return current_screen_type == SCREEN_INPUT_BLV; }); // Onto the cylinder's floor.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, sewer);
    useMM6CampaignChainIndoorEventFace(game, 20); // The ladder up to Takao's house.
    confirmTransition(game);

    enterOracle(game);
    game.pointMouseAtFace(461); // The power panel by the entrance.
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    useMM6CampaignChainIndoorEventFace(game, 16); // A crystal module.
    useMM6CampaignChainIndoorEventFace(game, 50);
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(outc2, oracle, outc2, outd3, outa3, t6, sewer, outc2, oracle, outc2));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{167, 168}, std::vector<int>{162, 163, 164, 165, 167, 168},
                             std::vector<int>{53, 162, 163, 164, 165, 167, 168, 191}, std::vector<int>{53, 162, 163, 164, 165, 167, 168, 176, 191},
                             std::vector<int>{53, 100, 162, 163, 164, 165, 167, 168, 176, 282}));
    EXPECT_EQ(awardsTape, tape(std::vector<int>{2, 3, 4, 5, 6, 7, 32, 58}));
    EXPECT_EQ(crystalTape, tape(false, true, false));
    EXPECT_EQ(oracleTopicsTape, tape(std::vector<unsigned>{73, 74})); // The Oracle waits for all four crystals.
    EXPECT_CONTAINS(textTape, std::string(pNPCTopics[87].pText)); // Where the four crystals are.
    EXPECT_EQ(experienceTape.delta(), 400000); // The module gives every character 100,000.
    EXPECT_EQ(reputationTape, tape(1600));
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 5);
}
