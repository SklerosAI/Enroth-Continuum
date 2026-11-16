#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>

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
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/MonsterEnums.h"
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

constexpr int lanternChestEventId = 177;
constexpr int lanternChestId = 2;
constexpr int fallbackChestId = 0;
constexpr int lanternMapVariable = 2;
constexpr Vec3f ratSpawnPosition(5024, -224, -1376);
constexpr EvtVariable lanternMapVariableType = static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentVariable_0) + lanternMapVariable);
constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";

int pointAtLanternChestFace(EngineController &game) {
    if (engine->_currentLoadedMapId != MAP_BARROW_II)
        game.teleportTo(MAP_BARROW_II, Vec3f(), 0);

    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.eventId != lanternChestEventId)
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
                game.teleportTo(MAP_BARROW_II, position, yaw);
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

void clickLanternChestFace(EngineController &game) {
    ASSERT_NE(pointAtLanternChestFace(game), -1);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

void closeChest(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void pickUpLantern(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    InventoryEntry entry = vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    Pointi itemPosition = chestTable[vChests[lanternChestId].chestTypeId].inventoryOffset +
                          entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_LANTERN_OF_LIGHT);
}

int countPartyLanterns() {
    int result = pParty->pPickedItem.itemId == ITEM_QUEST_LANTERN_OF_LIGHT;
    for (const Character &character : pParty->pCharacters)
        for (InventoryConstEntry entry : character.inventory.entries())
            result += entry->itemId == ITEM_QUEST_LANTERN_OF_LIGHT;
    return result;
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void saveCheckpoint(EngineController &game) {
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Lantern of Light regional checkpoint");
    });
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

void reloadCheckpoint(EngineController &game, Blob &saved) {
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    saved = Blob::copy(ufs->read(checkpointSlot));
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
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
}

void grantLanternReturnAward() {
    for (Character &character : pParty->pCharacters)
        character._achievedAwardsBits.set(AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT);
}

void installExtendedOriginalLanternEvent() {
    engine->_localEventMap.clear();

    EvtInstruction hint(EVENT_MouseOver);
    hint.step = -1;
    hint.data.text_id = 3;
    engine->_localEventMap.add(lanternChestEventId, hint);

    EvtInstruction awardGate(EVENT_Compare);
    awardGate.step = 0;
    awardGate.target_step = 3;
    awardGate.data.variable_descr.type = VAR_Award;
    awardGate.data.variable_descr.value = std::to_underlying(AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT);
    engine->_localEventMap.add(lanternChestEventId, awardGate);

    EvtInstruction sourceChest(EVENT_OpenChest);
    sourceChest.step = 1;
    sourceChest.data.chest_id = lanternChestId;
    engine->_localEventMap.add(lanternChestEventId, sourceChest);

    EvtInstruction stop(EVENT_Exit);
    stop.step = 2;
    engine->_localEventMap.add(lanternChestEventId, stop);

    EvtInstruction fallbackChest(EVENT_OpenChest);
    fallbackChest.step = 3;
    fallbackChest.data.chest_id = fallbackChestId;
    engine->_localEventMap.add(lanternChestEventId, fallbackChest);

    EvtInstruction marker(EVENT_Set);
    marker.step = 4;
    marker.data.variable_descr.type = lanternMapVariableType;
    marker.data.variable_descr.value = 7;
    engine->_localEventMap.add(lanternChestEventId, marker);
}

} // namespace

GAME_TEST(MM7LanternOfLight, SourceContracts) {
    game.startNewGame();
    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(lanternChestEventId));
    const std::span<const EvtInstruction> event = engine->_localEventMap.function(lanternChestEventId);
    ASSERT_EQ(event.size(), 5u);
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == -1 && instruction.opcode == EVENT_MouseOver && instruction.data.text_id == 3;
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 3 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value ==
                   std::to_underlying(AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT);
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == lanternChestId;
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.opcode == EVENT_Exit;
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == fallbackChestId;
    }));
    EXPECT_FALSE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Compare || instruction.opcode == EVENT_Set ||
                instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == lanternMapVariableType;
    }));
    EXPECT_TRUE(vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT));
}

GAME_TEST(MM7LanternOfLight, TrapDoesNotPublishMarkerBeforeSourceOpens) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    vChests[lanternChestId].flags |= CHEST_TRAPPED;
    for (Character &character : pParty->pCharacters)
        character.setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(0, MASTERY_NONE));

    clickLanternChestFace(game);

    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_FALSE(vChests[lanternChestId].Trapped());
    EXPECT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 0);

    clickLanternChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 1);
    closeChest(game);
}

