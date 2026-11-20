#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtContext.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/Houses/Transport.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Exception.h"

namespace {

constexpr int falkNpcId = 47;
constexpr int offerEventId = 88;
constexpr int returnEventId = 89;
constexpr int tidewaterChestEventId = 176;
constexpr int tidewaterChestId = 1;
constexpr int tidewaterFallbackChestId = 0;
constexpr int tidewaterMapVariable = 2;
constexpr int rebeccaNpcId = 46;
constexpr int lightOfferEventId = 86;
constexpr int lightReturnEventId = 87;
constexpr int darkOfferEventId = 90;
constexpr int darkReturnEventId = 91;
constexpr int altarEventId = 376;
constexpr std::string_view priestCheckpointSlot = "saves/save000.mm7";
constexpr std::string_view priestCheckpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> priestCheckpointExperience = {3000, 3100, 3200, 3300};
constexpr int priestCheckpointGold = 4000;

struct SecondPriestPath {
    int npcId;
    MapId promoterMap;
    HouseId promoterHouse;
    int offerEventId;
    int returnEventId;
    QuestBit pathBit;
    QuestBit activeBit;
    MapId altarMap;
    QuestBit objectiveBit;
    Class promotedClass;
    AwardId qualifyingAward;
    AwardId honoraryAward;
    int greeting;
};

constexpr SecondPriestPath lightPriestPath = {
    rebeccaNpcId,
    MAP_CELESTE,
    HOUSE_CELESTE_HOUSE_DEVINE,
    lightOfferEventId,
    lightReturnEventId,
    QBIT_LIGHT_PATH,
    QBIT_TEMPLE_OF_THE_MOON_ALTAR_ACTIVE,
    MAP_GRAND_TEMPLE_OF_THE_MOON,
    QBIT_TEMPLE_OF_THE_MOON_ALTAR_PURIFIED,
    CLASS_PRIEST_OF_SUN,
    AWARD_PROMOTION_PRIEST_OF_LIGHT,
    AWARD_PROMOTION_PRIEST_OF_LIGHT_HONORARY,
    73,
};

constexpr SecondPriestPath darkPriestPath = {
    falkNpcId,
    MAP_DEYJA,
    HOUSE_DEYJA_FALK_RESIDENCE,
    darkOfferEventId,
    darkReturnEventId,
    QBIT_DARK_PATH,
    QBIT_TEMPLE_OF_THE_SUN_ALTAR_ACTIVE,
    MAP_GRAND_TEMPLE_OF_THE_SUN,
    QBIT_TEMPLE_OF_THE_SUN_ALTAR_DEFILED,
    CLASS_PRIEST_OF_MOON,
    AWARD_PROMOTION_PRIEST_OF_DARK,
    AWARD_PROMOTION_PRIEST_OF_DARK_HONORARY,
    75,
};

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool eventChangesVariable(const EvtProgram &program, int eventId, EvtOpcode opcode, EvtVariable variable,
                          int value) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
                      instruction.data.variable_descr.value == value;
           });
}

bool eventSetsNpcTopic(const EvtProgram &program, int eventId, int npcId, int index, int topic) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == npcId &&
                      instruction.data.npc_topic_descr.index == index &&
                      instruction.data.npc_topic_descr.event_id == topic;
           });
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    const std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                                 npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    const char *prefix = current_screen_type == SCREEN_NPC_DIALOGUE ?
                         "NpcDialogue_Option" : "HouseNpcDialogue_Option";
    game.pressGuiButton(fmt::format("{}{}", prefix, option - topics.begin()));
    game.tick();
    return true;
}

bool npcEventVisible(NPCData *npc, unsigned int eventId) {
    const std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                                 npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(npc);
    return std::ranges::find(topics, line) != topics.end();
}

