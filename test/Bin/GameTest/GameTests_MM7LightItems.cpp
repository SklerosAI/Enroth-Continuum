#include <algorithm>
#include <array>
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
#include "GUI/UI/UIHouses.h"

static std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

static void openChest(EngineController &game, MapId map, int eventId) {
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable() || std::abs(face.facePlane.normal.z) > 0.5f)
            continue;
        auto [position, yaw] = approachFace(face);
        game.teleportTo(map, position, yaw);
        for (int i = 0; i < 2 && current_screen_type != SCREEN_CHEST; i++) { // A trapped chest goes off on the first press.
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
        }
        if (current_screen_type == SCREEN_CHEST)
            return;
    }
}

static void lootChest(EngineController &game, MapId map, int eventId, ItemId itemId) {
    openChest(game, map, eventId);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (int i = 0; i < 40 && !pParty->hasItem(itemId) && pParty->pPickedItem.itemId != itemId; i++) { // Space takes one item.
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasItem(itemId) || pParty->pPickedItem.itemId == itemId);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static void enterAdvisorHouse(EngineController &game, int doorEventId, HouseId houseId) {
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0); // Town portal arrival point.
    auto door = std::ranges::find_if(pIndoor->faces, [doorEventId](const BLVFace &face) {
        return face.eventId == doorEventId && face.Clickable();
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
}

static void leaveHouse(EngineController &game) {
    for (int i = 0; i < 2 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static void joinLightGuild() {
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
    pNPCStats->pNPCData[80].house = HOUSE_CELESTE_HOSTEL_1;
    pNPCStats->pNPCData[82].house = HOUSE_CELESTE_HOSTEL_3;
}

GAME_TEST(MM7LightItems, WallsPillarKeysFromChests) {
    // The Walls of Mist pillar keys lie in three chests, and the exit opens once all three are on their pedestals.
    constexpr std::array keys = {ITEM_WEST_PILLAR_KEY, ITEM_CENTRAL_PILLAR_KEY, ITEM_EAST_PILLAR_KEY};
    constexpr std::array chestEvents = {177, 178, 179};
    constexpr std::array pedestalFaces = {164, 4019, 4075};
    constexpr std::array pedestalPositions = {Vec3f(-1122, 3392, 1), Vec3f(-482, 3264, 1), Vec3f(158, 3392, 1)};
    for (bool looted : {false, true}) {
        SCOPED_TRACE(fmt::format("looted={}", looted));
        test.prepareForNextTest();
        game.startNewGame();
        game.teleportTo(MAP_WALLS_OF_MIST, Vec3f(-896, -4717, 161), 90); // Entrance from Celeste.
        auto westTape = tapes.hasItem(ITEM_WEST_PILLAR_KEY);
        auto centralTape = tapes.hasItem(ITEM_CENTRAL_PILLAR_KEY);
        auto eastTape = tapes.hasItem(ITEM_EAST_PILLAR_KEY);
        auto pedestalTape = tapes.custom([] { return engine->_persistentVariables.mapVars[18]; });
        auto exitTape = tapes.doorStates({1, 2});
        auto mapTape = tapes.map();
        auto screenTape = tapes.screen();
        test.startTaping();

        if (looted) {
            for (int i = 0; i < 3; i++) {
                lootChest(game, MAP_WALLS_OF_MIST, chestEvents[i], keys[i]);
                ASSERT_FALSE(::testing::Test::HasFatalFailure());
            }
        }
        for (int i = 0; i < 3; i++) {
            game.teleportTo(MAP_WALLS_OF_MIST, pedestalPositions[i], 180, -22); // Looking down at the pedestal.
            game.pointMouseAtFace(pedestalFaces[i]);
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
        }
        game.tick(20); // The exit doors slide open.

        EXPECT_EQ(mapTape, tape(MAP_WALLS_OF_MIST));
        if (looted) {
            EXPECT_CONTAINS(screenTape, SCREEN_CHEST);
            EXPECT_EQ(westTape, tape(false, true, false));
            EXPECT_EQ(centralTape, tape(false, true, false));
            EXPECT_EQ(eastTape, tape(false, true, false));
            EXPECT_EQ(pedestalTape, tape(0, 1, 2, 3));
            EXPECT_EQ(exitTape, tape({DOOR_CLOSED, DOOR_CLOSED}, {DOOR_OPENING, DOOR_OPENING}, {DOOR_OPEN, DOOR_OPEN}));
        } else {
            EXPECT_EQ(screenTape, tape(SCREEN_GAME));
            EXPECT_EQ(westTape, tape(false));
            EXPECT_EQ(centralTape, tape(false));
            EXPECT_EQ(eastTape, tape(false));
            EXPECT_EQ(pedestalTape, tape(0));
            EXPECT_EQ(exitTape, tape({DOOR_CLOSED, DOOR_CLOSED}));
        }
        test.stopTaping();
    }
}

GAME_TEST(MM7LightItems, AltarPiecesFromTempleChests) {
    // The Light advisor takes the altar pieces once both are looted from the temple chests, and refuses just one.
    game.startNewGame();
    joinLightGuild();
    auto activeTape = tapes.questBit(QBIT_ALTAR_PIECES_LIGHT_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
    auto lightOracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_ALTAR_PIECE_LIGHT);
    auto darkOracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_ALTAR_PIECE_DARK);
    auto lightPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT);
    auto darkPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_DARK);
    auto topicTape = tapes.custom([] { return pNPCStats->pNPCData[80].dialogue_1_evt_id; });
    auto mapTape = tapes.map();
    test.startTaping();

    enterAdvisorHouse(game, 420, HOUSE_CELESTE_HOSTEL_1);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_ACTIVE]);
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_TEMPLE_OF_THE_LIGHT, Vec3f(-6784, -2832, 1649), 0); // Arrival point of the temple travel topic.
    lootChest(game, MAP_TEMPLE_OF_THE_LIGHT, 176, ITEM_QUEST_ALTAR_PIECE_LIGHT);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    enterAdvisorHouse(game, 420, HOUSE_CELESTE_HOSTEL_1);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_TEMPLE_OF_THE_DARK, Vec3f(0, -4059, 513), 90); // Arrival point of the temple travel topic.
    lootChest(game, MAP_TEMPLE_OF_THE_DARK, 195, ITEM_QUEST_ALTAR_PIECE_DARK);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    enterAdvisorHouse(game, 420, HOUSE_CELESTE_HOSTEL_1);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(lightOracleTape, tape(false, true, false));
    EXPECT_EQ(darkOracleTape, tape(false, true, false));
    EXPECT_EQ(lightPieceTape, tape(false, true, false));
    EXPECT_EQ(darkPieceTape, tape(false, true, false));
    EXPECT_EQ(topicTape, tape(147u, 148u, 0u));
    EXPECT_EQ(mapTape, tape(MAP_EMERALD_ISLAND, MAP_CELESTE, MAP_TEMPLE_OF_THE_LIGHT, MAP_CELESTE, MAP_TEMPLE_OF_THE_DARK, MAP_CELESTE));
    test.stopTaping();
}

