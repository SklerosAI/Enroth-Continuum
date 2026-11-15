#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "Utility/Exception.h"
#include "Utility/String/Ascii.h"

namespace {

constexpr int entranceEventId = 101;
constexpr int exitEventId = 100;
constexpr int doorEventId = 4;
constexpr int doorId = 4;
constexpr ItemId floorTile = ITEM_QUEST_FLOOR_TILE_W_MOON_INSIGNIA;

const EvtInstruction *moveToMapInstruction(const EvtProgram &program, int eventId) {
    if (!program.hasEvent(eventId))
        return nullptr;
    auto instruction = std::ranges::find(program.function(eventId), EVENT_MoveToMap, &EvtInstruction::opcode);
    return instruction == program.function(eventId).end() ? nullptr : &*instruction;
}

void assertTransitionShape(const EvtProgram &program, int eventId, std::string_view destination,
                           const Vec3i &position, int yaw, HouseId house, int exitPicture) {
    ASSERT_TRUE(program.hasEvent(eventId));
    const auto &instructions = program.function(eventId);
    ASSERT_EQ(instructions.size(), 3u);
    EXPECT_EQ(instructions[0].step, -1);
    EXPECT_EQ(instructions[0].opcode, EVENT_MouseOver);
    EXPECT_EQ(instructions[1].step, 0);
    EXPECT_EQ(instructions[1].opcode, EVENT_MoveToMap);
    EXPECT_EQ(instructions[2].step, 1);
    EXPECT_EQ(instructions[2].opcode, EVENT_Exit);

    const EvtInstruction *move = moveToMapInstruction(program, eventId);
    ASSERT_NE(move, nullptr);
    EXPECT_TRUE(ascii::noCaseEquals(move->str, destination));
    EXPECT_EQ(move->data.move_map_descr.x, position.x);
    EXPECT_EQ(move->data.move_map_descr.y, position.y);
    EXPECT_EQ(move->data.move_map_descr.z, position.z);
    EXPECT_EQ(move->data.move_map_descr.yaw, yaw);
    EXPECT_EQ(move->data.move_map_descr.pitch, 0);
    EXPECT_EQ(move->data.move_map_descr.zspeed, 0);
    EXPECT_EQ(move->data.move_map_descr.house_id, house);
    EXPECT_EQ(move->data.move_map_descr.exit_pic_id, exitPicture);
}

bool clickOutdoorEventFace(EngineController &game, MapId map, int eventId) {
    // Setup teleport: event100's authored Emerald Island arrival stands directly before this entrance.
    game.teleportTo(map, Vec3f(15816, 12161, 1133), 0);
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (!face.Clickable() || face.eventId != eventId)
            continue;
        for (int pitch : {-10, 0, 10}) {
            game.teleportTo(map, Vec3f(15816, 12161, 1133), 0, pitch);
            try {
                game.pointMouseAtFace(faceId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            for (int frame = 0; frame < 5 && current_screen_type != SCREEN_INPUT_BLV; ++frame)
                game.tick();
            if (current_screen_type == SCREEN_INPUT_BLV)
                return true;
        }
    }
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (!face.Clickable() || face.eventId != eventId)
            continue;
        for (float distance : {150.0f, 250.0f, 350.0f}) {
            for (float direction : {1.0f, -1.0f}) {
                Vec3f position = face.boundingBox.center() + direction * distance * face.facePlane.normal;
                bool onWater = false;
                int floorFaceId = -1;
                position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
                if (floorFaceId == -1)
                    continue;
                int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                 -direction * face.facePlane.normal.x) *
                                      180 / std::numbers::pi);
                game.teleportTo(map, position, yaw); // Setup teleport to the authored viewport face.
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                for (int frame = 0; frame < 5 && current_screen_type != SCREEN_INPUT_BLV; ++frame)
                    game.tick();
                if (current_screen_type == SCREEN_INPUT_BLV)
                    return true;
            }
        }
    }
    return false;
}

template<class Done>
bool clickIndoorEventFace(EngineController &game, MapId map, int eventId, Done done) {
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (!face.Clickable() || face.eventId != eventId)
            continue;
        for (float distance : {150.0f, 250.0f, 350.0f, -150.0f, -250.0f}) {
            Vec3f position = face.boundingBox.center() + distance * face.facePlane.normal;
            int sectorId = pIndoor->GetSector(position);
            int floorFaceId = -1;
            float floor = sectorId ? BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius),
                                                       sectorId, &floorFaceId) : -30000;
            if (floor == -30000 || floorFaceId == -1)
                continue;
            position.z = floor;
            int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) *
                                  180 / std::numbers::pi);
            if (distance < 0)
                yaw += 180;
            for (int pitch : {-20, 0, 20}) {
                game.teleportTo(map, position, yaw, pitch); // Setup teleport to the authored interior face.
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (done())
                    return true;
            }
        }
    }
    return false;
}

void acceptTransition(EngineController &game, MapId destination) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, destination);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

int floorTileSprite() {
    auto sprite = std::ranges::find_if(pSpriteObjects, [](const SpriteObject &object) {
        return object.uObjectDescID != 0 && object.containing_item.itemId == floorTile;
    });
    return sprite == pSpriteObjects.end() ? -1 : sprite - pSpriteObjects.begin();
}

int liveFloorTileSprites() {
    return std::ranges::count_if(pSpriteObjects, [](const SpriteObject &object) {
        return object.uObjectDescID != 0 && object.containing_item.itemId == floorTile;
    });
}

