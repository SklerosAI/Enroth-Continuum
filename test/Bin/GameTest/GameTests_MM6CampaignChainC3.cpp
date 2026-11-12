#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM6CampaignChain.h"

static std::vector<int> councilBits() {
    // The set ones of Anthony Stone's quest, Stromgard's winter quest, the Prince of Thieves caught, the winter ended
    // and the Hermit's spell broken.
    std::vector<int> result;
    for (int bit : {98, 120, 170, 175, 228})
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

GAME_TEST(MM6CampaignChain, DISABLED_Chapter3) {
    // Fixtures: C1's party strength. Teleports to Castle Stromgard and Castle Stone, to the Hermit's hut on its
    // mountaintop, to Takao's house in Free Haven, and inside the sewer to the Prince's bed and back to the ladder.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 2);
    const MapId oute2 = pMapTable->GetMapInfo("oute2.odm");
    const MapId outc1 = pMapTable->GetMapInfo("outc1.odm");
    const MapId outb1 = pMapTable->GetMapInfo("outb1.odm");
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId sewer = pMapTable->GetMapInfo("sewer.blv");
    ASSERT_EQ(engine->_currentLoadedMapId, oute2);
    ASSERT_EQ(councilAwards(), std::vector<int>({2, 3, 6, 58}));

    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto bitsTape = tapes.custom([] { return councilBits(); });
    auto awardsTape = tapes.custom([] { return councilAwards(); });
    auto princeTape = tapes.custom([] { return pNPCStats->pNPCData[17].Hired(); });
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    talkInCastle(game, outc1, 34, 15, {87}); // Stromgard wants the winter ended.
    talkInCastle(game, outc1, 33, 16, {32}); // Anthony Stone wants the Prince of Thieves.

    useMM6CampaignChainEventFace(game, outb1, 14); // The Hermit's hut.
    talkToMM6CampaignChainNpc(game, 19);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 96)); // The Hermit breaks the spell on the weather.
    leaveMM6CampaignChainHouse(game);

    useMM6CampaignChainEventFace(game, outc2, 96); // Takao's house, with a trapdoor into the sewer.
    const std::string enterSewer = localization->format(LSTR_ENTER_S, pMapTable->pInfos[sewer].name);
    auto trapdoor = std::ranges::find(window_SpeakInHouse->vButtons, enterSewer, &GUIButton::label);
    ASSERT_NE(trapdoor, window_SpeakInHouse->vButtons.end());
    game.pressGuiButton((*trapdoor)->id);
    game.tick();
    game.pressGuiButton("HouseExit_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, sewer);

    useMM6CampaignChainIndoorEventFace(game, 8); // The Prince hides under his bed.
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    useMM6CampaignChainIndoorEventFace(game, 20); // The ladder up to Takao's house.
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, outc2);

    talkInCastle(game, outc1, 33, 16, {33}); // Anthony Stone takes the Prince.
    talkInCastle(game, outc1, 34, 15, {89}); // Stromgard sees the snow gone.

    EXPECT_EQ(mapTape, tape(oute2, outc1, outb1, outc2, sewer, outc2, outc1));
    EXPECT_CONTAINS(screenTape, SCREEN_NPC_DIALOGUE);
    EXPECT_EQ(bitsTape, tape(std::vector<int>(), std::vector<int>{120}, std::vector<int>{98, 120}, std::vector<int>{98, 120, 228},
                             std::vector<int>{98, 120, 170, 228}, std::vector<int>{120, 170, 228}, std::vector<int>{170, 175, 228}));
    EXPECT_EQ(awardsTape, tape(std::vector<int>{2, 3, 6, 58}, std::vector<int>{2, 3, 5, 6, 58}, std::vector<int>{2, 3, 5, 6, 7, 58}));
    EXPECT_EQ(princeTape, tape(false, true, false));
    EXPECT_EQ(reputationTape, tape(200, 400, 600)); // Anthony Stone's thanks, then Stromgard's.
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 3);
}
