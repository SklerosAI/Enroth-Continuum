#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>
#include <optional>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "Utility/Exception.h"

static std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

template<class Done>
static bool clickEventFace(EngineController &game, MapId map, int eventId, int clicks, Done done) {
    // Clicks a face of the event from wherever the party can stand and see it, `clicks` times per spot, until `done`.
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        const int faceId = &face - pIndoor->faces.data();
        for (float distance : {150.0f, 250.0f, 350.0f}) {
            for (auto [direction, yaw] : {std::pair(Vec3f(1, 0, 0), 180), std::pair(Vec3f(-1, 0, 0), 0),
                                          std::pair(Vec3f(0, 1, 0), 270), std::pair(Vec3f(0, -1, 0), 90)}) {
                Vec3f position = face.boundingBox.center() + distance * direction;
                int sector = pIndoor->GetSector(position);
                int floorFace = -1;
                float floor = sector ? BLV_GetFloorLevel(position, sector, &floorFace) : -30000;
                if (floorFace == -1 || floor > face.boundingBox.z2 || floor < face.boundingBox.z1 - 200)
                    continue;
                position.z = floor;
                for (int pitch : {-15, -30, -45}) {
                    game.teleportTo(map, position, yaw, pitch);
                    try {
                        game.pointMouseAtFace(faceId);
                    } catch (const Exception &) {
                        continue;
                    }
                    for (int i = 0; i < clicks && !done(); i++) {
                        game.pressAndReleaseButton(BUTTON_LEFT);
                        game.tick();
                    }
                    if (done())
                        return true;
                }
            }
        }
    }
    return false;
}

static BLVDoor &doorById(int doorId) {
    auto result = std::ranges::find(pIndoor->doors, doorId, &BLVDoor::doorId);
    assert(result != pIndoor->doors.end());
    return *result;
}

static void openDoorWith(EngineController &game, MapId map, int eventId, int doorId) {
    ASSERT_TRUE(clickEventFace(game, map, eventId, 1, [doorId] { return doorById(doorId).state != DOOR_CLOSED; }));
    for (int i = 0; i < 100 && doorById(doorId).state != DOOR_OPEN; i++)
        game.tick();
    ASSERT_EQ(doorById(doorId).state, DOOR_OPEN);
}

static void lootChest(EngineController &game, MapId map, int eventId, ItemId itemId) {
    ASSERT_TRUE(clickEventFace(game, map, eventId, 2, [] { return current_screen_type == SCREEN_CHEST; })); // A trapped chest goes off on the first click.
    for (int i = 0; i < 40 && !pParty->hasItem(itemId) && pParty->pPickedItem.itemId == ITEM_NULL; i++) { // Space takes one item.
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasItem(itemId)) << "held item: " << static_cast<int>(pParty->pPickedItem.itemId);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static void enterAdvisorHouse(EngineController &game, int doorEventId, HouseId houseId, int npcId) {
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
    ASSERT_EQ(houseNpcs.size(), 1);
    ASSERT_EQ(houseNpcs[0].npc, &pNPCStats->pNPCData[npcId]);
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
    pNPCStats->pNPCData[81].house = HOUSE_CELESTE_HOSTEL_2;
    pNPCStats->pNPCData[82].house = HOUSE_CELESTE_HOSTEL_3;
}

static void toughenParty() {
    // Guards in the Walls of Mist, Temple of the Dark, Castle Gloaming and Wine Cellar are hostile to a Light party.
    for (Character &character : pParty->pCharacters) {
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character.uLevel = 200;
        character.health = character.GetMaxHealth();
    }
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
        toughenParty();
        game.teleportTo(MAP_WALLS_OF_MIST, Vec3f(-896, -4717, 161), 90); // Entrance from Celeste.
        auto westTape = tapes.hasItem(ITEM_WEST_PILLAR_KEY);
        auto centralTape = tapes.hasItem(ITEM_CENTRAL_PILLAR_KEY);
        auto eastTape = tapes.hasItem(ITEM_EAST_PILLAR_KEY);
        auto pedestalTape = tapes.custom([] { return static_cast<int>(engine->_persistentVariables.mapVars[18]); });
        auto leverTape = tapes.custom([] { return static_cast<int>(engine->_persistentVariables.mapVars[0]); });
        auto floorTape = tapes.doorStates({3, 4});
        auto exitTape = tapes.doorStates({1, 2});
        auto mapTape = tapes.map();
        auto screenTape = tapes.screen();
        test.startTaping();

        if (looted) {
            for (int lever : {5, 8})
                ASSERT_TRUE(clickEventFace(game, MAP_WALLS_OF_MIST, lever, 1, [lever] { return engine->_persistentVariables.mapVars[lever] == 1; }));
            openDoorWith(game, MAP_WALLS_OF_MIST, 10, 4); // With levers 5 and 8 pulled, the button slides away the floor over the first chest.
            ASSERT_FALSE(::testing::Test::HasFatalFailure());
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
            EXPECT_EQ(leverTape, tape(0, 1, 2, 0));
            EXPECT_EQ(floorTape, tape({DOOR_CLOSED, DOOR_CLOSED}, {DOOR_OPENING, DOOR_OPENING}, {DOOR_OPEN, DOOR_OPEN}));
            EXPECT_EQ(exitTape, tape({DOOR_CLOSED, DOOR_CLOSED}, {DOOR_OPENING, DOOR_OPENING}, {DOOR_OPEN, DOOR_OPEN}));
        } else {
            EXPECT_EQ(screenTape, tape(SCREEN_GAME));
            EXPECT_EQ(westTape, tape(false));
            EXPECT_EQ(centralTape, tape(false));
            EXPECT_EQ(eastTape, tape(false));
            EXPECT_EQ(pedestalTape, tape(0));
            EXPECT_EQ(leverTape, tape(0));
            EXPECT_EQ(floorTape, tape({DOOR_CLOSED, DOOR_CLOSED}));
            EXPECT_EQ(exitTape, tape({DOOR_CLOSED, DOOR_CLOSED}));
        }
        test.stopTaping();
    }
}

