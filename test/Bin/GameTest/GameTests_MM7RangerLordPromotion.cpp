#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int lysanderNpcId = 44;
constexpr int oldestTreeNpcId = 53;
constexpr int offerEventId = 77;
constexpr int returnEventId = 78;
constexpr int treeOfferEventId = 84;
constexpr int treeReturnEventId = 85;
constexpr int treePressureEventId = 402;
constexpr int heartChestEventId = 178;
constexpr int heartChestId = 3;
constexpr int lysanderGreetingId = 65;
constexpr int treeGreetingId = 71;
constexpr int angryForestGroupId = 10;

bool eventChangesVariable(const EvtProgram &program, int eventId, int step, int targetStep,
                          EvtOpcode opcode, EvtVariable variable, int value) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.step == step && instruction.target_step == targetStep &&
                      instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
                      instruction.data.variable_descr.value == value;
           });
}

int countVariableInstruction(const EvtProgram &program, int eventId, EvtOpcode opcode,
                             EvtVariable variable, int value) {
    if (!program.hasEvent(eventId))
        return 0;
    return std::ranges::count_if(program.function(eventId), [=](const EvtInstruction &instruction) {
        return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
               instruction.data.variable_descr.value == value;
    });
}

bool eventSetsNpcTopic(const EvtProgram &program, int eventId, int step, int npcId, int index, int topic) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.step == step && instruction.opcode == EVENT_SetNPCTopic &&
                      instruction.data.npc_topic_descr.npc_id == npcId &&
                      instruction.data.npc_topic_descr.index == index &&
                      instruction.data.npc_topic_descr.event_id == topic;
           });
}

bool eventSetsNpcGreeting(const EvtProgram &program, int eventId, int step, int npcId, int greeting) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.step == step && instruction.opcode == EVENT_SetNPCGreeting &&
                      instruction.data.npc_descr.npc_id == npcId &&
                      instruction.data.npc_descr.greeting == greeting;
           });
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool eventSpeaksNpc(int eventId, int npcId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakNPC && instruction.data.npc_descr.npc_id == npcId;
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

void openLysanderDialogue(EngineController &game, NPCData *lysander) {
    // Setup teleport: the house door and all following interaction are physical.
    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(), 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_BRACADA_DESERT_SWEET_RESIDENCE))
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
            game.teleportTo(MAP_BRACADA_DESERT, position, yaw);
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
        ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_BRACADA_DESERT_SWEET_RESIDENCE);
    }
    auto entry = std::ranges::find(houseNpcs, lysander, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openOldestTreeDialogue(EngineController &game) {
    // Setup teleport: entering the Oldest Tree's authored pressure face below performs the physical interaction.
    game.teleportTo(MAP_TULAREAN_FOREST, Vec3f(), 0);
    auto face = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &candidate) {
        return candidate.eventId == treePressureEventId && candidate.attributes & FACE_PRESSURE_PLATE;
    });
    ASSERT_NE(face, pOutdoor->faces.end());
    ASSERT_TRUE(eventSpeaksNpc(treePressureEventId, oldestTreeNpcId));
    Vec3f position = face->boundingBox.center();
    position.z = face->boundingBox.z1 + 1;
    game.teleportTo(MAP_TULAREAN_FOREST, position, 0);
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
}

void acquireHeartFromChest(EngineController &game) {
    game.teleportTo(MAP_MERCENARY_GUILD, Vec3f(), 0);
    ASSERT_TRUE(vChests[heartChestId].inventory.find(ITEM_QUEST_HEART_OF_THE_WOOD));
    // Keep this promotion test focused on the authored chest transaction rather than trap lethality.
    pParty->activeCharacter().setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(30, MASTERY_MASTER));
    bool opened = false;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces) && !opened; ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (!face.Clickable() || face.eventId != heartChestEventId)
            continue;
        for (float distance : {80.0f, 150.0f, 250.0f}) {
            for (float direction : {1.0f, -1.0f}) {
                Vec3f position = face.boundingBox.center() + direction * distance * face.facePlane.normal;
                int sectorId = 0;
                int floorFaceId = -1;
                float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius),
                                               &sectorId, &floorFaceId);
                if (floorZ == -30000 || floorFaceId == -1)
                    continue;
                position.z = floorZ;
                int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                 -direction * face.facePlane.normal.x) *
                                      180 / std::numbers::pi);
                game.teleportTo(MAP_MERCENARY_GUILD, position, yaw);
                try {
                    game.pointMouseAtFace(faceId);
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                } catch (const Exception &) {
                }
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
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), heartChestId);
    for (int attempt = 0; attempt < 40 && !pParty->hasItem(ITEM_QUEST_HEART_OF_THE_WOOD); ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_HEART_OF_THE_WOOD));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void prepareHunterOutcome(AwardId prerequisite, bool lightPath, bool darkPath) {
    const std::array classes = {CLASS_HUNTER, CLASS_KNIGHT, CLASS_HUNTER, CLASS_SORCERER};
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = classes[i];
        if (classes[i] == CLASS_HUNTER)
            pParty->pCharacters[i]._achievedAwardsBits.set(AWARD_PROMOTION_HUNTER);
        else
            pParty->pCharacters[i]._achievedAwardsBits.set(AWARD_PROMOTION_HUNTER_HONORARY);
    }
    pParty->pCharacters[0]._achievedAwardsBits.reset(AWARD_PROMOTION_HUNTER);
    pParty->pCharacters[0]._achievedAwardsBits.reset(AWARD_PROMOTION_HUNTER_HONORARY);
    if (prerequisite != AWARD_INVALID)
        pParty->pCharacters[0]._achievedAwardsBits.set(prerequisite);
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
}

