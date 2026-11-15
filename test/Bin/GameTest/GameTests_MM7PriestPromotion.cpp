#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/Houses/Transport.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int falkNpcId = 47;
constexpr int offerEventId = 88;
constexpr int returnEventId = 89;
constexpr int tidewaterChestEventId = 176;
constexpr int tidewaterChestId = 1;

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

void openFalkDialogue(EngineController &game, NPCData *falk) {
    openOutdoorHouse(game, MAP_DEYJA, HOUSE_DEYJA_FALK_RESIDENCE, falk);
    ASSERT_EQ(falk->house, HOUSE_DEYJA_FALK_RESIDENCE);
}

bool bracadaBoatAvailable(EngineController &game) {
    openOutdoorHouse(game, MAP_BRACADA_DESERT, HOUSE_BOAT_BRACADA_DESERT);
    const bool result = isTravelAvailable(HOUSE_BOAT_BRACADA_DESERT);
    leaveDialogue(game);
    return result;
}

void pickUpEvenmornMap(EngineController &game) {
    game.teleportTo(MAP_TIDEWATER_CAVERNS, Vec3f(), 0);
    ASSERT_LT(tidewaterChestId, std::ssize(vChests));
    ASSERT_TRUE(vChests[tidewaterChestId].inventory.find(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND));

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
    ASSERT_TRUE(opened);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), tidewaterChestId);

    InventoryEntry entry = vChests[tidewaterChestId].inventory.find(ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    Pointi itemPosition = chestTable[vChests[tidewaterChestId].chestTypeId].inventoryOffset +
                          entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_MAP_TO_EVENMORN_ISLAND);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
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
