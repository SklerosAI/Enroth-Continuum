#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

namespace {

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool opensHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    const auto &function = engine->_localEventMap.function(eventId);
    return std::ranges::any_of(function, [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

void openAdvisorHouse(EngineController &game) {
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && opensHouse(face.eventId, HOUSE_CELESTE_HOSTEL_4);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CELESTE_HOSTEL_4);
}

}  // namespace

GAME_TEST(MM7LightAssassination, RobertDoorRequiresAdvisors) {
    for (bool allAdvisorsFinished : {false, true}) {
        SCOPED_TRACE(fmt::format("allAdvisorsFinished={}", allAdvisorsFinished));
        test.prepareForNextTest();
        game.startNewGame();
        engine->config->debug.NoDamage.setValue(true);
        pParty->_questBits.set(QBIT_LIGHT_PATH);
        pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
        pParty->_questBits.set(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
        pParty->_questBits.set(QBIT_WINE_CELLAR_FINISHED);
        if (allAdvisorsFinished)
            pParty->_questBits.set(QBIT_SOUL_JARS_LIGHT_FINISHED);
        auto movedTape = tapes.questBit(QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB);
        test.startTaping();

        game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
        auto robertDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
            return face.eventId == 376 && face.Clickable();
        });
        ASSERT_NE(robertDoor, pIndoor->faces.end());
        auto [position, yaw] = approachFace(*robertDoor);
        game.teleportTo(MAP_CELESTE, position, yaw);
        ASSERT_FALSE(pParty->_questBits[QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB]);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        EXPECT_EQ(pParty->_questBits[QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB], allAdvisorsFinished);
        EXPECT_EQ(movedTape, allAdvisorsFinished ? tape(false, true) : tape(false));
        test.stopTaping();
    }
}

GAME_TEST(MM7LightAssassination, RequestAndHandIn) {
    for (bool hasCube : {false, true}) {
        SCOPED_TRACE(fmt::format("hasCube={}", hasCube));
        test.prepareForNextTest();
        game.startNewGame();
        engine->config->debug.NoDamage.setValue(true);
        pParty->_questBits.set(QBIT_LIGHT_PATH);
        pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
        pParty->_questBits.set(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
        pParty->_questBits.set(QBIT_WINE_CELLAR_FINISHED);
        pParty->_questBits.set(QBIT_SOUL_JARS_LIGHT_FINISHED);
        pNPCStats->pNPCData[83].house = HOUSE_CELESTE_HOSTEL_4;
        auto activeTape = tapes.questBit(QBIT_TOLBERTI_ASSASSINATION_ACTIVE);
        auto finishedTape = tapes.questBit(QBIT_TOLBERTI_ASSASSINATED);
        test.startTaping();

        openAdvisorHouse(game);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
        auto advisor = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) {
            return npc.npc == &pNPCStats->pNPCData[83];
        });
        ASSERT_NE(advisor, houseNpcs.end());
        if (houseNpcs.size() > 1) {
            game.pressGuiButton(fmt::format("House_Npc{}", advisor - houseNpcs.begin() + 1));
            game.tick();
        }
        ASSERT_EQ(pNPCStats->pNPCData[83].dialogue_1_evt_id, 155);
        game.pressGuiButton("HouseNpcDialogue_Option0");
        game.tick();
        ASSERT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
        ASSERT_FALSE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);

        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.teleportTo(MAP_PIT, Vec3f(), 0);
        auto tolbertiDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
            return face.eventId == 376 && face.Clickable();
        });
        ASSERT_NE(tolbertiDoor, pIndoor->faces.end());
        auto [position, yaw] = approachFace(*tolbertiDoor);
        game.teleportTo(MAP_PIT, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
        ASSERT_EQ(engine->_currentLoadedMapId, MAP_SMALL_HOUSE);

        openAdvisorHouse(game);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());

        if (hasCube)
            pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_CONTROL_CUBE));
        game.pressGuiButton("HouseNpcDialogue_Option0");
        game.tick();
        EXPECT_EQ(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED], hasCube);
        EXPECT_EQ(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE], !hasCube);
        EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_CONTROL_CUBE));
        if (hasCube)
            EXPECT_EQ(pNPCStats->pNPCData[80].house, HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
        EXPECT_EQ(activeTape, hasCube ? tape(false, true, false) : tape(false, true));
        EXPECT_EQ(finishedTape, hasCube ? tape(false, true) : tape(false));
        test.stopTaping();
    }
}