bool hasGoldMutation(const EvtInstruction &instruction) {
    if (instruction.opcode != EVENT_Add && instruction.opcode != EVENT_Set &&
        instruction.opcode != EVENT_Subtract)
        return false;
    return instruction.data.variable_descr.type == VAR_FixedGold ||
           instruction.data.variable_descr.type == VAR_RandomGold;
}

} // namespace

GAME_TEST(MM7RangerLordPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &lysander = pNPCStats->pNPCData[lysanderNpcId];
    const NPCData &oldestTree = pNPCStats->pNPCData[oldestTreeNpcId];
    EXPECT_EQ(lysander.house, HOUSE_BRACADA_DESERT_SWEET_RESIDENCE);
    EXPECT_EQ(lysander.dialogue_1_evt_id, offerEventId);
    EXPECT_EQ(oldestTree.house, HOUSE_INVALID);
    EXPECT_EQ(oldestTree.dialogue_1_evt_id, treeOfferEventId);

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 0, 4, EVENT_Compare,
                                     VAR_Award, std::to_underlying(AWARD_PROMOTION_HUNTER)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 1, 4, EVENT_Compare,
                                     VAR_Award, std::to_underlying(AWARD_PROMOTION_HUNTER_HONORARY)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 4, 10, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_LIGHT_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 5, 8, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_DARK_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 11, 0, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_CALM_TULAREAN_TREES_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, 12, lysanderNpcId, 0, returnEventId));

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, 0, 6, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HEART_OF_THE_WOOD_RETURNED)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, 1, 4, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_TALKED_TO_OLDEST_TREE)));
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Compare,
                                       VAR_Class, std::to_underlying(CLASS_HUNTER)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Set,
                                       VAR_Class, std::to_underlying(CLASS_RANGER_LORD)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add,
                                       VAR_Award, std::to_underlying(AWARD_PROMOTION_RANGER_LORD)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add,
                                       VAR_Award, std::to_underlying(AWARD_PROMOTION_RANGER_LORD_HONORARY)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add, VAR_Experience, 80000), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add, VAR_Experience, 40000), 4);
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, 39, 0, EVENT_Subtract,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_CALM_TULAREAN_TREES_ACTIVE)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, 40, 0, EVENT_Subtract,
                                     VAR_ReputationInCurrentLocation, 10));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, returnEventId, 42, lysanderNpcId, 0, 0));
    EXPECT_TRUE(eventSetsNpcGreeting(engine->_globalEventMap, returnEventId, 43, lysanderNpcId,
                                    lysanderGreetingId));
    EXPECT_TRUE(std::ranges::none_of(engine->_globalEventMap.function(returnEventId), hasGoldMutation));

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeOfferEventId, 0, 3,
                                     EVENT_OnCanShowDialogItemCmp, VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_CALM_TULAREAN_TREES_ACTIVE)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeOfferEventId, 6, 0, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HEART_OF_THE_WOOD_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, treeOfferEventId, 7, oldestTreeNpcId, 0,
                                 treeReturnEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeReturnEventId, 1, 4, EVENT_Compare,
                                     VAR_PlayerItemInHands, std::to_underlying(ITEM_QUEST_HEART_OF_THE_WOOD)));
    ASSERT_EQ(engine->_globalEventMap.instruction(treeReturnEventId, 0).opcode, EVENT_ForPartyMember);
    EXPECT_EQ(engine->_globalEventMap.instruction(treeReturnEventId, 0).who, CHOOSE_PARTY);
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeReturnEventId, 5, 0, EVENT_Subtract,
                                     VAR_PlayerItemInHands, std::to_underlying(ITEM_QUEST_HEART_OF_THE_WOOD)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeReturnEventId, 6, 0, EVENT_Subtract,
                                     VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_ORACLE_RECOVERS_HEART_OF_THE_WOOD)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeReturnEventId, 7, 0, EVENT_Add,
                                     VAR_Experience, 5000));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeReturnEventId, 8, 0, EVENT_Subtract,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HEART_OF_THE_WOOD_ACTIVE)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeReturnEventId, 9, 0, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HEART_OF_THE_WOOD_RETURNED)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, treeReturnEventId, 10, oldestTreeNpcId, 0, 0));
    EXPECT_TRUE(eventSetsNpcGreeting(engine->_globalEventMap, treeReturnEventId, 11, oldestTreeNpcId,
                                    treeGreetingId));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, treeReturnEventId, 12, 0, EVENT_Set,
                                     VAR_Award, std::to_underlying(AWARD_AVLEE_RETRIEVED_FOREST_HEART)));
    EXPECT_TRUE(std::ranges::any_of(engine->_globalEventMap.function(treeReturnEventId),
                                   [](const EvtInstruction &instruction) {
        return instruction.step == 13 && instruction.opcode == EVENT_ToggleActorGroupFlag &&
               instruction.data.actor_flag_descr.id == angryForestGroupId &&
               instruction.data.actor_flag_descr.attr == ACTOR_AGGRESSOR &&
               !instruction.data.actor_flag_descr.is_set;
    }));

    game.teleportTo(MAP_TULAREAN_FOREST, Vec3f(), 0);
    EXPECT_TRUE(eventSpeaksNpc(treePressureEventId, oldestTreeNpcId));
    EXPECT_TRUE(std::ranges::any_of(pOutdoor->faces, [](const BLVFace &face) {
        return face.eventId == treePressureEventId && face.attributes & FACE_PRESSURE_PLATE;
    }));

    game.teleportTo(MAP_MERCENARY_GUILD, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(heartChestEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_localEventMap, heartChestEventId, 0, 3, EVENT_Compare,
                                     VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_ORACLE_RECOVERS_HEART_OF_THE_WOOD)));
    EXPECT_TRUE(std::ranges::any_of(engine->_localEventMap.function(heartChestEventId),
                                   [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == heartChestId;
    }));
    EXPECT_TRUE(eventChangesVariable(engine->_localEventMap, heartChestEventId, 2, 0, EVENT_Add,
                                     VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_ORACLE_RECOVERS_HEART_OF_THE_WOOD)));
    ASSERT_LT(heartChestId, std::ssize(vChests));
    EXPECT_TRUE(vChests[heartChestId].inventory.find(ITEM_QUEST_HEART_OF_THE_WOOD));
    EXPECT_TRUE(std::ranges::any_of(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && face.eventId == heartChestEventId;
    }));
}

