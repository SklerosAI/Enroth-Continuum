#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

namespace {

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

void seedPostAltarLightPath() {
    // Light alignment, Walls and altar pieces are covered by earlier tests.
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
    pParty->_questBits.set(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
    pNPCStats->pNPCData[80].house = HOUSE_CASTLE_LAMBENT_THRONE_ROOM;
    pNPCStats->pNPCData[80].dialogue_1_evt_id = 149;
}

void openThroneRoom(EngineController &game) {
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    auto advisor = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) {
        return npc.npc == &pNPCStats->pNPCData[80];
    });
    ASSERT_NE(advisor, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", advisor - houseNpcs.begin()));
        game.tick();
    }
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 3 && current_screen_type == SCREEN_HOUSE; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

bool selectAdvisorEvent(EngineController &game, unsigned int eventId) {
    NPCData &npc = pNPCStats->pNPCData[80];
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(&npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

}  // namespace

GAME_TEST(MM7LightXenofex, QuestTopicRequiresAssassination) {
    game.startNewGame();
    seedPostAltarLightPath();
    openThroneRoom(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_1_evt_id, 149);
    EXPECT_TRUE(prepareScriptedNPCDialogueTopics(&pNPCStats->pNPCData[80]).empty());
    EXPECT_FALSE(pParty->_questBits[QBIT_XENOFEX_LIGHT_ACTIVE]);
    leaveHouse(game);

    pParty->_questBits.set(QBIT_TOLBERTI_ASSASSINATED);
    openThroneRoom(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectAdvisorEvent(game, 149));
    EXPECT_TRUE(pParty->_questBits[QBIT_XENOFEX_LIGHT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_XENOFEX_QUEST_FINISHED]);
}

GAME_TEST(MM7LightXenofex, MapLeaveAndHandIn) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    seedPostAltarLightPath();
    pParty->_questBits.set(QBIT_TOLBERTI_ASSASSINATED);
    openThroneRoom(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectAdvisorEvent(game, 149));
    ASSERT_TRUE(pParty->_questBits[QBIT_XENOFEX_LIGHT_ACTIVE]);
    leaveHouse(game);

    // Travel and the encounter are fixtures. Changing maps still runs the
    // production Colony Zod OnMapLeave event in the real MM7 event data.
    game.teleportTo(MAP_COLONY_ZOD, Vec3f(-1849, 6726, 934), 0);
    ASSERT_FALSE(pActors.empty());
    ASSERT_FALSE(pActors[0].IsNotAlive());
    game.teleportTo(MAP_LAND_OF_THE_GIANTS, Vec3f(), 0);
    EXPECT_FALSE(pParty->_questBits[QBIT_XENOFEX_KILLED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_XENOFEX_QUEST_FINISHED]);

    game.teleportTo(MAP_COLONY_ZOD, Vec3f(-1849, 6726, 934), 0);
    ASSERT_FALSE(pActors.empty());
    Actor::Die(0);  // Fixture the outcome, not a player attack or encounter.
    ASSERT_TRUE(pActors[0].IsNotAlive());
    game.teleportTo(MAP_LAND_OF_THE_GIANTS, Vec3f(), 0);
    ASSERT_TRUE(pParty->_questBits[QBIT_XENOFEX_KILLED]);
    ASSERT_FALSE(pParty->_questBits[QBIT_XENOFEX_QUEST_FINISHED]);

    openThroneRoom(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    int goldBeforeHandIn = pParty->GetGold();
    ASSERT_TRUE(selectAdvisorEvent(game, 150));
    EXPECT_TRUE(pParty->_questBits[QBIT_XENOFEX_QUEST_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_XENOFEX_LIGHT_ACTIVE]);
    EXPECT_EQ(pParty->GetGold() - goldBeforeHandIn, 50000);
    EXPECT_EQ(current_screen_type, SCREEN_HOUSE);
    EXPECT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
}