GAME_TEST(MM7LanternOfLight, SourceChestReopensAfterCloseWithoutPickup) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    vChests[lanternChestId].flags &= ~CHEST_TRAPPED;

    clickLanternChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    ASSERT_TRUE(vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT));
    EXPECT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 1);
    closeChest(game);

    clickLanternChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    EXPECT_TRUE(vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT));
    closeChest(game);
}

GAME_TEST(MM7LanternOfLight, MarkerSurvivesReloadAndAwardStillUsesSourceChest) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    vChests[lanternChestId].flags &= ~CHEST_TRAPPED;

    clickLanternChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    ASSERT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 1);
    closeChest(game);
    grantLanternReturnAward();

    game.loadGame(game.saveGame());

    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_II);
    ASSERT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 1);
    for (const Character &character : pParty->pCharacters)
        ASSERT_TRUE(character._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT]);
    clickLanternChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    EXPECT_TRUE(vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT));
    closeChest(game);
}

GAME_TEST(MM7LanternOfLight, FreshLegacyAwardUsesFallbackChest) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    grantLanternReturnAward();
    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    ASSERT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 0);
    vChests[fallbackChestId].flags &= ~CHEST_TRAPPED;

    clickLanternChestFace(game);

    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), fallbackChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 0);
    closeChest(game);
}

GAME_TEST(MM7LanternOfLight, ExtendedMapVariableShapeDisengagesCompatibilityGuard) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    grantLanternReturnAward();
    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    vChests[fallbackChestId].flags &= ~CHEST_TRAPPED;
    vChests[lanternChestId].flags &= ~CHEST_TRAPPED;
    installExtendedOriginalLanternEvent();
    engine->_persistentVariables.mapVars[lanternMapVariable] = 1;

    clickLanternChestFace(game);

    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), fallbackChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 7);
    closeChest(game);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteLanternOfLight) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_LANTERN_OF_LIGHT_ACTIVE);
    ASSERT_FALSE(pParty->_questBits[QBIT_LANTERN_OF_LIGHT_TAKEN]);

    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    vChests[lanternChestId].flags &= ~CHEST_TRAPPED;
    ASSERT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 0);
    clickLanternChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    ASSERT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 1);
    pickUpLantern(game);
    pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_EQ(countPartyLanterns(), 1);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_FALSE(vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT));
    closeChest(game);

    ASSERT_TRUE(pParty->_questBits[QBIT_LANTERN_OF_LIGHT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_LANTERN_OF_LIGHT_TAKEN]);
    for (const Character &character : pParty->pCharacters)
        ASSERT_FALSE(character._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT]);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_II);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    saveCheckpoint(game);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadLanternOfLight) {
    assertDisposableHeadlessProfile();
    Blob saved;
    reloadCheckpoint(game, saved);

    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_II);
    EXPECT_TRUE(pParty->_questBits[QBIT_LANTERN_OF_LIGHT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_LANTERN_OF_LIGHT_TAKEN]);
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[lanternMapVariable], 1);
    EXPECT_EQ(countPartyLanterns(), 1);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT));

    clickLanternChestFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), lanternChestId);
    EXPECT_FALSE(vChests[lanternChestId].inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT));
    closeChest(game);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}

GAME_TEST(MM7BarrowII, RatEncounterStartsOnValidIndoorFloor) {
    engine->config->debug.NoActors.setValue(false);
    game.startNewGame();
    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);

    constexpr std::array ratIds = {MONSTER_RAT_A, MONSTER_RAT_B, MONSTER_RAT_C};
    const auto isTargetRat = [&](const Actor &actor) {
        return actor.group == 5 && actor.initialPosition == ratSpawnPosition &&
               std::ranges::find(ratIds, actor.monsterInfo.id) != ratIds.end();
    };
    ASSERT_TRUE(std::ranges::any_of(pActors, isTargetRat));

    for (const Actor &actor : pActors) {
        if (!isTargetRat(actor))
            continue;
        ASSERT_NE(actor.sectorId, 0);
        EXPECT_EQ(pIndoor->GetSector(actor.pos), actor.sectorId);
        for (const Vec3f &offset : std::array{
                 Vec3f(), Vec3f(actor.radius, 0, 0), Vec3f(-actor.radius, 0, 0),
                 Vec3f(0, actor.radius, 0), Vec3f(0, -actor.radius, 0)}) {
            const Vec3f footprint = actor.pos + offset;
            int floorFaceId = -1;
            const int sectorId = pIndoor->GetSector(footprint);
            EXPECT_EQ(sectorId, actor.sectorId);
            EXPECT_NE(BLV_GetFloorLevel(footprint, sectorId, &floorFaceId), -30000);
            EXPECT_NE(floorFaceId, -1);
        }
    }
}