void leaveDialogue(EngineController &game) {
    for (int attempt = 0; attempt < 5 && current_screen_type != SCREEN_GAME; ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openOutdoorHouse(EngineController &game, MapId map, HouseId houseId, NPCData *npc = nullptr) {
    game.teleportTo(map, Vec3f(), 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, houseId))
            doorIds.push_back(faceId);
    }
    ASSERT_FALSE(doorIds.empty());

    for (int faceId : doorIds) {
        const BLVFace &face = pOutdoor->faces[faceId];
        for (float direction : {1.0f, -1.0f}) {
            Vec3f position = face.boundingBox.center() + direction * 200 * face.facePlane.normal;
            bool onWater = false;
            int floorFaceId = -1;
            position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
            int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                             -direction * face.facePlane.normal.x) *
                                  180 / std::numbers::pi);
            game.teleportTo(map, position, yaw);
            try {
                game.pointMouseAtFace(faceId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)
                break;
        }
        if (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)
            break;
    }

    ASSERT_TRUE(current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE);
    if (current_screen_type == SCREEN_HOUSE) {
        ASSERT_NE(window_SpeakInHouse, nullptr);
        ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    }
    if (!npc)
        return;

    auto entry = std::ranges::find(houseNpcs, npc, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openIndoorHouse(EngineController &game, MapId map, Vec3f arrival, HouseId houseId, NPCData *npc = nullptr) {
    game.teleportTo(map, arrival, 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, houseId))
            doorIds.push_back(faceId);
    }
    ASSERT_FALSE(doorIds.empty());

    for (int faceId : doorIds) {
        const BLVFace &face = pIndoor->faces[faceId];
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
                game.teleportTo(map, position, yaw);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)
                    break;
            }
            if (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)
                break;
        }
        if (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)
            break;
    }

    ASSERT_TRUE(current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE);
    if (current_screen_type == SCREEN_HOUSE) {
        ASSERT_NE(window_SpeakInHouse, nullptr);
        ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    }
    if (!npc)
        return;

    auto entry = std::ranges::find(houseNpcs, npc, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openFalkDialogue(EngineController &game, NPCData *falk) {
    openOutdoorHouse(game, MAP_DEYJA, HOUSE_DEYJA_FALK_RESIDENCE, falk);
    ASSERT_EQ(falk->house, HOUSE_DEYJA_FALK_RESIDENCE);
}

void openPromoterDialogue(EngineController &game, const SecondPriestPath &path, NPCData *promoter) {
    if (path.promoterMap == MAP_CELESTE) {
        openIndoorHouse(game, MAP_CELESTE, Vec3f(-158, 7624, 1), path.promoterHouse, promoter);
    } else {
        openOutdoorHouse(game, path.promoterMap, path.promoterHouse, promoter);
    }
    ASSERT_EQ(promoter->house, path.promoterHouse);
}

void loadMapAtPartyStart(EngineController &game, MapId map) {
    if (engine->_currentLoadedMapId == map)
        return;
    game.runGameRoutine([map] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_SKIP_NEXT_WORLD_UPDATE;
        uGameState = GAME_STATE_CHANGE_LOCATION;
        onMapLeave();
    });
    game.tick();
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, map);
}

bool eventSetsQuestBit(const EvtProgram &program, int eventId, QuestBit bit) {
    return eventChangesVariable(program, eventId, EVENT_Set, VAR_QBits_QuestsDone, std::to_underlying(bit)) ||
           eventChangesVariable(program, eventId, EVENT_Add, VAR_QBits_QuestsDone, std::to_underlying(bit));
}

void useAltar(EngineController &game, const SecondPriestPath &path) {
    loadMapAtPartyStart(game, path.altarMap);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(altarEventId));
    ASSERT_TRUE(eventSetsQuestBit(engine->_localEventMap, altarEventId, path.objectiveBit));

    bool used = false;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces) && !used; ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (!face.Clickable() || face.eventId != altarEventId)
            continue;
        if (std::abs(face.facePlane.normal.z) > 0.5f) {
            for (const auto &[offset, yaw] : std::array{
                     std::pair{Vec3f(300, 0, 50), 180}, std::pair{Vec3f(-300, 0, 50), 0},
                     std::pair{Vec3f(0, 300, 50), 270}, std::pair{Vec3f(0, -300, 50), 90}}) {
                Vec3f position = face.boundingBox.center() + offset;
                int sectorId = 0;
                int floorFaceId = -1;
                float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
                if (floorZ == -30000 || floorFaceId == -1)
                    continue;
                position.z = floorZ;
                game.teleportTo(path.altarMap, position, yaw, -45);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                used = pParty->_questBits[path.objectiveBit];
                if (used)
                    break;
            }
            if (used)
                break;
        }
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
                game.teleportTo(path.altarMap, position, yaw);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                used = pParty->_questBits[path.objectiveBit];
                if (used)
                    break;
            }
            if (used)
                break;
        }
    }
    ASSERT_TRUE(used);
}