bool partyHasFloorTile() {
    return pParty->hasItem(floorTile) || pParty->pPickedItem.itemId == floorTile;
}

bool pickUpFloorTile(EngineController &game, int spriteId) {
    const Vec3f itemPosition = pSpriteObjects[spriteId].vPosition;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(0, -200, 0), 90}, std::pair{Vec3f(-200, 0, 0), 0},
             std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(200, 0, 0), 180}}) {
        Vec3f position = itemPosition + offset;
        int sectorId = pIndoor->GetSector(position);
        int floorFaceId = -1;
        float floor = sectorId ? BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius),
                                                   sectorId, &floorFaceId) : -30000;
        if (floor == -30000 || floorFaceId == -1)
            continue;
        position.z = floor;
        game.teleportTo(MAP_TEMPLE_OF_THE_MOON, position, yaw, -10); // Setup teleport beside item636.
        try {
            game.pointMouseAtSprite(spriteId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        if (partyHasFloorTile())
            return true;
    }
    return false;
}

BLVDoor *findDoor(int id) {
    auto door = std::ranges::find(pIndoor->doors, id, &BLVDoor::doorId);
    return door == pIndoor->doors.end() ? nullptr : &*door;
}

void enterTemple(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    ASSERT_TRUE(clickOutdoorEventFace(game, MAP_EMERALD_ISLAND, entranceEventId));
    acceptTransition(game, MAP_TEMPLE_OF_THE_MOON);
}

void exitTemple(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TEMPLE_OF_THE_MOON);
    ASSERT_TRUE(clickIndoorEventFace(game, MAP_TEMPLE_OF_THE_MOON, exitEventId, [] {
        return current_screen_type == SCREEN_INPUT_BLV;
    }));
    acceptTransition(game, MAP_EMERALD_ISLAND);
}

}  // namespace

GAME_TEST(MM7TempleOfMoon, PhysicalContestItemRoute) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    ASSERT_TRUE(ascii::noCaseEquals(pMapTable->pInfos[MAP_TEMPLE_OF_THE_MOON].fileName, "d06.blv"));
    assertTransitionShape(engine->_localEventMap, entranceEventId, "D06.blv",
                          Vec3i(-1208, -4225, 366), 320,
                          HOUSE_EMERALD_ISLAND_TEMPLE_OF_THE_MOON, 3);
    EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_FLOOR_TILE_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_FLOOR_TILE_GIVEN]);
    ASSERT_FALSE(partyHasFloorTile());

    enterTemple(game);
    assertTransitionShape(engine->_localEventMap, exitEventId, "Out01.Odm",
                          Vec3i(15816, 12161, 1133), 1024, HOUSE_INVALID, 4);

    ASSERT_EQ(liveFloorTileSprites(), 1);
    const int spriteId = floorTileSprite();
    ASSERT_GE(spriteId, 0);
    SpriteObject &tile = pSpriteObjects[spriteId];
    EXPECT_EQ(tile.spriteId, static_cast<SpriteId>(113));
    EXPECT_EQ(tile.uObjectDescID, 114);
    EXPECT_EQ(tile.vPosition, Vec3f(-4160, 6336, -95));
    EXPECT_EQ(tile.uSectorID, 26);
    EXPECT_EQ(tile.vVelocity, Vec3f());
    EXPECT_FALSE(tile.IsUnpickable());

    ASSERT_TRUE(pickUpFloorTile(game, spriteId));
    EXPECT_EQ(pSpriteObjects[spriteId].uObjectDescID, 0);
    EXPECT_EQ(liveFloorTileSprites(), 0);
    EXPECT_TRUE(partyHasFloorTile());
    EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_FLOOR_TILE_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_FLOOR_TILE_GIVEN]);

    ASSERT_TRUE(engine->_localEventMap.hasEvent(doorEventId));
    const auto &doorEvent = engine->_localEventMap.function(doorEventId);
    auto doorChange = std::ranges::find(doorEvent, EVENT_ChangeDoorState, &EvtInstruction::opcode);
    ASSERT_NE(doorChange, doorEvent.end());
    EXPECT_EQ(doorChange->data.door_descr.door_id, doorId);
    EXPECT_EQ(doorChange->data.door_descr.door_action, DOOR_ACTION_OPEN);
    BLVDoor *door = findDoor(doorId);
    ASSERT_NE(door, nullptr);
    ASSERT_EQ(door->state, DOOR_CLOSED);
    ASSERT_TRUE(clickIndoorEventFace(game, MAP_TEMPLE_OF_THE_MOON, doorEventId, [door] {
        return door->state != DOOR_CLOSED;
    }));
    for (int frame = 0; frame < 100 && door->state != DOOR_OPEN; ++frame)
        game.tick();
    EXPECT_EQ(door->state, DOOR_OPEN);
    EXPECT_TRUE(partyHasFloorTile());

    exitTemple(game);
    EXPECT_TRUE(partyHasFloorTile());
    EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_FLOOR_TILE_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_FLOOR_TILE_GIVEN]);

    // Immediate map reload preserves the physically removed source, preventing a second contest tile.
    enterTemple(game);
    EXPECT_EQ(liveFloorTileSprites(), 0);
    EXPECT_TRUE(partyHasFloorTile());
    exitTemple(game);
    EXPECT_TRUE(partyHasFloorTile());
}