GAME_TEST(MM7RangerLordPromotion, OfferRequiresHunterAwardAndLightPath) {
    for (const auto &[prerequisite, lightPath, darkPath, accepted] : std::array{
             std::tuple{AWARD_INVALID, true, false, false},
             std::tuple{AWARD_PROMOTION_HUNTER, false, false, false},
             std::tuple{AWARD_PROMOTION_HUNTER, false, true, false},
             std::tuple{AWARD_PROMOTION_HUNTER_HONORARY, true, false, true}}) {
        SCOPED_TRACE(fmt::format("award={}, light={}, dark={}", std::to_underlying(prerequisite),
                                 lightPath, darkPath));
        test.prepareForNextTest();
        game.startNewGame();
        prepareHunterOutcome(prerequisite, lightPath, darkPath);
        NPCData &lysander = pNPCStats->pNPCData[lysanderNpcId];
        const int goldBefore = pParty->GetGold();

        openLysanderDialogue(game, &lysander);
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &lysander, offerEventId));
        EXPECT_EQ(pParty->_questBits[QBIT_CALM_TULAREAN_TREES_ACTIVE], accepted);
        EXPECT_EQ(lysander.dialogue_1_evt_id, accepted ? returnEventId : offerEventId);
        EXPECT_FALSE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_RETURNED]);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }
}

