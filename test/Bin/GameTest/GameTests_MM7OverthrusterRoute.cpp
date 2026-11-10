#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "Utility/Exception.h"

namespace {

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 3 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
}

template <typename Predicate>
bool interactWithLincolnEvent(EngineController &game, int eventId, Predicate done) {
    auto safeFloor = [](Vec3f position) {
        int sector = pIndoor->GetSector(position);
        int floorFace = -1;
        return sector != 0 &&
               BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius), sector, &floorFace) != -30000 &&
               floorFace != -1;
    };
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        if (std::abs(face.facePlane.normal.z) > 0.5f) {
            for (const auto &[offset, yaw] : std::array{
                     std::pair{Vec3f(300, 0, 50), 180}, std::pair{Vec3f(-300, 0, 50), 0},
                     std::pair{Vec3f(0, 300, 50), 270}, std::pair{Vec3f(0, -300, 50), 90}}) {
                Vec3f position = face.boundingBox.center() + offset;
                if (!safeFloor(position))
                    continue;
                game.teleportTo(MAP_LINCOLN, position, yaw, -45);
                try {
                    game.pointMouseAtFace(&face - pIndoor->faces.data());
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                    if (done())
                        return true;
                } catch (const Exception &) {
                }
            }
        }
        for (float distance : {150.0f, 300.0f, -150.0f, -300.0f}) {
            for (float height : {face.boundingBox.z1, face.boundingBox.center().z}) {
                Vec3f position = face.boundingBox.center() + distance * face.facePlane.normal;
                position.z = height;
                if (!safeFloor(position))
                    continue;
                int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
                if (distance < 0)
                    yaw += 180;
                game.teleportTo(MAP_LINCOLN, position, yaw);
                game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                game.tick();
                if (done())
                    return true;
                try {
                    game.pointMouseAtFace(&face - pIndoor->faces.data());
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                    if (done())
                        return true;
                } catch (const Exception &) {
                }
            }
        }
    }
    return false;
}

}  // namespace

GAME_TEST(MM7OverthrusterRoute, WetsuitBorder) {
    for (bool allSuited : {false, true}) {
        SCOPED_TRACE(fmt::format("allSuited={}", allSuited));
        test.prepareForNextTest();
        game.startNewGame();
        engine->config->debug.NoDamage.setValue(true);
        for (int i = 0; i < (allSuited ? 4 : 3); i++)
            pParty->pCharacters[i].inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
        game.teleportTo(MAP_AVLEE, Vec3f(-22200, 0, 1), 180);
        for (int i = 0; i < 4; i++)
            EXPECT_EQ(pParty->pCharacters[i].hasUnderwaterSuitEquipped(), allSuited || i < 3);
        auto mapTape = tapes.map();
        test.startTaping();

        game.pressKey(PlatformKey::KEY_UP);
        game.tick(12);
        game.releaseKey(PlatformKey::KEY_UP);
        if (allSuited) {
            ASSERT_EQ(current_screen_type, SCREEN_CHANGE_LOCATION);
            game.pressGuiButton("Transition_Yes");
            game.skipLoadingScreen();
            ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);
            EXPECT_EQ(mapTape, tape(MAP_AVLEE, MAP_SHOALS));
        } else {
            EXPECT_EQ(current_screen_type, SCREEN_GAME);
            EXPECT_EQ(engine->_currentLoadedMapId, MAP_AVLEE);
            EXPECT_EQ(mapTape, tape(MAP_AVLEE));
        }
        test.stopTaping();
    }
}