GAME_TEST(MM7LightItems, AltarPiecesFromTempleChests) {
    // The Light advisor takes the altar pieces once both are looted from the temple chests, and refuses just one.
    game.startNewGame();
    joinLightGuild();
    toughenParty();
    auto activeTape = tapes.questBit(QBIT_ALTAR_PIECES_LIGHT_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
    auto lightOracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_ALTAR_PIECE_LIGHT);
    auto darkOracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_ALTAR_PIECE_DARK);
    auto lightPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT);
    auto darkPieceTape = tapes.hasItem(ITEM_QUEST_ALTAR_PIECE_DARK);
    auto topicTape = tapes.custom([] { return pNPCStats->pNPCData[80].dialogue_1_evt_id; });
    auto mapTape = tapes.map();
    test.startTaping();

    enterAdvisorHouse(game, 420, HOUSE_CELESTE_HOSTEL_1, 80);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_ACTIVE]);
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_TEMPLE_OF_THE_LIGHT, Vec3f(-6784, -2832, 1649), 0); // Arrival point of the temple travel topic.
    lootChest(game, MAP_TEMPLE_OF_THE_LIGHT, 176, ITEM_QUEST_ALTAR_PIECE_LIGHT);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    enterAdvisorHouse(game, 420, HOUSE_CELESTE_HOSTEL_1, 80);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_TEMPLE_OF_THE_DARK, Vec3f(0, -4059, 513), 90); // Arrival point of the temple travel topic.
    openDoorWith(game, MAP_TEMPLE_OF_THE_DARK, 13, 17); // The altar's lid covers the chest.
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    lootChest(game, MAP_TEMPLE_OF_THE_DARK, 195, ITEM_QUEST_ALTAR_PIECE_DARK);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    enterAdvisorHouse(game, 420, HOUSE_CELESTE_HOSTEL_1, 80);
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
    toughenParty();
    auto activeTape = tapes.questBit(QBIT_SOUL_JARS_LIGHT_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_SOUL_JARS_LIGHT_FINISHED);
    auto takenTape = tapes.questBit(QBIT_CASTLE_GLOAMING_SOUL_JARS_TAKEN);
    auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_CASE_OF_SOUL_JARS);
    auto caseTape = tapes.hasItem(ITEM_QUEST_CASE_OF_SOUL_JARS);
    auto mapTape = tapes.map();
    test.startTaping();

    enterAdvisorHouse(game, 422, HOUSE_CELESTE_HOSTEL_3, 82);
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

    enterAdvisorHouse(game, 422, HOUSE_CELESTE_HOSTEL_3, 82);
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

GAME_TEST(MM7LightItems, WineCellarVampireQuest) {
    // The Light advisor closes the Wine Cellar quest after the party walks out of the cellar through its exit.
    game.startNewGame();
    joinLightGuild();
    toughenParty();
    auto activeTape = tapes.questBit(QBIT_WINE_CELLAR_ACTIVE);
    auto finishedTape = tapes.questBit(QBIT_WINE_CELLAR_FINISHED);
    auto killedTape = tapes.questBit(QBIT_WINE_CELLAR_VAMPIRE_KILLED);
    auto vampireTape = tapes.custom([] {
        return engine->_currentLoadedMapId == MAP_WINE_CELLAR ? std::optional(pActors[0].aiState) : std::nullopt; // Outside of it the actor list is another map's.
    });
    auto mapTape = tapes.map();
    test.startTaping();

    enterAdvisorHouse(game, 421, HOUSE_CELESTE_HOSTEL_2, 81);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_WINE_CELLAR_ACTIVE]);
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_WINE_CELLAR, Vec3f(601, -512, 1), 0); // Cellar entrance from Tatalia.
    ASSERT_EQ(pActors[0].monsterInfo.id, MONSTER_VAMPIRE_C); // The vampire the leave event checks. The map data has it disabled.
    auto exit = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 501 && face.Clickable();
    });
    ASSERT_NE(exit, pIndoor->faces.end());
    auto [exitPosition, exitYaw] = approachFace(*exit);
    game.teleportTo(MAP_WINE_CELLAR, exitPosition, exitYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TATALIA);

    enterAdvisorHouse(game, 421, HOUSE_CELESTE_HOSTEL_2, 81);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_WINE_CELLAR_FINISHED]);
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(finishedTape, tape(false, true));
    EXPECT_EQ(killedTape, tape(false, true));
    EXPECT_EQ(vampireTape, tape(std::optional<AIState>(), std::optional(Disabled), std::optional<AIState>()));
    EXPECT_EQ(mapTape, tape(MAP_EMERALD_ISLAND, MAP_CELESTE, MAP_WINE_CELLAR, MAP_TATALIA, MAP_CELESTE));
    test.stopTaping();
}