GAME_TEST(MM7RangerLordPromotion, PhysicalHeartReturnPromotesOnce) {
    game.startNewGame();
    prepareHunterOutcome(AWARD_PROMOTION_HUNTER, true, false);
    NPCData &lysander = pNPCStats->pNPCData[lysanderNpcId];
    NPCData &oldestTree = pNPCStats->pNPCData[oldestTreeNpcId];
    const std::array initialClasses = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                       pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array initialExperience = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                          pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array initialAwards = {pParty->pCharacters[0]._achievedAwardsBits,
                                      pParty->pCharacters[1]._achievedAwardsBits,
                                      pParty->pCharacters[2]._achievedAwardsBits,
                                      pParty->pCharacters[3]._achievedAwardsBits};
    const int initialGold = pParty->GetGold();

    openLysanderDialogue(game, &lysander);
    ASSERT_TRUE(selectNpcEvent(game, &lysander, offerEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_CALM_TULAREAN_TREES_ACTIVE]);
    ASSERT_EQ(lysander.dialogue_1_evt_id, returnEventId);
    ASSERT_TRUE(selectNpcEvent(game, &lysander, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_RETURNED]);
    EXPECT_EQ(lysander.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), initialGold);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, initialExperience[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, initialAwards[i]);
    }
    leaveDialogue(game);

    openOldestTreeDialogue(game);
    ASSERT_TRUE(npcEventVisible(&oldestTree, treeOfferEventId));
    ASSERT_TRUE(selectNpcEvent(game, &oldestTree, treeOfferEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_ACTIVE]);
    ASSERT_EQ(oldestTree.dialogue_1_evt_id, treeReturnEventId);
    ASSERT_TRUE(selectNpcEvent(game, &oldestTree, treeReturnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_RETURNED]);
    EXPECT_EQ(oldestTree.dialogue_1_evt_id, treeReturnEventId);
    EXPECT_EQ(pParty->GetGold(), initialGold);
    leaveDialogue(game);

    // Setup teleport: acquisition still uses the real event-178 face, chest UI, and player item transfer.
    acquireHeartFromChest(game);
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_HEART_OF_THE_WOOD));
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_HEART_OF_THE_WOOD]);
    EXPECT_EQ(pParty->GetGold(), initialGold);

    openOldestTreeDialogue(game);
    ASSERT_TRUE(selectNpcEvent(game, &oldestTree, treeReturnEventId));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_HEART_OF_THE_WOOD));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_HEART_OF_THE_WOOD]);
    EXPECT_FALSE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_RETURNED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_TALKED_TO_OLDEST_TREE]);
    EXPECT_EQ(oldestTree.dialogue_1_evt_id, 0u);
    EXPECT_EQ(oldestTree.greetingIndex, treeGreetingId);
    EXPECT_EQ(pParty->GetGold(), initialGold);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, initialExperience[i] + 5000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_AVLEE_RETRIEVED_FOREST_HEART]);
    }
    ASSERT_TRUE(std::ranges::any_of(pActors, [](const Actor &actor) {
        return actor.group == angryForestGroupId;
    }));
    EXPECT_TRUE(std::ranges::none_of(pActors, [](const Actor &actor) {
        return actor.group == angryForestGroupId && actor.attributes & ACTOR_AGGRESSOR;
    }));
    leaveDialogue(game);

    openOldestTreeDialogue(game);
    EXPECT_FALSE(npcEventVisible(&oldestTree, treeReturnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &oldestTree, treeReturnEventId));
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, initialExperience[i] + 5000);
    EXPECT_TRUE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_RETURNED]);
    leaveDialogue(game);

    const std::array experienceBeforePromotion = {
        pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsBeforePromotion = {pParty->pCharacters[0]._achievedAwardsBits,
                                              pParty->pCharacters[1]._achievedAwardsBits,
                                              pParty->pCharacters[2]._achievedAwardsBits,
                                              pParty->pCharacters[3]._achievedAwardsBits};
    openLysanderDialogue(game, &lysander);
    const int reputationBeforePromotion = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &lysander, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_CALM_TULAREAN_TREES_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_RETURNED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_HEART_OF_THE_WOOD_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_HEART_OF_THE_WOOD]);
    EXPECT_EQ(lysander.dialogue_1_evt_id, 0u);
    EXPECT_EQ(lysander.greetingIndex, lysanderGreetingId);
    EXPECT_EQ(pParty->GetGold(), initialGold);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBeforePromotion - 10);

    for (int i : {0, 2}) {
        auto expectedAwards = awardsBeforePromotion[i];
        expectedAwards.set(AWARD_PROMOTION_RANGER_LORD);
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_RANGER_LORD);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBeforePromotion[i] + 80000);
    }
    for (int i : {1, 3}) {
        auto expectedAwards = awardsBeforePromotion[i];
        expectedAwards.set(AWARD_PROMOTION_RANGER_LORD_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBeforePromotion[i] + 40000);
    }

    const std::array finalExperience = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array finalClasses = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                     pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array finalAwards = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    const int finalGold = pParty->GetGold();
    const int finalReputation = pParty->GetPartyReputation();
    leaveDialogue(game);
    openLysanderDialogue(game, &lysander);
    EXPECT_FALSE(npcEventVisible(&lysander, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &lysander, returnEventId));
    EXPECT_EQ(pParty->GetGold(), finalGold);
    EXPECT_EQ(pParty->GetPartyReputation(), finalReputation);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, finalExperience[i]);
        EXPECT_EQ(pParty->pCharacters[i].classType, finalClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, finalAwards[i]);
    }
    leaveDialogue(game);
}