GAME_TEST(MM7OverthrusterRoute, QuestAndWetsuitChests) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_XENOFEX_QUEST_FINISHED); // The L8 hand-in is covered by the advisor lane.
    pNPCStats->pNPCData[80].house = HOUSE_CASTLE_LAMBENT_THRONE_ROOM;
    pNPCStats->pNPCData[80].dialogue_2_evt_id = 169;
    auto activeTape = tapes.questBit(QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE);
    auto wetsuitTape = tapes.questBit(QBIT_ORACLE_RECOVERS_WETSUIT);
    test.startTaping();

    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("House_Npc1");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_WETSUIT]);
    leaveHouse(game);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    for (int eventId : {182, 183}) {
        bool opened = false;
        for (const BLVFace &face : pIndoor->faces) {
            if (face.eventId != eventId || !face.Clickable())
                continue;
            auto [position, yaw] = approachFace(face);
            game.teleportTo(MAP_CASTLE_LAMBENT, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            if (current_screen_type == SCREEN_CHEST) {
                opened = true;
                break;
            }
        }
        ASSERT_TRUE(opened);
        ASSERT_EQ(current_screen_type, SCREEN_CHEST);
        for (int i = 0; i < 8; i++) {
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
        }
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
    }
    EXPECT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_WETSUIT]);
    int wetsuitsCollected = 0;
    for (const Character &character : pParty->pCharacters)
        for (InventoryConstEntry entry : character.inventory.entries())
            wetsuitsCollected += entry->itemId == ITEM_QUEST_WETSUIT;
    EXPECT_GE(wetsuitsCollected, 4);
    EXPECT_EQ(activeTape, tape(false, true));
    EXPECT_EQ(wetsuitTape, tape(false, true));
    test.stopTaping();
}

GAME_TEST(MM7OverthrusterRoute, ShoalsLincolnEntry) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    for (Character &character : pParty->pCharacters)
        character.inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
    game.teleportTo(MAP_SHOALS, Vec3f(), 0);
    auto mapTape = tapes.map();
    test.startTaping();

    bool opened = false;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.eventId != 501)
            continue;
        if (face.Clickable()) {
            auto [position, yaw] = approachFace(face);
            game.teleportTo(MAP_SHOALS, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
        } else {
            Vec3f position = face.boundingBox.center();
            position.z = face.boundingBox.z1 + 50;
            game.teleportTo(MAP_SHOALS, position, 0);
            game.tick(5);
        }
        if (current_screen_type == SCREEN_INPUT_BLV) {
            opened = true;
            break;
        }
    }
    ASSERT_TRUE(opened);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_LINCOLN);
    EXPECT_EQ(mapTape, tape(MAP_SHOALS, MAP_LINCOLN));
    test.stopTaping();
}

GAME_TEST(MM7OverthrusterRoute, LincolnRetrievalAndExit) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE); // L9 request is exercised above.
    for (int i = 0; i < 3; i++)
        pParty->pCharacters[i].inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
    auto takenTape = tapes.questBit(QBIT_OSCILLATION_OVERTHRUSTER_TAKEN);
    auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER);
    auto itemTape = tapes.custom([] {
        return pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER) ||
               pParty->pPickedItem.itemId == ITEM_QUEST_OSCILLATION_OVERTHRUSTER;
    });
    auto mapTape = tapes.map();

    game.teleportTo(MAP_LINCOLN, Vec3f(2100, -960, 219), 180); // Existing Lincoln plate-test anchor.
    test.startTaping();
    ASSERT_EQ(engine->_persistentVariables.mapVars[2], 0);
    ASSERT_FALSE(interactWithLincolnEvent(game, 376, [] {
        return pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN];
    }));
    ASSERT_FALSE(pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER));
    ASSERT_TRUE(interactWithLincolnEvent(game, 475, [] {
        return engine->_persistentVariables.mapVars[2] == 1;
    }));
    ASSERT_FALSE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN]);
    ASSERT_TRUE(interactWithLincolnEvent(game, 376, [] {
        return pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN];
    }));
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_OSCILLATION_OVERTHRUSTER);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER]);

    ASSERT_FALSE(pParty->pCharacters[3].hasUnderwaterSuitEquipped());
    ASSERT_FALSE(interactWithLincolnEvent(game, 501, [] {
        return current_screen_type == SCREEN_INPUT_BLV;
    }));
    EXPECT_EQ(engine->_currentLoadedMapId, MAP_LINCOLN);
    pParty->pCharacters[3].inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
    ASSERT_TRUE(interactWithLincolnEvent(game, 501, [] {
        return current_screen_type == SCREEN_INPUT_BLV;
    }));
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);
    EXPECT_EQ(mapTape, tape(MAP_LINCOLN, MAP_SHOALS));
    EXPECT_EQ(takenTape, tape(false, true));
    EXPECT_EQ(oracleTape, tape(false, true));
    EXPECT_EQ(itemTape, tape(false, true));
    test.stopTaping();
}
