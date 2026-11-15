#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/ChestTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIChest.h"

#include "Utility/Exception.h"

namespace {

constexpr int remainsChestEventId = 176;
constexpr int remainsChestId = 1;
constexpr int fallbackChestId = 0;
constexpr int remainsMapVariable = 0;

int pointAtRemainsChestFace(EngineController &game) {
    if (engine->_currentLoadedMapId != MAP_MAZE)
        game.teleportTo(MAP_MAZE, Vec3f(), 0);

    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.eventId != remainsChestEventId)
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
                game.teleportTo(MAP_MAZE, position, yaw);
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

void clickRemainsChestFace(EngineController &game) {
    ASSERT_NE(pointAtRemainsChestFace(game), -1);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

void closeChest(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void pickUpRemains(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), remainsChestId);
    InventoryEntry entry = vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    Pointi itemPosition = chestTable[vChests[remainsChestId].chestTypeId].inventoryOffset +
                          entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_HALDARS_REMAINS);
}

} // namespace

GAME_TEST(MM7HaldarsRemains, SourceContracts) {
    game.startNewGame();
    game.teleportTo(MAP_MAZE, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(remainsChestEventId));
    const std::span<const EvtInstruction> event = engine->_localEventMap.function(remainsChestEventId);
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 5 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value == std::to_underlying(AWARD_NIGHON_RETRIEVED_HALDARS_REMAINS);
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 5 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HALDARS_REMAINS_TAKEN);
    }));
    EXPECT_FALSE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Compare || instruction.opcode == EVENT_Set ||
                instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_MapPersistentVariable_0;
    }));
    EXPECT_TRUE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));
}

GAME_TEST(MM7HaldarsRemains, TrapDoesNotPublishMarkerBeforeSourceOpens) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_HALDARS_REMAINS_ACTIVE);
    game.teleportTo(MAP_MAZE, Vec3f(), 0);
    vChests[remainsChestId].flags |= CHEST_TRAPPED;
    for (Character &character : pParty->pCharacters)
        character.setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(0, MASTERY_NONE));

    clickRemainsChestFace(game);

    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_FALSE(vChests[remainsChestId].Trapped());
    EXPECT_FALSE(pParty->_questBits[QBIT_HALDARS_REMAINS_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 0);

    clickRemainsChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), remainsChestId);
    EXPECT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 1);
    closeChest(game);
}

GAME_TEST(MM7HaldarsRemains, SourceChestReopensAfterCloseWithoutPickup) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_MAZE, Vec3f(), 0);
    vChests[remainsChestId].flags &= ~CHEST_TRAPPED;

    clickRemainsChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), remainsChestId);
    ASSERT_TRUE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));
    EXPECT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 1);
    closeChest(game);

    clickRemainsChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), remainsChestId);
    EXPECT_TRUE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));
    pickUpRemains(game);
    Item remains = pParty->takeHoldingItem();
    ASSERT_EQ(remains.itemId, ITEM_QUEST_HALDARS_REMAINS);
    EXPECT_FALSE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));
    closeChest(game);

    clickRemainsChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), remainsChestId);
    EXPECT_FALSE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));
    EXPECT_EQ(remains.itemId, ITEM_QUEST_HALDARS_REMAINS);
    closeChest(game);
}

GAME_TEST(MM7HaldarsRemains, LegacyTakenMarkerUsesFallbackChest) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_HALDARS_REMAINS_TAKEN);
    game.teleportTo(MAP_MAZE, Vec3f(), 0);
    ASSERT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 0);
    vChests[fallbackChestId].flags &= ~CHEST_TRAPPED;

    clickRemainsChestFace(game);

    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), fallbackChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 0);
    closeChest(game);
}