void prepareSecondPromotionParty() {
    const std::array classes = {CLASS_PRIEST, CLASS_PRIEST, CLASS_CLERIC, CLASS_KNIGHT};
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = classes[i];
        pParty->pCharacters[i]._achievedAwardsBits.set(i < 2 ? AWARD_PROMOTION_PRIEST :
                                                       AWARD_PROMOTION_PRIEST_HONORARY);
    }
}

bool bracadaBoatAvailable(EngineController &game) {
    openOutdoorHouse(game, MAP_BRACADA_DESERT, HOUSE_BOAT_BRACADA_DESERT);
    const bool result = isTravelAvailable(HOUSE_BOAT_BRACADA_DESERT);
    leaveDialogue(game);
    return result;
}

int openTidewaterQuestChest(EngineController &game) {
    if (engine->_currentLoadedMapId != MAP_TIDEWATER_CAVERNS)
        game.teleportTo(MAP_TIDEWATER_CAVERNS, Vec3f(), 0);
    bool opened = false;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces) && !opened; ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (!face.Clickable() || face.eventId != tidewaterChestEventId)
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
                game.teleportTo(MAP_TIDEWATER_CAVERNS, position, yaw);
                for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
                    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                    game.tick();
                }
                opened = current_screen_type == SCREEN_CHEST;
                if (opened)
                    break;
            }
            if (opened)
                break;
        }
    }
    EXPECT_TRUE(opened);
    return opened ? pGUIWindow_CurrentChest->chestId() : -1;
}

void closeChest(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void pickUpEvenmornMap(EngineController &game) {
    ASSERT_EQ(openTidewaterQuestChest(game), tidewaterChestId);
    ASSERT_LT(tidewaterChestId, std::ssize(vChests));
    ASSERT_TRUE(vChests[tidewaterChestId].inventory.find(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND));

    InventoryEntry entry = vChests[tidewaterChestId].inventory.find(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    Pointi itemPosition = chestTable[vChests[tidewaterChestId].chestTypeId].inventoryOffset +
                          entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND);
    closeChest(game);
}

void assertPriestDisposableProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void setPriestCheckpointBaseline() {
    pParty->SetGold(priestCheckpointGold);
    for (int i = 0; i < 4; ++i)
        pParty->pCharacters[i].experience = priestCheckpointExperience[i];
}

void writePriestCheckpoint(EngineController &game, std::string_view description) {
    ASSERT_FALSE(ufs->exists(priestCheckpointSlot));
    game.runGameRoutine([description] {
        saveGame(false, false, priestCheckpointSlot, description);
    });
    ASSERT_TRUE(ufs->exists(priestCheckpointSlot));
    const Blob saved = ufs->read(priestCheckpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

void loadPriestCheckpoint(EngineController &game) {
    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, priestCheckpointFileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, priestCheckpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, priestCheckpointFileName);
}

} // namespace

GAME_TEST(MM7PriestPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &falk = pNPCStats->pNPCData[falkNpcId];
    EXPECT_EQ(falk.house, HOUSE_DEYJA_FALK_RESIDENCE);
    EXPECT_EQ(falk.dialogue_1_evt_id, offerEventId);
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, EVENT_Set, VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_EVENMORN_MAP_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, falkNpcId, 0, returnEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, EVENT_Subtract, VAR_PlayerItemInHands,
                                     std::to_underlying(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, EVENT_Set, VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_EVENMORN_MAP_GIVEN)));

    game.teleportTo(MAP_TIDEWATER_CAVERNS, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(tidewaterChestEventId));
    const std::span<const EvtInstruction> source = engine->_localEventMap.function(tidewaterChestEventId);
    EXPECT_TRUE(std::ranges::any_of(source, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_MouseOver;
    }));
    EXPECT_TRUE(std::ranges::any_of(source, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == tidewaterChestId;
    }));
    EXPECT_TRUE(vChests[tidewaterChestId].inventory.find(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND));
}

