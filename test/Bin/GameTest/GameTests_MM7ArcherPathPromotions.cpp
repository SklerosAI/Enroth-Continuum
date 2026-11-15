#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/EvtContext.h"
#include "Engine/Evt/Processor.h"
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
#include "GUI/UI/UIChest.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int lawrenceNpcId = 40;
constexpr int snickNpcId = 41;
constexpr int lightOfferEventId = 65;
constexpr int lightReturnEventId = 66;
constexpr int darkOfferEventId = 69;
constexpr int darkReturnEventId = 70;
constexpr int bowChestEventId = 176;
constexpr int bowChestId = 1;
constexpr int fallbackChestId = 0;
constexpr int bowMapVariable = 2;
constexpr int lawrenceGreetingId = 57;
constexpr int snickGreetingId = 59;

bool eventChangesQuestBit(const EvtProgram &program, int eventId, EvtOpcode opcode, QuestBit bit) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == opcode &&
                      instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                      instruction.data.variable_descr.value == std::to_underlying(bit);
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

bool eventSetsNpcGreeting(const EvtProgram &program, int eventId, int npcId, int greeting) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SetNPCGreeting && instruction.data.npc_descr.npc_id == npcId &&
                      instruction.data.npc_descr.greeting == greeting;
           });
}

bool eventChangesItem(const EvtProgram &program, int eventId, EvtOpcode opcode, ItemId item) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == opcode &&
                      instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
                      instruction.data.variable_descr.value == std::to_underlying(item);
           });
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
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
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
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

void openPromoterDialogue(EngineController &game, MapId mapId, HouseId houseId, NPCData *npc) {
    game.teleportTo(mapId, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> approaches;
    for (const BLVFace &face : pOutdoor->faces) {
        if (!face.Clickable() || !eventOpensHouse(face.eventId, houseId))
            continue;
        Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
        bool onWater = false;
        int floorFaceId = -1;
        position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
        int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) *
                              180 / std::numbers::pi);
        approaches.emplace_back(position, yaw);
    }
    ASSERT_FALSE(approaches.empty());
    for (const auto &[position, yaw] : approaches) {
        game.teleportTo(mapId, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == houseId)
            break;
        if (current_screen_type == SCREEN_NPC_DIALOGUE) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    auto entry = std::ranges::find(houseNpcs, npc, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

int pointAtTitansBowFace(EngineController &game) {
    if (engine->_currentLoadedMapId != MAP_TITANS_STRONGHOLD)
        game.teleportTo(MAP_TITANS_STRONGHOLD, Vec3f(), 0);

    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.eventId != bowChestEventId)
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
                game.teleportTo(MAP_TITANS_STRONGHOLD, position, yaw);
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

void clickTitansBowFace(EngineController &game) {
    ASSERT_NE(pointAtTitansBowFace(game), -1);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

void closeChest(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void pickUpPerfectBow(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), bowChestId);
    InventoryEntry entry = vChests[bowChestId].inventory.find(ITEM_SPECIAL_THE_PERFECT_BOW);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    Pointi itemPosition = chestTable[vChests[bowChestId].chestTypeId].inventoryOffset +
                          entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_SPECIAL_THE_PERFECT_BOW);
    closeChest(game);
}

void preparePromotionParty(AwardId prerequisite, bool lightPath, bool darkPath) {
    const std::array classes = {CLASS_WARRIOR_MAGE, CLASS_KNIGHT, CLASS_WARRIOR_MAGE, CLASS_SORCERER};
    for (int i = 0; i < 4; ++i)
        pParty->pCharacters[i].classType = classes[i];
    if (prerequisite != AWARD_INVALID)
        pParty->pCharacters[0]._achievedAwardsBits.set(prerequisite);
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
}

void prepareTrappedBowChest() {
    vChests[bowChestId].flags |= CHEST_TRAPPED;
    for (Character &character : pParty->pCharacters)
        character.setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(0, MASTERY_NONE));
}

