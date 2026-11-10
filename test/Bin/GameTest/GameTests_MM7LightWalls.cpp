#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"

namespace {

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

}  // namespace

GAME_TEST(MM7LightWalls, LightRoute) {
    // Light choice, city entry, the Walls quest, and its no-kill hand-in all run through their own interactions.
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    auto lightTape = tapes.questBit(QBIT_LIGHT_PATH);
    auto activeTape = tapes.questBit(QBIT_WALLS_OF_MIST_ACTIVE);
    auto passedTape = tapes.questBit(QBIT_WALLS_OF_MIST_PASSED);
    auto guildTape = tapes.questBit(QBIT_LIGHT_GUILD_JOINED);
    auto mapTape = tapes.map();
    pParty->_questBits.set(QBIT_ARBITER_MESSENGER_MET);
    pParty->_questBits.set(QBIT_ARBITER_CHOICE_ACTIVE);
    pNPCStats->pNPCData[77].house = HOUSE_TAVERN_BRACADA_DESERT;
    test.startTaping();

    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(-13180, 18432, 800), 0);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("House_Npc1"); // The Light judge.
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.teleportTo(MAP_HARMONDALE, Vec3f(20992, -6450, 672), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    ASSERT_FALSE(pParty->_questBits[QBIT_DARK_PATH]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(-13180, 18432, 800), 0);
    auto portal = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 503; });
    ASSERT_NE(portal, pOutdoor->faces.end());
    ASSERT_FALSE(portal->Clickable()); // This floor trigger opens a transition when the party steps onto it.
    Vec3f portalPosition = portal->boundingBox.center();
    portalPosition.z = portal->boundingBox.z1 + 50;
    game.teleportTo(MAP_BRACADA_DESERT, portalPosition, 0);
    game.tick(5);
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);

    auto castleDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 503 && face.Clickable();
    });
    ASSERT_NE(castleDoor, pIndoor->faces.end());
    auto [castlePosition, castleYaw] = approachFace(*castleDoor);
    game.teleportTo(MAP_CELESTE, castlePosition, castleYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_LAMBENT);
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90); // Existing Endings test door anchor.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    ASSERT_EQ(houseNpcs.size(), 1);
    ASSERT_EQ(houseNpcs[0].npc, &pNPCStats->pNPCData[79]);
    ASSERT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0); // Existing town-portal arrival anchor.
    auto wallsDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 502 && face.Clickable();
    });
    ASSERT_NE(wallsDoor, pIndoor->faces.end());
    auto [wallsPosition, wallsYaw] = approachFace(*wallsDoor);
    game.teleportTo(MAP_CELESTE, wallsPosition, wallsYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_WALLS_OF_MIST);
    ASSERT_FALSE(engine->config->debug.NoActors.value());
    ASSERT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);

    constexpr std::array keys = {ITEM_WEST_PILLAR_KEY, ITEM_CENTRAL_PILLAR_KEY, ITEM_EAST_PILLAR_KEY};
    constexpr std::array pedestalFaces = {164, 4019, 4075};
    constexpr std::array positions = {Vec3f(-1122, 3392, 1), Vec3f(-482, 3264, 1), Vec3f(158, 3392, 1)};
    for (ItemId key : keys)
        pParty->pCharacters[0].inventory.add(Item(key));
    for (int i = 0; i < 3; i++) {
        game.teleportTo(MAP_WALLS_OF_MIST, positions[i], 180, -22);
        game.pointMouseAtFace(pedestalFaces[i]);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick(2);
        ASSERT_FALSE(pParty->hasItem(keys[i]));
    }
    game.tick(20);
    auto exitDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 502 && face.Clickable();
    });
    ASSERT_NE(exitDoor, pIndoor->faces.end());
    auto [exitPosition, exitYaw] = approachFace(*exitDoor);
    game.teleportTo(MAP_WALLS_OF_MIST, exitPosition, exitYaw);
    ASSERT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]); // The exit event runs before transition acceptance.
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);

    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(pNPCStats->pNPCData[79].dialogue_1_evt_id, 146);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_GUILD_JOINED]);
    EXPECT_EQ(lightTape, tape(false, true));
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(passedTape, tape(false, true));
    EXPECT_EQ(guildTape, tape(false, true));
    EXPECT_CONTAINS(mapTape, MAP_CELESTE);
    EXPECT_CONTAINS(mapTape, MAP_WALLS_OF_MIST);
}

GAME_TEST(MM7LightWalls, ThroneClosedAtDawn) {
    // A Light-path party reaches the Lambent throne door before its daytime opening hour.
    engine->config->debug.NoActors.setValue(true); // No monster can interrupt the wait.
    game.startNewGame();
    pParty->_questBits.set(QBIT_LIGHT_PATH); // The alignment route is exercised by LightRoute.
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_REST);
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    for (int i = 0; i < 200 && currentRestType == REST_WAIT; i++)
        game.tick();
    ASSERT_EQ(currentRestType, REST_NONE);
    ASSERT_EQ(pParty->GetPlayingTime().toCivilTime().hour, 5);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
}