GAME_TEST(MM7PriestPromotion, QuestChestReopensDuringSameVisit) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();

    ASSERT_EQ(openTidewaterQuestChest(game), tidewaterChestId);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_EVENMORN_MAP]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 1);
    ASSERT_TRUE(vChests[tidewaterChestId].inventory.find(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND));
    closeChest(game);

    EXPECT_EQ(openTidewaterQuestChest(game), tidewaterChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 1);
    EXPECT_TRUE(vChests[tidewaterChestId].inventory.find(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND));
    closeChest(game);
}

GAME_TEST(MM7PriestPromotion, SourceChestMarkerSurvivesSaveReload) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();

    ASSERT_EQ(openTidewaterQuestChest(game), tidewaterChestId);
    ASSERT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 1);
    closeChest(game);
    Blob checkpoint = game.saveGame();
    game.loadGame(checkpoint);
    game.tick(2);

    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TIDEWATER_CAVERNS);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_EVENMORN_MAP]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 1);
    EXPECT_EQ(openTidewaterQuestChest(game), tidewaterChestId);
    closeChest(game);
}

GAME_TEST(MM7PriestPromotion, FreshRecoveryBitUsesFallbackChest) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_ORACLE_RECOVERS_EVENMORN_MAP);

    ASSERT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 0);
    EXPECT_EQ(openTidewaterQuestChest(game), tidewaterFallbackChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 0);
    closeChest(game);
}

GAME_TEST(MM7PriestPromotion, PromotionAwardsUseFallbackChest) {
    for (AwardId award : {AWARD_PROMOTION_PRIEST, AWARD_PROMOTION_PRIEST_HONORARY}) {
        SCOPED_TRACE(fmt::format("award={}", std::to_underlying(award)));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        engine->config->debug.NoDamage.setValue(true);
        game.startNewGame();
        pParty->pCharacters[0]._achievedAwardsBits.set(award);

        ASSERT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 0);
        EXPECT_EQ(openTidewaterQuestChest(game), tidewaterFallbackChestId);
        EXPECT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 0);
        closeChest(game);
    }
}

GAME_TEST(MM7PriestPromotion, CompatibilitySeamIgnoresPatchedEventShape) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_TIDEWATER_CAVERNS, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(tidewaterChestEventId));

    EvtInstruction patchedExit(EVENT_Exit);
    patchedExit.step = 7;
    engine->_localEventMap.add(tidewaterChestEventId, patchedExit);
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_PRIEST);
    engine->_persistentVariables.mapVars[tidewaterMapVariable] = 1;
    vChests[tidewaterFallbackChestId].flags &= ~CHEST_TRAPPED;

    runEvent(EvtContext(EVT_SOURCE_MAP, tidewaterChestEventId));

    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), tidewaterFallbackChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 1);
    closeChest(game);
}

GAME_TEST(MM7PriestSecondPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &rebecca = pNPCStats->pNPCData[rebeccaNpcId];
    const NPCData &falk = pNPCStats->pNPCData[falkNpcId];
    EXPECT_EQ(rebecca.house, HOUSE_CELESTE_HOUSE_DEVINE);
    EXPECT_EQ(rebecca.dialogue_1_evt_id, lightOfferEventId);
    EXPECT_EQ(falk.house, HOUSE_DEYJA_FALK_RESIDENCE);
    EXPECT_EQ(falk.dialogue_1_evt_id, offerEventId);

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, lightOfferEventId, EVENT_Compare, VAR_Award,
                                     std::to_underlying(AWARD_PROMOTION_PRIEST)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, lightOfferEventId, EVENT_Compare, VAR_Award,
                                     std::to_underlying(AWARD_PROMOTION_PRIEST_HONORARY)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, lightOfferEventId, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_LIGHT_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, lightOfferEventId, EVENT_Set, VAR_QBits_QuestsDone,
                                     std::to_underlying(lightPriestPath.activeBit)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, lightOfferEventId, rebeccaNpcId, 0,
                                  lightReturnEventId));

    // Falk's dark offer is chained from the first promotion return instead of repeating the award comparison.
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, returnEventId, falkNpcId, 0, darkOfferEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, darkOfferEventId, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_DARK_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, darkOfferEventId, EVENT_Set, VAR_QBits_QuestsDone,
                                     std::to_underlying(darkPriestPath.activeBit)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, darkOfferEventId, falkNpcId, 0,
                                  darkReturnEventId));

    for (const SecondPriestPath &path : {lightPriestPath, darkPriestPath}) {
        EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, path.returnEventId, EVENT_Compare,
                                         VAR_QBits_QuestsDone, std::to_underlying(path.objectiveBit)));
        EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, path.returnEventId, EVENT_Subtract,
                                         VAR_QBits_QuestsDone, std::to_underlying(path.activeBit)));
        EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, path.returnEventId, EVENT_Add, VAR_FixedGold,
                                         10000));
        EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, path.returnEventId, EVENT_Subtract,
                                         VAR_ReputationInCurrentLocation, 10));
        EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, path.returnEventId, path.npcId, 0, 0));

        loadMapAtPartyStart(game, path.altarMap);
        ASSERT_TRUE(engine->_localEventMap.hasEvent(altarEventId));
        EXPECT_TRUE(eventChangesVariable(engine->_localEventMap, altarEventId, EVENT_Compare,
                                         VAR_QBits_QuestsDone, std::to_underlying(path.activeBit)));
        EXPECT_TRUE(eventSetsQuestBit(engine->_localEventMap, altarEventId, path.objectiveBit));
        EXPECT_TRUE(std::ranges::any_of(pIndoor->faces, [](const BLVFace &face) {
            return face.Clickable() && face.eventId == altarEventId;
        }));
    }
}

