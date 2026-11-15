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
#include "Engine/Evt/EvtContext.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
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

constexpr int wynacNpcId = 49;
constexpr int offerEventId = 96;
constexpr int returnEventId = 97;
constexpr int jarChestEventId = 176;
constexpr int jarChestId = 0;
constexpr int fallbackChestId = 1;
constexpr int lichGreetingId = 79;
constexpr int jarDecorVariable = 23;

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

void openWynacDialogue(EngineController &game, NPCData *wynac) {
    game.teleportTo(MAP_PIT, Vec3f(3398, -7952, 59), 0);
    game.tick(2);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick(2);
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(wynac->house, HOUSE_PIT_DARKENMORE_RESIDENCE);
    auto entry = std::ranges::find(houseNpcs, wynac, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
}

int openWallsJarChest(EngineController &game) {
    if (engine->_currentLoadedMapId != MAP_WALLS_OF_MIST)
        game.teleportTo(MAP_WALLS_OF_MIST, Vec3f(), 0);

    bool opened = false;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces) && !opened; ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.eventId != jarChestEventId)
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
                game.teleportTo(MAP_WALLS_OF_MIST, position, yaw);
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
    EXPECT_TRUE(opened);
    return opened ? pGUIWindow_CurrentChest->chestId() : -1;
}

void closeChest(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void pickUpJarFromChest(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    InventoryEntry entry = vChests[jarChestId].inventory.find(ITEM_QUEST_LICH_JAR_EMPTY);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    Pointi itemPosition = chestTable[vChests[jarChestId].chestTypeId].inventoryOffset +
                          entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_LICH_JAR_EMPTY);
}

void takeJarFromChest(EngineController &game, int characterIndex) {
    pickUpJarFromChest(game);
    ASSERT_TRUE(pParty->pCharacters[characterIndex].inventory.add(pParty->takeHoldingItem()));
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
}

void prepareLichParty(AwardId prerequisite, bool lightPath, bool darkPath) {
    pParty->pCharacters[0].classType = CLASS_WIZARD;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
    if (prerequisite != AWARD_INVALID)
        pParty->pCharacters[0]._achievedAwardsBits.set(prerequisite);
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
}

} // namespace

GAME_TEST(MM7LichPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &wynac = pNPCStats->pNPCData[wynacNpcId];
    EXPECT_EQ(wynac.house, HOUSE_PIT_DARKENMORE_RESIDENCE);
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, offerEventId, EVENT_Set, QBIT_LICH_JARS_ACTIVE));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, wynacNpcId, 0, returnEventId));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, returnEventId, EVENT_Subtract,
                                     QBIT_LICH_JARS_ACTIVE));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, returnEventId, EVENT_Subtract,
                                     QBIT_ORACLE_RECOVERS_LICH_JAR_EMPTY));
    EXPECT_TRUE(eventSetsNpcGreeting(engine->_globalEventMap, returnEventId, wynacNpcId, lichGreetingId));

    game.teleportTo(MAP_WALLS_OF_MIST, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(jarChestEventId));
    const std::span<const EvtInstruction> event = engine->_localEventMap.function(jarChestEventId);
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_LICH_JARS_ACTIVE);
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare && instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value == std::to_underlying(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);
    }));
    for (int chestId : {jarChestId, fallbackChestId}) {
        EXPECT_TRUE(std::ranges::any_of(event, [chestId](const EvtInstruction &instruction) {
            return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == chestId;
        }));
    }
    EXPECT_FALSE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type ==
                   static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentDecorVariable_0) + jarDecorVariable);
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type ==
                   static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentDecorVariable_0) +
                                            jarDecorVariable) &&
               instruction.data.variable_descr.value == 1;
    }));
    EXPECT_EQ(std::ranges::distance(vChests[jarChestId].inventory.entries(ITEM_QUEST_LICH_JAR_EMPTY)), 4);
}

GAME_TEST(MM7LichPromotion, ActiveQuestOverridesSoulJarAward) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_LICH_JARS_ACTIVE);
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);

    EXPECT_EQ(openWallsJarChest(game), jarChestId);
    closeChest(game);
}

GAME_TEST(MM7LichPromotion, InactiveFreshMapStillUsesFallbackChest) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);

    EXPECT_EQ(openWallsJarChest(game), fallbackChestId);
    EXPECT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_LICH_JARS_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.decorVars[jarDecorVariable], 0);
    closeChest(game);
}