void runArcherPathPromotion(EngineController &game, bool lightPath) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    NPCData &promoter = pNPCStats->pNPCData[lightPath ? lawrenceNpcId : snickNpcId];
    const MapId promoterMap = lightPath ? MAP_HARMONDALE : MAP_AVLEE;
    const HouseId promoterHouse = lightPath ? HOUSE_HARMONDALE_MARK_MANOR : HOUSE_AVLEE_SNICK_HOUSE;
    const int offerEventId = lightPath ? lightOfferEventId : darkOfferEventId;
    const int returnEventId = lightPath ? lightReturnEventId : darkReturnEventId;
    const QuestBit activeBit = lightPath ? QBIT_PERFECT_BOW_LIGHT_ACTIVE : QBIT_PERFECT_BOW_DARK_ACTIVE;
    const AwardId promotionAward = lightPath ? AWARD_PROMOTION_MASTER_ARCHER : AWARD_PROMOTION_SNIPER;
    const AwardId honoraryAward = lightPath ? AWARD_PROMOTION_MASTER_ARCHER_HONORARY :
                                               AWARD_PROMOTION_SNIPER_HONORARY;
    const Class promotionClass = lightPath ? CLASS_MASTER_ARCHER : CLASS_SNIPER;
    const int greetingId = lightPath ? lawrenceGreetingId : snickGreetingId;
    const AwardId prerequisite = lightPath ? AWARD_PROMOTION_WARRIOR_MAGE :
                                             AWARD_PROMOTION_WARRIOR_MAGE_HONORARY;
    preparePromotionParty(prerequisite, lightPath, !lightPath);
    if (!lightPath)
        promoter.dialogue_1_evt_id = darkOfferEventId; // The first promotion hands Snick this authored topic.

    const int greetingBefore = promoter.greetingIndex;
    ASSERT_NE(greetingBefore, greetingId);
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};

    openPromoterDialogue(game, promoterMap, promoterHouse, &promoter);
    const int offerGold = pParty->GetGold();
    const int offerReputation = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &promoter, offerEventId));
    EXPECT_TRUE(pParty->_questBits[activeBit]);
    EXPECT_EQ(promoter.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), offerGold);
    EXPECT_EQ(pParty->GetPartyReputation(), offerReputation);
    leaveDialogue(game);

    openPromoterDialogue(game, promoterMap, promoterHouse, &promoter);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_TRUE(selectNpcEvent(game, &promoter, returnEventId));
    EXPECT_TRUE(pParty->_questBits[activeBit]);
    EXPECT_EQ(promoter.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), offerGold);
    EXPECT_EQ(pParty->GetPartyReputation(), offerReputation);
    EXPECT_EQ(promoter.greetingIndex, greetingBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    game.teleportTo(MAP_TITANS_STRONGHOLD, Vec3f(), 0);
    prepareTrappedBowChest();
    ASSERT_TRUE(vChests[bowChestId].Trapped());
    clickTitansBowFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_FALSE(pParty->_questBits[QBIT_PERFECT_BOW_TAKEN]);
    clickTitansBowFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), bowChestId);
    ASSERT_TRUE(pParty->_questBits[QBIT_PERFECT_BOW_TAKEN]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[bowMapVariable], 1);
    pickUpPerfectBow(game);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_SPECIAL_THE_PERFECT_BOW);

    openPromoterDialogue(game, promoterMap, promoterHouse, &promoter);
    const int goldBeforeReturn = pParty->GetGold();
    const int reputationBeforeReturn = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &promoter, returnEventId));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_SPECIAL_THE_PERFECT_BOW_FIXED);
    EXPECT_FALSE(pParty->_questBits[activeBit]);
    EXPECT_TRUE(pParty->_questBits[QBIT_PERFECT_BOW_TAKEN]);
    EXPECT_EQ(promoter.dialogue_1_evt_id, 0u);
    EXPECT_EQ(promoter.greetingIndex, greetingId);
    EXPECT_EQ(pParty->GetGold(), goldBeforeReturn);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, -10);
    for (int i : {0, 2}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, promotionClass);
        auto expected = awardsBefore[i];
        expected.set(promotionAward);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expected);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 80000);
    }
    for (int i : {1, 3}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        auto expected = awardsBefore[i];
        expected.set(honoraryAward);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expected);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 40000);
    }

    Item fixedBow = pParty->takeHoldingItem();
    ASSERT_EQ(fixedBow.itemId, ITEM_SPECIAL_THE_PERFECT_BOW_FIXED);
    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    openPromoterDialogue(game, promoterMap, promoterHouse, &promoter);
    EXPECT_FALSE(npcEventVisible(&promoter, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &promoter, returnEventId));
    EXPECT_EQ(pParty->GetGold(), goldBeforeReturn);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, -10);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);

    game.teleportTo(MAP_TITANS_STRONGHOLD, Vec3f(), 0);
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt)
        clickTitansBowFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    const int reopenedChestId = pGUIWindow_CurrentChest->chestId();
    EXPECT_EQ(reopenedChestId, engine->_persistentVariables.mapVars[bowMapVariable] ? bowChestId : fallbackChestId);
    EXPECT_FALSE(vChests[reopenedChestId].inventory.find(ITEM_SPECIAL_THE_PERFECT_BOW));
    EXPECT_EQ(fixedBow.itemId, ITEM_SPECIAL_THE_PERFECT_BOW_FIXED);
    closeChest(game);
}

} // namespace