GAME_TEST(MM7PriestSecondPromotion, LightOfferRequiresPriestAwardAndLightPath) {
    game.startNewGame();
    NPCData &rebecca = pNPCStats->pNPCData[rebeccaNpcId];
    pParty->_questBits.set(QBIT_LIGHT_PATH);

    openPromoterDialogue(game, lightPriestPath, &rebecca);
    ASSERT_TRUE(selectNpcEvent(game, &rebecca, lightOfferEventId));
    EXPECT_FALSE(pParty->_questBits[lightPriestPath.activeBit]);
    EXPECT_EQ(rebecca.dialogue_1_evt_id, lightOfferEventId);
    leaveDialogue(game);

    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_PRIEST);
    pParty->_questBits.reset(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_DARK_PATH);
    openPromoterDialogue(game, lightPriestPath, &rebecca);
    ASSERT_TRUE(selectNpcEvent(game, &rebecca, lightOfferEventId));
    EXPECT_FALSE(pParty->_questBits[lightPriestPath.activeBit]);
    EXPECT_EQ(rebecca.dialogue_1_evt_id, lightOfferEventId);
    leaveDialogue(game);

    pParty->_questBits.reset(QBIT_DARK_PATH);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    openPromoterDialogue(game, lightPriestPath, &rebecca);
    ASSERT_TRUE(selectNpcEvent(game, &rebecca, lightOfferEventId));
    EXPECT_TRUE(pParty->_questBits[lightPriestPath.activeBit]);
    EXPECT_EQ(rebecca.dialogue_1_evt_id, lightReturnEventId);
    leaveDialogue(game);
}

GAME_TEST(MM7PriestSecondPromotion, DarkOfferRequiresFirstPromotionTopicAndDarkPath) {
    game.startNewGame();
    NPCData &falk = pNPCStats->pNPCData[falkNpcId];
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_PRIEST);

    openPromoterDialogue(game, darkPriestPath, &falk);
    EXPECT_FALSE(npcEventVisible(&falk, darkOfferEventId));
    EXPECT_FALSE(selectNpcEvent(game, &falk, darkOfferEventId));
    EXPECT_FALSE(pParty->_questBits[darkPriestPath.activeBit]);
    leaveDialogue(game);

    // Event 89 assigns this endpoint only after the physical Evenmorn map return succeeds.
    falk.dialogue_1_evt_id = darkOfferEventId;
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    openPromoterDialogue(game, darkPriestPath, &falk);
    ASSERT_TRUE(selectNpcEvent(game, &falk, darkOfferEventId));
    EXPECT_FALSE(pParty->_questBits[darkPriestPath.activeBit]);
    EXPECT_EQ(falk.dialogue_1_evt_id, darkOfferEventId);
    leaveDialogue(game);

    pParty->_questBits.reset(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_DARK_PATH);
    openPromoterDialogue(game, darkPriestPath, &falk);
    ASSERT_TRUE(selectNpcEvent(game, &falk, darkOfferEventId));
    EXPECT_TRUE(pParty->_questBits[darkPriestPath.activeBit]);
    EXPECT_EQ(falk.dialogue_1_evt_id, darkReturnEventId);
    leaveDialogue(game);
}

