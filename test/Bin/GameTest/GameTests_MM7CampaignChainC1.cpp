#include <algorithm>
#include <iterator>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM7CampaignChain.h"

GAME_TEST(MM7CampaignChain, DISABLED_Chapter1) {
    // Fixtures: the castle goblins die by AI state writes. The party walks to the castle and tavern doors by teleport.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save001.mm7"));
    loadCampaignChainSlot(game, 0);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_ESCAPED_EMERALD_ISLE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_BUTLER_MET]);

    auto mapTape = tapes.map();
    auto activeTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_GOBLINS_ACTIVE);
    auto butlerTape = tapes.questBit(QBIT_BUTLER_MET);
    auto killedTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_GOBLINS_KILLED);
    auto repairTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE);
    auto butlerHouseTape = tapes.custom([] { return pNPCStats->pNPCData[58].house; });
    auto awardTape = tapes.custom([] { return static_cast<bool>(pParty->pCharacters[0]._achievedAwardsBits[AWARD_HARMONDALE_CASTLE_CLEARED]); });
    auto experienceTape = tapes.totalExperience();
    test.startTaping();

    auto castleDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 301; });
    ASSERT_NE(castleDoor, pOutdoor->faces.end());
    auto [castlePosition, castleYaw] = approachCampaignChainFace(*castleDoor);
    game.teleportTo(MAP_HARMONDALE, castlePosition, castleYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE); // The butler greets the party.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);
    for (Actor &actor : pActors)
        if (actor.group == 5)
            actor.aiState = Dead;
    game.teleportTo(MAP_CASTLE_HARMONDALE, pParty->pos, 180); // Turn around to the entrance door.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);

    auto tavernDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 15; });
    ASSERT_NE(tavernDoor, pOutdoor->faces.end());
    auto [tavernPosition, tavernYaw] = approachCampaignChainFace(*tavernDoor);
    game.teleportTo(MAP_HARMONDALE, tavernPosition, tavernYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_TAVERN_HARMONDALE);
    auto butler = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) { return npc.npc == &pNPCStats->pNPCData[58]; });
    ASSERT_NE(butler, houseNpcs.end());
    game.pressGuiButton(fmt::format("House_Npc{}", std::distance(houseNpcs.begin(), butler)));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    EXPECT_EQ(mapTape, tape(MAP_HARMONDALE, MAP_CASTLE_HARMONDALE, MAP_HARMONDALE));
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(butlerTape, tape(false, true));
    EXPECT_EQ(killedTape, tape(false, true));
    EXPECT_EQ(repairTape, tape(false, true));
    EXPECT_EQ(butlerHouseTape, tape(HOUSE_INVALID, HOUSE_TAVERN_HARMONDALE, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM));
    EXPECT_EQ(awardTape, tape(false, true));
    EXPECT_EQ(experienceTape.delta(), 4 * 5000);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 1);
}
