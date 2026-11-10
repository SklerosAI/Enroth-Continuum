#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
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

void leaveHouse(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    if (current_screen_type == SCREEN_HOUSE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
}

// All three advisers move to these houses after the Walls hand-in. The route to
// that hand-in is covered by MM7LightWalls.LightRoute; this fixture starts at L5.
void seedAdvisors() {
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
    pNPCStats->pNPCData[80].house = HOUSE_CELESTE_HOSTEL_1;
    pNPCStats->pNPCData[81].house = HOUSE_CELESTE_HOSTEL_2;
    pNPCStats->pNPCData[82].house = HOUSE_CELESTE_HOSTEL_3;
}

}  // namespace

GAME_TEST(MM7LightAdvisors, AltarPiecesRequireBoth) {
    game.startNewGame();
    seedAdvisors();
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 420 && face.Clickable(); });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CELESTE_HOSTEL_1);
    ASSERT_EQ(houseNpcs.size(), 1);
    ASSERT_EQ(houseNpcs[0].npc, &pNPCStats->pNPCData[80]);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_1_evt_id, 147);
    const auto topics = prepareScriptedNPCDialogueTopics(&pNPCStats->pNPCData[80]);
    ASSERT_FALSE(topics.empty());
    ASSERT_EQ(topics.front(), DIALOGUE_SCRIPTED_LINE_1);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    leaveHouse(game);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_1_evt_id, 148);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    leaveHouse(game);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_DARK));
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_DARK));
}

GAME_TEST(MM7LightAdvisors, WineCellarRequiresVampireFlag) {
    game.startNewGame();
    seedAdvisors();
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 421 && face.Clickable(); });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CELESTE_HOSTEL_2);
    ASSERT_EQ(houseNpcs.size(), 1);
    ASSERT_EQ(houseNpcs[0].npc, &pNPCStats->pNPCData[81]);
    ASSERT_EQ(pNPCStats->pNPCData[81].dialogue_1_evt_id, 151);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_WINE_CELLAR_ACTIVE]);
    leaveHouse(game);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(pNPCStats->pNPCData[81].dialogue_1_evt_id, 152);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_WINE_CELLAR_FINISHED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WINE_CELLAR_ACTIVE]);
    leaveHouse(game);

    // The Wine Cellar exit can set this bit without an actual kill. Keep the
    // fixture explicit; this tests only the adviser hand-in, not combat.
    pParty->_questBits.set(QBIT_WINE_CELLAR_VAMPIRE_KILLED);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_WINE_CELLAR_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_WINE_CELLAR_ACTIVE]);
}

GAME_TEST(MM7LightAdvisors, SoulJarsRequireCase) {
    game.startNewGame();
    seedAdvisors();
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 422 && face.Clickable(); });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CELESTE_HOSTEL_3);
    ASSERT_EQ(houseNpcs.size(), 1);
    ASSERT_EQ(houseNpcs[0].npc, &pNPCStats->pNPCData[82]);
    ASSERT_EQ(pNPCStats->pNPCData[82].dialogue_1_evt_id, 153);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_ACTIVE]);
    leaveHouse(game);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(pNPCStats->pNPCData[82].dialogue_1_evt_id, 154);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_FINISHED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_ACTIVE]);
    leaveHouse(game);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_CASE_OF_SOUL_JARS));
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_CASE_OF_SOUL_JARS));
}