void runSecondPriestPromotion(EngineController &game, const SecondPriestPath &path) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    prepareSecondPromotionParty();
    pParty->_questBits.set(path.pathBit);
    NPCData &promoter = pNPCStats->pNPCData[path.npcId];
    if (path.npcId == falkNpcId)
        promoter.dialogue_1_evt_id = darkOfferEventId; // First promotion event 89 establishes this endpoint.

    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};

    openPromoterDialogue(game, path, &promoter);
    ASSERT_TRUE(selectNpcEvent(game, &promoter, path.offerEventId));
    ASSERT_TRUE(pParty->_questBits[path.activeBit]);
    ASSERT_EQ(promoter.dialogue_1_evt_id, path.returnEventId);

    const int goldBefore = pParty->GetGold();
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_FALSE(pParty->_questBits[path.objectiveBit]);
    ASSERT_TRUE(selectNpcEvent(game, &promoter, path.returnEventId));
    EXPECT_TRUE(pParty->_questBits[path.activeBit]);
    EXPECT_FALSE(pParty->_questBits[path.objectiveBit]);
    EXPECT_EQ(promoter.dialogue_1_evt_id, path.returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    useAltar(game, path);
    EXPECT_TRUE(pParty->_questBits[path.activeBit]);
    EXPECT_TRUE(pParty->_questBits[path.objectiveBit]);

    openPromoterDialogue(game, path, &promoter);
    const int goldBeforeReturn = pParty->GetGold();
    const int reputationBeforeReturn = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &promoter, path.returnEventId));
    EXPECT_FALSE(pParty->_questBits[path.activeBit]);
    EXPECT_TRUE(pParty->_questBits[path.objectiveBit]);
    EXPECT_EQ(promoter.dialogue_1_evt_id, 0u);
    EXPECT_EQ(promoter.greetingIndex, path.greeting);
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 10000);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, -10);

    for (int i = 0; i < 4; ++i) {
        const bool qualifying = classBefore[i] == CLASS_PRIEST;
        EXPECT_EQ(pParty->pCharacters[i].classType, qualifying ? path.promotedClass : classBefore[i]);
        auto expectedAwards = awardsBefore[i];
        expectedAwards.set(qualifying ? path.qualifyingAward : path.honoraryAward);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], qualifying ? 80000 : 40000);
    }

    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    const std::array classAfter = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                   pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const int goldAfter = pParty->GetGold();
    const int reputationAfter = pParty->GetPartyReputation();
    openPromoterDialogue(game, path, &promoter);
    EXPECT_FALSE(npcEventVisible(&promoter, path.returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &promoter, path.returnEventId));
    EXPECT_FALSE(pParty->_questBits[path.activeBit]);
    EXPECT_TRUE(pParty->_questBits[path.objectiveBit]);
    EXPECT_EQ(promoter.dialogue_1_evt_id, 0u);
    EXPECT_EQ(promoter.greetingIndex, path.greeting);
    EXPECT_EQ(pParty->GetGold(), goldAfter);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationAfter);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}

GAME_TEST(MM7PriestSecondPromotion, PriestOfLightUsesMoonAltarAndPromotes) {
    runSecondPriestPromotion(game, lightPriestPath);
}

GAME_TEST(MM7PriestSecondPromotion, PriestOfDarkUsesSunAltarAndPromotes) {
    runSecondPriestPromotion(game, darkPriestPath);
}

