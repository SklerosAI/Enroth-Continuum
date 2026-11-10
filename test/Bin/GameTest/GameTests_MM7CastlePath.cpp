#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

static std::pair<Vec3f, int> approachOutdoorFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

GAME_TEST(MM7CastlePath, Goblins) {
    // The first castle visit introduces the butler, and leaving after killing every eligible goblin completes his task.
    for (bool survivor : {false, true}) {
        SCOPED_TRACE(fmt::format("survivor={}", survivor));
        test.prepareForNextTest();
        auto mapTape = tapes.map();
        auto activeTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_GOBLINS_ACTIVE);
        auto butlerTape = tapes.questBit(QBIT_BUTLER_MET);
        auto killedTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_GOBLINS_KILLED);
        auto repairTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE);
        auto butlerHouseTape = tapes.custom([] { return pNPCStats->pNPCData[58].house; });
        auto awardTape = tapes.custom([] { return static_cast<bool>(pParty->pCharacters[0]._achievedAwardsBits[AWARD_HARMONDALE_CASTLE_CLEARED]); });
        auto experienceTape = tapes.totalExperience();
        game.startNewGame();
        engine->config->debug.NoDamage.setValue(true);
        test.startTaping();
        game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
        auto castleDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 301; });
        ASSERT_NE(castleDoor, pOutdoor->faces.end());
        auto [castlePosition, castleYaw] = approachOutdoorFace(*castleDoor);
        game.teleportTo(MAP_HARMONDALE, castlePosition, castleYaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_TRUE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_GOBLINS_ACTIVE]);
        ASSERT_TRUE(pParty->_questBits[QBIT_BUTLER_MET]);
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
        ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);
        ASSERT_FALSE(Actor::isActorKilled(KILL_CHECK_GROUPID, 5, 0));
        const bool alert = GetAlertStatus();
        auto firstGoblin = std::ranges::find_if(pActors, [alert](Actor &actor) {
            return actor.group == 5 && !!(actor.attributes & ACTOR_UNKNOW7) == alert && !actor.IsNotAlive();
        });
        ASSERT_NE(firstGoblin, pActors.end());
        for (Actor &actor : pActors)
            if (actor.group == 5 && (!survivor || &actor != &*firstGoblin))
                actor.aiState = Dead;
        ASSERT_EQ(Actor::isActorKilled(KILL_CHECK_GROUPID, 5, 0), !survivor);
        game.teleportTo(MAP_CASTLE_HARMONDALE, pParty->pos, 180);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
        ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
        EXPECT_FALSE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE]);
        EXPECT_FALSE(pParty->pCharacters[0]._achievedAwardsBits[AWARD_HARMONDALE_CASTLE_CLEARED]);
        auto tavernDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 15; });
        ASSERT_NE(tavernDoor, pOutdoor->faces.end());
        ASSERT_TRUE(tavernDoor->Clickable());
        auto [doorPosition, doorYaw] = approachOutdoorFace(*tavernDoor);
        game.teleportTo(MAP_HARMONDALE, doorPosition, doorYaw);
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

        EXPECT_EQ(mapTape, tape(MAP_EMERALD_ISLAND, MAP_HARMONDALE, MAP_CASTLE_HARMONDALE, MAP_HARMONDALE));
        EXPECT_EQ(activeTape, survivor ? tape(false, true) : tape(false, true, false));
        EXPECT_EQ(butlerTape, tape(false, true));
        EXPECT_EQ(killedTape, survivor ? tape(false) : tape(false, true));
        EXPECT_EQ(repairTape, survivor ? tape(false) : tape(false, true));
        EXPECT_EQ(butlerHouseTape, survivor ? tape(HOUSE_INVALID, HOUSE_TAVERN_HARMONDALE)
                                            : tape(HOUSE_INVALID, HOUSE_TAVERN_HARMONDALE, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM));
        EXPECT_EQ(awardTape, survivor ? tape(false) : tape(false, true));
        EXPECT_EQ(experienceTape.delta(), survivor ? 0 : 4 * 5000);
    }
}