GAME_TEST(MM7LichPromotion, OfferRequiresWizardAwardAndDarkPath) {
    const std::array cases = {
        std::tuple{AWARD_INVALID, false, true},
        std::tuple{AWARD_PROMOTION_WIZARD, false, false},
        std::tuple{AWARD_PROMOTION_WIZARD_HONORARY, true, false},
    };
    for (const auto &[prerequisite, lightPath, darkPath] : cases) {
        SCOPED_TRACE(fmt::format("award={}, light={}, dark={}", std::to_underlying(prerequisite),
                                 lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        prepareLichParty(prerequisite, lightPath, darkPath);
        NPCData &wynac = pNPCStats->pNPCData[wynacNpcId];
        const int goldBefore = pParty->GetGold();
        openWynacDialogue(game, &wynac);
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &wynac, offerEventId));
        EXPECT_FALSE(pParty->_questBits[QBIT_LICH_JARS_ACTIVE]);
        EXPECT_EQ(wynac.dialogue_1_evt_id, offerEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }
}

GAME_TEST(MM7LichPromotion, SourceChestMarkerSurvivesSaveReload) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_LICH_JARS_ACTIVE);
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);

    ASSERT_EQ(openWallsJarChest(game), jarChestId);
    ASSERT_EQ(engine->_persistentVariables.decorVars[jarDecorVariable], 1);
    closeChest(game);
    pParty->_questBits.reset(QBIT_LICH_JARS_ACTIVE); // Model the completed promotion without leaving this map.
    Blob checkpoint = game.saveGame();
    game.loadGame(checkpoint);
    game.tick(2);

    ASSERT_EQ(engine->_currentLoadedMapId, MAP_WALLS_OF_MIST);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_FALSE(pParty->_questBits[QBIT_LICH_JARS_ACTIVE]);
    ASSERT_EQ(engine->_persistentVariables.decorVars[jarDecorVariable], 1);
    runEvent(EvtContext(EVT_SOURCE_MAP, jarChestEventId));
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), jarChestId);
    closeChest(game);
}

GAME_TEST(MM7LichPromotion, FourPhysicalJarsPromoteWizard) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    prepareLichParty(AWARD_PROMOTION_WIZARD, false, true);
    NPCData &wynac = pNPCStats->pNPCData[wynacNpcId];
    const int greetingBefore = wynac.greetingIndex;
    ASSERT_NE(greetingBefore, lichGreetingId);
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};

    openWynacDialogue(game, &wynac);
    const int offerGold = pParty->GetGold();
    const int offerReputation = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &wynac, offerEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_LICH_JARS_ACTIVE]);
    EXPECT_EQ(wynac.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), offerGold);
    EXPECT_EQ(pParty->GetPartyReputation(), offerReputation);
    leaveDialogue(game);

    // Award 80 normally redirects this event to empty chest 1. An active Lich promotion must use the physical
    // source chest instead and establish the persistent decor marker for later visits.
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);
    ASSERT_EQ(openWallsJarChest(game), jarChestId);
    EXPECT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_LICH_JARS_TAKEN]);
    EXPECT_EQ(engine->_persistentVariables.decorVars[jarDecorVariable], 1);
    for (int characterIndex = 0; characterIndex < 3; ++characterIndex)
        takeJarFromChest(game, characterIndex);
    EXPECT_EQ(std::ranges::distance(vChests[jarChestId].inventory.entries(ITEM_QUEST_LICH_JAR_EMPTY)), 1);
    pickUpJarFromChest(game);
    Item fourthJar = pParty->takeHoldingItem();
    ASSERT_EQ(fourthJar.itemId, ITEM_QUEST_LICH_JAR_EMPTY);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(vChests[jarChestId].inventory.find(ITEM_QUEST_LICH_JAR_EMPTY));
    closeChest(game);

    openWynacDialogue(game, &wynac);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    const int goldBeforeRefusal = pParty->GetGold();
    const int reputationBeforeRefusal = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &wynac, returnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_LICH_JARS_ACTIVE]);
    EXPECT_EQ(wynac.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBeforeRefusal);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBeforeRefusal);
    EXPECT_EQ(wynac.greetingIndex, greetingBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        auto expected = awardsBefore[i];
        if (i == 0)
            expected.set(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expected);
    }
    leaveDialogue(game);

    ASSERT_TRUE(pParty->pCharacters[3].inventory.add(std::move(fourthJar)));
    pParty->_questBits.set(QBIT_ORACLE_RECOVERS_LICH_JAR_EMPTY);

    openWynacDialogue(game, &wynac);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    const int goldBeforeReturn = pParty->GetGold();
    const int reputationBeforeReturn = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &wynac, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_LICH_JARS_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_LICH_JAR_EMPTY]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_LICH_JARS_TAKEN]);
    EXPECT_EQ(wynac.dialogue_1_evt_id, 0u);
    EXPECT_EQ(wynac.greetingIndex, lichGreetingId);
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 7500);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, -10);

    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_LICH);
    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);
    expectedAwards.set(AWARD_PROMOTION_LICH);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBefore[0], 80000);
    InventoryEntry fullJar = pParty->pCharacters[0].inventory.find(ITEM_QUEST_LICH_JAR_FULL);
    ASSERT_TRUE(fullJar);
    EXPECT_EQ(fullJar->lichJarCharacterIndex, 0);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_LICH_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 40000);
        EXPECT_FALSE(pParty->pCharacters[i].inventory.find(ITEM_QUEST_LICH_JAR_FULL));
    }
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character.inventory.find(ITEM_QUEST_LICH_JAR_EMPTY));

    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    openWynacDialogue(game, &wynac);
    EXPECT_FALSE(npcEventVisible(&wynac, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &wynac, returnEventId));
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 7500);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, -10);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}
