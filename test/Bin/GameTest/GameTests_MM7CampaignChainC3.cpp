#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"

#include "MM7CampaignChain.h"

GAME_TEST(MM7CampaignChain, DISABLED_Chapter3) {
    // Fixtures: no actors while the party rests in the castle. Teleports to Harmondale, to the castle door, to the
    // Bracada Desert tavern door and to the arbiter's door.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save003.mm7"));
    loadCampaignChainSlot(game, 2);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_STONE_CITY);
    ASSERT_TRUE(pParty->_questBits[QBIT_HARMONDALE_REBUILT]);
    ASSERT_FALSE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_TIMER_STARTED]);

    auto timerTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_TIMER_STARTED);
    auto messengerTape = tapes.questBit(QBIT_ARBITER_MESSENGER_MET);
    auto choiceTape = tapes.questBit(QBIT_ARBITER_CHOICE_ACTIVE);
    auto lightTape = tapes.questBit(QBIT_LIGHT_PATH);
    auto darkTape = tapes.questBit(QBIT_DARK_PATH);
    auto gavinTape = tapes.questBit(QBIT_GAVIN_MAGNUS_VISIT_ACTIVE);
    auto alignmentTape = tapes.custom([] { return pParty->alignment; });
    auto judgesTape = tapes.custom([] { return std::pair(pNPCStats->pNPCData[77].house, pNPCStats->pNPCData[78].house); });
    auto judgeHiredTape = tapes.custom([] { return pNPCStats->pNPCData[77].Hired(); });
    auto foodTape = tapes.food();
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    test.startTaping();

    game.teleportTo(MAP_HARMONDALE, Vec3f(-17331, 12547, 465), 0); // The Harmondale arrival spot.
    auto castleDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 301; });
    ASSERT_NE(castleDoor, pOutdoor->faces.end());
    auto [castlePosition, castleYaw] = approachCampaignChainFace(*castleDoor);
    game.teleportTo(MAP_HARMONDALE, castlePosition, castleYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);

    engine->config->debug.NoActors.setValue(true); // Rest encounters would wake the party and block the next rest.
    for (int i = 0; i < 112; i++) { // The messenger comes 112 days after the timer starts.
        game.pressAndReleaseKey(PlatformKey::KEY_R); // The rest screen closes after each rest.
        game.tick();
        for (std::string_view button : {"Rest_WaitTillDawn", "Rest_RestAndHeal"}) {
            game.pressGuiButton(button);
            game.tick();
            while (currentRestType != REST_NONE)
                game.tick();
        }
    }
    engine->config->debug.NoActors.setValue(false);
    ASSERT_FALSE(pParty->_questBits[QBIT_ARBITER_MESSENGER_MET]);
    game.teleportTo(MAP_CASTLE_HARMONDALE, pParty->pos, 180); // Turn around to the entrance door.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE); // The messenger.
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(-13180, 18432, 800), 0); // In front of the tavern door.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_TAVERN_BRACADA_DESERT);
    auto judge = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) { return npc.npc == &pNPCStats->pNPCData[77]; });
    ASSERT_NE(judge, houseNpcs.end());
    game.pressGuiButton(fmt::format("House_Npc{}", std::distance(houseNpcs.begin(), judge)));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    game.teleportTo(MAP_HARMONDALE, Vec3f(20992, -6450, 672), 90); // In front of the arbiter's door.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    EXPECT_EQ(timerTape, tape(false, true));
    EXPECT_EQ(messengerTape, tape(false, true));
    EXPECT_EQ(choiceTape, tape(false, true, false));
    EXPECT_EQ(lightTape, tape(false, true));
    EXPECT_EQ(darkTape, tape(false));
    EXPECT_EQ(gavinTape, tape(false, true));
    EXPECT_EQ(alignmentTape, tape(PartyAlignment::PartyAlignment_Neutral, PartyAlignment::PartyAlignment_Good));
    EXPECT_EQ(judgesTape, tape(std::pair(HOUSE_INVALID, HOUSE_INVALID),
                               std::pair(HOUSE_TAVERN_BRACADA_DESERT, HOUSE_TAVERN_DEYJA),
                               std::pair(HOUSE_HARMONDALE_ARBITER_1, HOUSE_INVALID)));
    EXPECT_EQ(judgeHiredTape, tape(false, true, false)); // The arbiter takes the judge back.
    EXPECT_EQ(foodTape.size(), 1); // Rest in the rebuilt castle is free.
    EXPECT_EQ(mapTape, tape(MAP_STONE_CITY, MAP_HARMONDALE, MAP_CASTLE_HARMONDALE, MAP_HARMONDALE, MAP_BRACADA_DESERT, MAP_HARMONDALE));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_TAVERN_BRACADA_DESERT, HOUSE_INVALID, HOUSE_HARMONDALE_ARBITER_1, HOUSE_INVALID));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 3);
}