GAME_TEST(MM7PriestPromotion, MapReturnPromotesClericAndUnlocksEvenmornBoat) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->GetPlayingTime() += Duration::fromDays(1);
    game.tick();
    ASSERT_EQ(pParty->uCurrentDayOfMonth % 7, 1);

    NPCData &falk = pNPCStats->pNPCData[falkNpcId];
    pParty->pCharacters[0].classType = CLASS_CLERIC;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};
    const int goldBefore = pParty->GetGold();

    openFalkDialogue(game, &falk);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &falk, offerEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_EVENMORN_MAP_ACTIVE]);
    EXPECT_EQ(falk.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    leaveDialogue(game);

    EXPECT_FALSE(bracadaBoatAvailable(game));

    // Falk's return topic is available immediately, but refuses while the physical map is not held.
    openFalkDialogue(game, &falk);
    ASSERT_EQ(pParty->GetPartyReputation(), reputationBefore);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_TRUE(selectNpcEvent(game, &falk, returnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_EVENMORN_MAP_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_EVENMORN_MAP_GIVEN]);
    EXPECT_EQ(falk.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    pickUpEvenmornMap(game);
    pParty->_questBits.set(QBIT_ORACLE_RECOVERS_EVENMORN_MAP);
    openFalkDialogue(game, &falk);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND);
    const int goldBeforeReturn = pParty->GetGold();
    ASSERT_TRUE(selectNpcEvent(game, &falk, returnEventId));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(pParty->_questBits[QBIT_EVENMORN_MAP_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_EVENMORN_MAP_GIVEN]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_EVENMORN_MAP]);
    EXPECT_EQ(falk.dialogue_1_evt_id, 90u);
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 5000);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -5);

    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_PRIEST);
    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_PROMOTION_PRIEST);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBefore[0], 30000);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_PRIEST_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 15000);
    }

    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    const int goldAfter = pParty->GetGold();
    openFalkDialogue(game, &falk);
    EXPECT_FALSE(npcEventVisible(&falk, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &falk, returnEventId));
    EXPECT_EQ(pParty->GetGold(), goldAfter);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -5);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);

    ASSERT_EQ(pParty->uCurrentDayOfMonth % 7, 1);
    EXPECT_TRUE(bracadaBoatAvailable(game));
}

GAME_TEST(MM7PriestCheckpoint, DISABLED_WriteEvenmornMap) {
    assertPriestDisposableProfile();
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    setPriestCheckpointBaseline();
    pParty->pCharacters[0].classType = CLASS_CLERIC;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;

    NPCData &falk = pNPCStats->pNPCData[falkNpcId];
    openFalkDialogue(game, &falk);
    ASSERT_TRUE(selectNpcEvent(game, &falk, offerEventId));
    leaveDialogue(game);
    ASSERT_TRUE(pParty->_questBits[QBIT_EVENMORN_MAP_ACTIVE]);
    ASSERT_EQ(falk.dialogue_1_evt_id, returnEventId);
    pickUpEvenmornMap(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TIDEWATER_CAVERNS);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_EVENMORN_MAP]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 1);
    pParty->SetGold(priestCheckpointGold);
    ASSERT_FALSE(::testing::Test::HasFailure());
    writePriestCheckpoint(game, "Evenmorn map Priest checkpoint");
}

GAME_TEST(MM7PriestCheckpoint, DISABLED_ReloadEvenmornMapAndPromote) {
    assertPriestDisposableProfile();
    ASSERT_TRUE(ufs->exists(priestCheckpointSlot));
    const Blob saved = Blob::copy(ufs->read(priestCheckpointSlot));
    ASSERT_GT(saved.size(), 1024u);
    loadPriestCheckpoint(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TIDEWATER_CAVERNS);
    ASSERT_TRUE(pParty->_questBits[QBIT_EVENMORN_MAP_ACTIVE]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_EVENMORN_MAP]);
    ASSERT_FALSE(pParty->_questBits[QBIT_EVENMORN_MAP_GIVEN]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[tidewaterMapVariable], 1);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND);
    ASSERT_EQ(pParty->GetGold(), priestCheckpointGold);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, i == 0 ? CLASS_CLERIC : CLASS_KNIGHT);
        EXPECT_EQ(pParty->pCharacters[i].experience, priestCheckpointExperience[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_PRIEST]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_PRIEST_HONORARY]);
    }

    NPCData &falk = pNPCStats->pNPCData[falkNpcId];
    ASSERT_EQ(falk.dialogue_1_evt_id, returnEventId);
    openFalkDialogue(game, &falk);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &falk, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_EVENMORN_MAP_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_EVENMORN_MAP]);
    EXPECT_TRUE(pParty->_questBits[QBIT_EVENMORN_MAP_GIVEN]);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(falk.dialogue_1_evt_id, darkOfferEventId);
    EXPECT_EQ(pParty->GetGold(), priestCheckpointGold + 5000);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -5);
    for (int i = 0; i < 4; ++i) {
        const bool promoted = i == 0;
        EXPECT_EQ(pParty->pCharacters[i].classType, promoted ? CLASS_PRIEST : CLASS_KNIGHT);
        EXPECT_EQ(pParty->pCharacters[i].experience,
                  priestCheckpointExperience[i] + (promoted ? 30000 : 15000));
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_PRIEST], promoted);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_PRIEST_HONORARY], !promoted);
    }
    EXPECT_TRUE(ufs->read(priestCheckpointSlot).str() == saved.str());
}