GAME_TEST(MM7LightItems, SoulJarsFromGloamingChest) {
    // The Light advisor takes the case of soul jars looted from the Castle Gloaming chest, and refuses without it.
    game.startNewGame();
    joinLightGuild();
    auto activeTape = tapes.questBit(QBIT_SOUL_JARS_LIGHT_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_SOUL_JARS_LIGHT_FINISHED);
    auto takenTape = tapes.questBit(QBIT_CASTLE_GLOAMING_SOUL_JARS_TAKEN);
    auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_CASE_OF_SOUL_JARS);
    auto caseTape = tapes.hasItem(ITEM_QUEST_CASE_OF_SOUL_JARS);
    auto mapTape = tapes.map();
    test.startTaping();

    enterAdvisorHouse(game, 422, HOUSE_CELESTE_HOSTEL_3);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_ACTIVE]);
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(pNPCStats->pNPCData[82].dialogue_1_evt_id, 154);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_FINISHED]);
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_CASTLE_GLOAMING, Vec3f(96, 3424, 1), 90); // Castle entrance from the Pit.
    lootChest(game, MAP_CASTLE_GLOAMING, 176, ITEM_QUEST_CASE_OF_SOUL_JARS);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    enterAdvisorHouse(game, 422, HOUSE_CELESTE_HOSTEL_3);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_FINISHED]);
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(takenTape, tape(false, true));
    EXPECT_EQ(oracleTape, tape(false, true, false));
    EXPECT_EQ(caseTape, tape(false, true, false));
    EXPECT_EQ(mapTape, tape(MAP_EMERALD_ISLAND, MAP_CELESTE, MAP_CASTLE_GLOAMING, MAP_CELESTE));
    test.stopTaping();
}
