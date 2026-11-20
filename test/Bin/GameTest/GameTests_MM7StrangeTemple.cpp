#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/mm7_data.h"

#include "Utility/Exception.h"

namespace {

constexpr int emptyBottleEventId = 454;
constexpr int exitEventId = 501;
constexpr Pointi firstPortrait(60, 423);

bool eventChangesVariable(const EvtProgram &program, int eventId, EvtOpcode opcode, EvtVariable variable,
                          int value) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
                      instruction.data.variable_descr.value == value;
           });
}

bool eventMovesToMap(const EvtProgram &program, int eventId, std::string_view fileName) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_MoveToMap && instruction.str == fileName;
           });
}

int pointAtEventFace(EngineController &game, int eventId) {
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (!face.Clickable() || face.eventId != eventId)
            continue;
        for (float distance : {80.0f, 150.0f, 250.0f}) {
            for (float direction : {1.0f, -1.0f}) {
                Vec3f position = face.boundingBox.center() + direction * distance * face.facePlane.normal;
                int sectorId = 0;
                int floorFaceId = -1;
                float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
                if (floorZ == -30000 || floorFaceId == -1)
                    continue;
                position.z = floorZ;
                int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                 -direction * face.facePlane.normal.x) *
                                      180 / std::numbers::pi);
                game.teleportTo(MAP_STRANGE_TEMPLE, position, yaw);
                try {
                    game.pointMouseAtFace(faceId);
                    return faceId;
                } catch (const Exception &) {
                }
            }
        }
    }
    return -1;
}

void useBottleOnPortrait(EngineController &game) {
    pParty->setHoldingItem(Item(ITEM_TEMPLE_IN_A_BOTTLE));
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_TEMPLE_IN_A_BOTTLE);
    ASSERT_EQ(pParty->pPickedItem.type(), ITEM_TYPE_MISC);
    game.pressButton(BUTTON_RIGHT, firstPortrait);
    for (int i = 0; i < 128 && (engine->_currentLoadedMapId != MAP_STRANGE_TEMPLE ||
                                uGameState != GAME_STATE_PLAYING); ++i)
        game.tick();
    game.releaseButton(BUTTON_RIGHT, firstPortrait);
    game.tick();
}

void leftClickPortrait(EngineController &game) {
    game.pressAndReleaseButton(BUTTON_LEFT, firstPortrait);
    game.tick();
}

}  // namespace

GAME_TEST(MM7StrangeTemple, SourceContracts) {
    game.startNewGame();
    EXPECT_EQ(pMapTable->GetMapInfo("nwc.blv"), MAP_STRANGE_TEMPLE);
    EXPECT_EQ(pMapTable->pInfos[MAP_STRANGE_TEMPLE].fileName, "nwc.blv");
    EXPECT_EQ(pMapTable->GetMapInfo("out02.odm"), MAP_HARMONDALE);

    game.teleportTo(MAP_STRANGE_TEMPLE, Vec3f(), 0);
    EXPECT_TRUE(eventChangesVariable(engine->_localEventMap, emptyBottleEventId, EVENT_Subtract,
                                     VAR_FixedGold, 10));
    EXPECT_TRUE(eventChangesVariable(engine->_localEventMap, emptyBottleEventId, EVENT_Add,
                                     VAR_PlayerItemInHands, std::to_underlying(ITEM_POTION_BOTTLE)));
    EXPECT_TRUE(eventMovesToMap(engine->_localEventMap, exitEventId, "out02.odm"));
    const auto &exit = engine->_localEventMap.function(exitEventId);
    auto move = std::ranges::find_if(exit, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_MoveToMap && instruction.str == "out02.odm";
    });
    ASSERT_NE(move, exit.end());
    // This shipped coordinate is beyond the outdoor boundary; keep the unusual source data visible
    // while native settled-position parity remains unresolved.
    EXPECT_GT(std::abs(move->data.move_map_descr.x), maxPartyAxisDistance);
    for (int eventId : {emptyBottleEventId, exitEventId}) {
        EXPECT_TRUE(std::ranges::any_of(pIndoor->faces, [eventId](const BLVFace &face) {
            return face.eventId == eventId && face.Clickable();
        }));
    }
}

GAME_TEST(MM7StrangeTemple, BottleEntryInteractionAndExit) {
    game.startNewGame();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);

    useBottleOnPortrait(game);
    ASSERT_EQ(uGameState, GAME_STATE_PLAYING);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_STRANGE_TEMPLE);
    ASSERT_EQ(pMapTable->pInfos[engine->_currentLoadedMapId].fileName, "nwc.blv");
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_TEMPLE_IN_A_BOTTLE);

    // Using the reusable bottle again while already inside is a no-op and does not consume it.
    game.pressButton(BUTTON_RIGHT, firstPortrait.x, firstPortrait.y);
    game.tick();
    game.releaseButton(BUTTON_RIGHT);
    game.tick();
    EXPECT_EQ(engine->_currentLoadedMapId, MAP_STRANGE_TEMPLE);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_TEMPLE_IN_A_BOTTLE);

    // Left-clicking the actual portrait now stows the reusable bottle before using event 454.
    leftClickPortrait(game);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_TRUE(pParty->hasItem(ITEM_TEMPLE_IN_A_BOTTLE));

    pParty->SetGold(100, true);
    ASSERT_NE(pointAtEventFace(game, emptyBottleEventId), -1);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    EXPECT_EQ(pParty->GetGold(), 90);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_POTION_BOTTLE);

    leftClickPortrait(game);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_TRUE(pParty->hasItem(ITEM_POTION_BOTTLE));
    ASSERT_TRUE(pParty->hasItem(ITEM_TEMPLE_IN_A_BOTTLE));

    ASSERT_NE(pointAtEventFace(game, exitEventId), -1);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    for (int i = 0; i < 128 && (engine->_currentLoadedMapId != MAP_HARMONDALE ||
                                uGameState != GAME_STATE_PLAYING); ++i)
        game.tick();
    EXPECT_EQ(uGameState, GAME_STATE_PLAYING);
    EXPECT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    EXPECT_EQ(pMapTable->pInfos[engine->_currentLoadedMapId].fileName, "out02.odm");
    EXPECT_LE(std::abs(pParty->pos.x), maxPartyAxisDistance);
    EXPECT_LE(std::abs(pParty->pos.y), maxPartyAxisDistance);
    EXPECT_TRUE(pParty->hasItem(ITEM_TEMPLE_IN_A_BOTTLE));
}
