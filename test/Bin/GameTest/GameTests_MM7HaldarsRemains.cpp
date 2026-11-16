#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ChestTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Exception.h"

namespace {

constexpr int remainsChestEventId = 176;
constexpr int remainsChestId = 1;
constexpr int fallbackChestId = 0;
constexpr int remainsMapVariable = 0;
constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";

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

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
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

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteHaldarsRemains) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_HALDARS_REMAINS_ACTIVE);
    game.teleportTo(MAP_MAZE, Vec3f(), 0);
    vChests[remainsChestId].flags |= CHEST_TRAPPED;
    for (Character &character : pParty->pCharacters)
        character.setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(0, MASTERY_NONE));

    clickRemainsChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_FALSE(vChests[remainsChestId].Trapped());
    ASSERT_FALSE(pParty->_questBits[QBIT_HALDARS_REMAINS_TAKEN]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 0);

    clickRemainsChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), remainsChestId);
    ASSERT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_TAKEN]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 1);
    pickUpRemains(game);
    pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_HALDARS_REMAINS));
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_FALSE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));
    closeChest(game);
    ASSERT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_ACTIVE]);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_MAZE);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());

    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Haldar's Remains regional checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadHaldarsRemains) {
    assertDisposableHeadlessProfile();
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = Blob::copy(ufs->read(checkpointSlot));
    ASSERT_GT(saved.size(), 1024u);

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, checkpointFileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, checkpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);

    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_MAZE);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[remainsMapVariable], 1);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_HALDARS_REMAINS));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));

    clickRemainsChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), remainsChestId);
    EXPECT_FALSE(vChests[remainsChestId].inventory.find(ITEM_QUEST_HALDARS_REMAINS));
    closeChest(game);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