GAME_TEST(MM7ArcherPathPromotions, SourceContracts) {
    game.startNewGame();
    const NPCData &lawrence = pNPCStats->pNPCData[lawrenceNpcId];
    const NPCData &snick = pNPCStats->pNPCData[snickNpcId];
    EXPECT_EQ(lawrence.house, HOUSE_HARMONDALE_MARK_MANOR);
    EXPECT_EQ(snick.house, HOUSE_AVLEE_SNICK_HOUSE);
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, lightOfferEventId, EVENT_Add,
                                     QBIT_PERFECT_BOW_LIGHT_ACTIVE));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, lightOfferEventId, lawrenceNpcId, 0,
                                  lightReturnEventId));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, darkOfferEventId, EVENT_Set,
                                     QBIT_PERFECT_BOW_DARK_ACTIVE));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, darkOfferEventId, snickNpcId, 0,
                                  darkReturnEventId));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, lightReturnEventId, EVENT_Subtract,
                                     QBIT_PERFECT_BOW_LIGHT_ACTIVE));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, darkReturnEventId, EVENT_Subtract,
                                     QBIT_PERFECT_BOW_DARK_ACTIVE));
    EXPECT_TRUE(eventChangesItem(engine->_globalEventMap, lightReturnEventId, EVENT_Subtract,
                                 ITEM_SPECIAL_THE_PERFECT_BOW));
    EXPECT_TRUE(eventChangesItem(engine->_globalEventMap, lightReturnEventId, EVENT_Add,
                                 ITEM_SPECIAL_THE_PERFECT_BOW_FIXED));
    EXPECT_TRUE(eventChangesItem(engine->_globalEventMap, darkReturnEventId, EVENT_Subtract,
                                 ITEM_SPECIAL_THE_PERFECT_BOW));
    EXPECT_TRUE(eventChangesItem(engine->_globalEventMap, darkReturnEventId, EVENT_Add,
                                 ITEM_SPECIAL_THE_PERFECT_BOW_FIXED));
    EXPECT_TRUE(eventSetsNpcGreeting(engine->_globalEventMap, lightReturnEventId, lawrenceNpcId,
                                     lawrenceGreetingId));
    EXPECT_TRUE(eventSetsNpcGreeting(engine->_globalEventMap, darkReturnEventId, snickNpcId,
                                     snickGreetingId));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, 68, snickNpcId, 0, darkOfferEventId));

    game.teleportTo(MAP_TITANS_STRONGHOLD, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(bowChestEventId));
    const std::span<const EvtInstruction> event = engine->_localEventMap.function(bowChestEventId);
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 8 && instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_PERFECT_BOW_TAKEN);
    }));
    EXPECT_TRUE(vChests[bowChestId].Trapped());
    EXPECT_TRUE(vChests[bowChestId].inventory.find(ITEM_SPECIAL_THE_PERFECT_BOW));
}