GAME_TEST(MM7PriestCheckpoint, DISABLED_WriteMoonAltar) {
    assertPriestDisposableProfile();
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    setPriestCheckpointBaseline();
    prepareSecondPromotionParty();
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    NPCData &rebecca = pNPCStats->pNPCData[rebeccaNpcId];
    openPromoterDialogue(game, lightPriestPath, &rebecca);
    ASSERT_TRUE(selectNpcEvent(game, &rebecca, lightOfferEventId));
    leaveDialogue(game);
    ASSERT_TRUE(pParty->_questBits[lightPriestPath.activeBit]);
    ASSERT_EQ(rebecca.dialogue_1_evt_id, lightReturnEventId);
    useAltar(game, lightPriestPath);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_GRAND_TEMPLE_OF_THE_MOON);
    ASSERT_TRUE(pParty->_questBits[lightPriestPath.objectiveBit]);
    pParty->SetGold(priestCheckpointGold);
    ASSERT_FALSE(::testing::Test::HasFailure());
    writePriestCheckpoint(game, "Moon altar Priest of Light checkpoint");
}

GAME_TEST(MM7PriestCheckpoint, DISABLED_ReloadMoonAltarAndPromote) {
    assertPriestDisposableProfile();
    ASSERT_TRUE(ufs->exists(priestCheckpointSlot));
    const Blob saved = Blob::copy(ufs->read(priestCheckpointSlot));
    ASSERT_GT(saved.size(), 1024u);
    loadPriestCheckpoint(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_GRAND_TEMPLE_OF_THE_MOON);
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    ASSERT_TRUE(pParty->_questBits[lightPriestPath.activeBit]);
    ASSERT_TRUE(pParty->_questBits[lightPriestPath.objectiveBit]);
    ASSERT_EQ(pParty->GetGold(), priestCheckpointGold);
    const std::array classes = {CLASS_PRIEST, CLASS_PRIEST, CLASS_CLERIC, CLASS_KNIGHT};
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classes[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, priestCheckpointExperience[i]);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[i < 2 ? AWARD_PROMOTION_PRIEST :
                                                               AWARD_PROMOTION_PRIEST_HONORARY]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_PRIEST_OF_LIGHT]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_PRIEST_OF_LIGHT_HONORARY]);
    }

    NPCData &rebecca = pNPCStats->pNPCData[rebeccaNpcId];
    ASSERT_EQ(rebecca.dialogue_1_evt_id, lightReturnEventId);
    openPromoterDialogue(game, lightPriestPath, &rebecca);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &rebecca, lightReturnEventId));
    EXPECT_FALSE(pParty->_questBits[lightPriestPath.activeBit]);
    EXPECT_TRUE(pParty->_questBits[lightPriestPath.objectiveBit]);
    EXPECT_EQ(rebecca.dialogue_1_evt_id, 0u);
    EXPECT_EQ(rebecca.greetingIndex, lightPriestPath.greeting);
    EXPECT_EQ(pParty->GetGold(), priestCheckpointGold + 10000);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -10);
    for (int i = 0; i < 4; ++i) {
        const bool promoted = classes[i] == CLASS_PRIEST;
        EXPECT_EQ(pParty->pCharacters[i].classType, promoted ? CLASS_PRIEST_OF_SUN : classes[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience,
                  priestCheckpointExperience[i] + (promoted ? 80000 : 40000));
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[promoted ? AWARD_PROMOTION_PRIEST_OF_LIGHT :
                                                                AWARD_PROMOTION_PRIEST_OF_LIGHT_HONORARY]);
    }
    EXPECT_TRUE(ufs->read(priestCheckpointSlot).str() == saved.str());
}