GAME_TEST(MM7ArcherPathPromotions, TrapDoesNotMarkBowTakenBeforeChestOpens) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_PERFECT_BOW_LIGHT_ACTIVE);
    game.teleportTo(MAP_TITANS_STRONGHOLD, Vec3f(), 0);
    prepareTrappedBowChest();
    ASSERT_TRUE(vChests[bowChestId].Trapped());

    clickTitansBowFace(game);

    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_FALSE(vChests[bowChestId].Trapped());
    EXPECT_FALSE(pParty->_questBits[QBIT_PERFECT_BOW_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[bowMapVariable], 0);

    clickTitansBowFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), bowChestId);
    EXPECT_TRUE(pParty->_questBits[QBIT_PERFECT_BOW_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.mapVars[bowMapVariable], 1);
    closeChest(game);

    clickTitansBowFace(game);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), bowChestId);
    closeChest(game);

    Blob checkpoint = game.saveGame();
    game.loadGame(checkpoint);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TITANS_STRONGHOLD);
    ASSERT_EQ(engine->_persistentVariables.mapVars[bowMapVariable], 1);
    runEvent(EvtContext(EVT_SOURCE_MAP, bowChestEventId));
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), bowChestId);
    closeChest(game);
}

GAME_TEST(MM7ArcherPathPromotions, FreshTakenMapUsesFallbackChest) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_PERFECT_BOW_TAKEN);
    ASSERT_EQ(engine->_persistentVariables.mapVars[bowMapVariable], 0);
    game.teleportTo(MAP_TITANS_STRONGHOLD, Vec3f(), 0);
    vChests[fallbackChestId].flags &= ~CHEST_TRAPPED;

    clickTitansBowFace(game);

    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), fallbackChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[bowMapVariable], 0);
    closeChest(game);
}

GAME_TEST(MM7ArcherPathPromotions, OffersRequirePriorPromotionAndMatchingPath) {
    const std::array lightCases = {
        std::tuple{AWARD_INVALID, true, false},
        std::tuple{AWARD_PROMOTION_WARRIOR_MAGE, false, false},
        std::tuple{AWARD_PROMOTION_WARRIOR_MAGE_HONORARY, false, true},
    };
    for (const auto &[prerequisite, lightPath, darkPath] : lightCases) {
        SCOPED_TRACE(fmt::format("light offer: award={}, light={}, dark={}", std::to_underlying(prerequisite),
                                 lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        preparePromotionParty(prerequisite, lightPath, darkPath);
        NPCData &lawrence = pNPCStats->pNPCData[lawrenceNpcId];
        openPromoterDialogue(game, MAP_HARMONDALE, HOUSE_HARMONDALE_MARK_MANOR, &lawrence);
        const int goldBefore = pParty->GetGold();
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &lawrence, lightOfferEventId));
        EXPECT_FALSE(pParty->_questBits[QBIT_PERFECT_BOW_LIGHT_ACTIVE]);
        EXPECT_EQ(lawrence.dialogue_1_evt_id, lightOfferEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }

    for (const auto &[lightPath, darkPath] :
         std::array{std::pair{false, false}, std::pair{true, false}}) {
        SCOPED_TRACE(fmt::format("dark offer: light={}, dark={}", lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        preparePromotionParty(AWARD_PROMOTION_WARRIOR_MAGE, lightPath, darkPath);
        NPCData &snick = pNPCStats->pNPCData[snickNpcId];
        snick.dialogue_1_evt_id = darkOfferEventId;
        openPromoterDialogue(game, MAP_AVLEE, HOUSE_AVLEE_SNICK_HOUSE, &snick);
        const int goldBefore = pParty->GetGold();
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &snick, darkOfferEventId));
        EXPECT_FALSE(pParty->_questBits[QBIT_PERFECT_BOW_DARK_ACTIVE]);
        EXPECT_EQ(snick.dialogue_1_evt_id, darkOfferEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }

    test.prepareForNextTest();
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    preparePromotionParty(AWARD_INVALID, false, true);
    NPCData &snick = pNPCStats->pNPCData[snickNpcId];
    ASSERT_EQ(snick.dialogue_1_evt_id, 67u);
    openPromoterDialogue(game, MAP_AVLEE, HOUSE_AVLEE_SNICK_HOUSE, &snick);
    EXPECT_FALSE(npcEventVisible(&snick, darkOfferEventId));
    EXPECT_FALSE(selectNpcEvent(game, &snick, darkOfferEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_PERFECT_BOW_DARK_ACTIVE]);
    leaveDialogue(game);
}

GAME_TEST(MM7ArcherPathPromotions, PerfectBowPromotesWarriorMagesToMasterArchers) {
    runArcherPathPromotion(game, true);
}

GAME_TEST(MM7ArcherPathPromotions, PerfectBowPromotesWarriorMagesToSnipers) {
    runArcherPathPromotion(game, false);
}
