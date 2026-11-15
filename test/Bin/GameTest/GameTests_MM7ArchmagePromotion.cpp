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
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIDialogue.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int thomasNpcId = 48;
constexpr int offerEventId = 94;
constexpr int returnEventId = 95;
constexpr int bookChestEventId = 195;
constexpr int bookChestId = 0;
constexpr int fallbackChestId = 19;

bool eventSpeaksNpc(int eventId, int npcId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [npcId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakNPC && instruction.data.npc_descr.npc_id == npcId;
           });
}

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

void openThomasGrey(EngineController &game) {
    game.teleportTo(MAP_SCHOOL_OF_SORCERY, Vec3f(), 0);
    auto actor = std::ranges::find(pActors, thomasNpcId, &Actor::npcId);
    if (actor != pActors.end()) {
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(200, 0, 0), 180}, std::pair{Vec3f(-200, 0, 0), 0},
                 std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(0, -200, 0), 90}}) {
            game.teleportTo(MAP_SCHOOL_OF_SORCERY, actor->pos + offset, yaw);
            try {
                game.pointMouseAtActor(actor->id);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (current_screen_type == SCREEN_NPC_DIALOGUE)
                break;
        }
    } else {
        auto decoration = std::ranges::find_if(pLevelDecorations, [](const LevelDecoration &candidate) {
            return candidate.uEventID != 0 && eventSpeaksNpc(candidate.uEventID, thomasNpcId);
        });
        if (decoration != pLevelDecorations.end()) {
            const int decorationId = decoration - pLevelDecorations.begin();
            game.teleportTo(MAP_SCHOOL_OF_SORCERY, decoration->vPosition + Vec3f(300, 0, 0), 180);
            game.pointMouseAtDecoration(decorationId);
        } else {
            auto face = std::ranges::find_if(pIndoor->faces, [](const BLVFace &candidate) {
                return candidate.Clickable() && eventSpeaksNpc(candidate.eventId, thomasNpcId);
            });
            ASSERT_NE(face, pIndoor->faces.end());
            const int faceId = face - pIndoor->faces.begin();
            Vec3f position = face->boundingBox.center() + 150 * face->facePlane.normal;
            position.z = face->boundingBox.z1;
            int yaw = std::lround(std::atan2(-face->facePlane.normal.y, -face->facePlane.normal.x) *
                                  180 / std::numbers::pi);
            game.teleportTo(MAP_SCHOOL_OF_SORCERY, position, yaw);
            game.pointMouseAtFace(faceId);
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    ASSERT_EQ(getNPCData(speakingNpcId)->name, pNPCStats->pNPCData[thomasNpcId].name);
}

int openBreedingZoneQuestChest(EngineController &game) {
    if (engine->_currentLoadedMapId != MAP_BREEDING_ZONE)
        game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
    bool opened = false;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces) && !opened; ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (!face.Clickable() || face.eventId != bookChestEventId)
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
                game.teleportTo(MAP_BREEDING_ZONE, position, yaw);
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

void pickUpDivineInterventionBook(EngineController &game) {
    ASSERT_EQ(openBreedingZoneQuestChest(game), bookChestId);
    InventoryEntry entry = vChests[bookChestId].inventory.find(ITEM_SPELLBOOK_DIVINE_INTERVENTION);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.zone(), INVENTORY_ZONE_GRID);
    Pointi itemPosition = chestTable[vChests[bookChestId].chestTypeId].inventoryOffset +
                          entry.geometry().topLeft() * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPosition);
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_SPELLBOOK_DIVINE_INTERVENTION);
    closeChest(game);
}

} // namespace

GAME_TEST(MM7ArchmagePromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &thomas = pNPCStats->pNPCData[thomasNpcId];
    EXPECT_EQ(thomas.house, HOUSE_521);
    EXPECT_TRUE(engine->_globalEventMap.hasEvent(offerEventId));
    EXPECT_TRUE(engine->_globalEventMap.hasEvent(returnEventId));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, offerEventId, EVENT_Set,
                                     QBIT_DIVINE_INTERVENTION_BOOK_ACTIVE));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, thomasNpcId, 0, returnEventId));

    game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(bookChestEventId));
    const std::span<const EvtInstruction> event = engine->_localEventMap.function(bookChestEventId);
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_DIVINE_INTERVENTION_RETRIEVED);
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == bookChestId;
    }));
    EXPECT_TRUE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == fallbackChestId;
    }));
    EXPECT_FALSE(std::ranges::any_of(event, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_MapPersistentVariable_0;
    }));
}

GAME_TEST(MM7ArchmagePromotion, QuestChestReopensDuringSameVisit) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
    ASSERT_EQ(openBreedingZoneQuestChest(game), bookChestId);
    ASSERT_TRUE(pParty->_questBits[QBIT_DIVINE_INTERVENTION_RETRIEVED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_DIVINE_INTERVENTION_BOOK]);
    ASSERT_EQ(engine->_persistentVariables.mapVars[0], 1);
    ASSERT_TRUE(vChests[bookChestId].inventory.find(ITEM_SPELLBOOK_DIVINE_INTERVENTION));
    closeChest(game);

    EXPECT_EQ(openBreedingZoneQuestChest(game), bookChestId);
    EXPECT_EQ(engine->_persistentVariables.mapVars[0], 1);
    EXPECT_TRUE(vChests[bookChestId].inventory.find(ITEM_SPELLBOOK_DIVINE_INTERVENTION));
    closeChest(game);
}

GAME_TEST(MM7ArchmagePromotion, RetrievedBookOnFreshMapUsesFallbackChest) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    pParty->_questBits.set(QBIT_DIVINE_INTERVENTION_RETRIEVED);
    game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
    ASSERT_EQ(engine->_persistentVariables.mapVars[0], 0);
    EXPECT_EQ(openBreedingZoneQuestChest(game), fallbackChestId);
    closeChest(game);
}

GAME_TEST(MM7ArchmagePromotion, OfferRequiresLightPath) {
    for (const auto &[lightPath, darkPath] : std::array{std::pair{false, false}, std::pair{false, true}}) {
        SCOPED_TRACE(fmt::format("light={}, dark={}", lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        NPCData &thomas = pNPCStats->pNPCData[thomasNpcId];
        thomas.dialogue_1_evt_id = offerEventId;
        pParty->pCharacters[0].classType = CLASS_WIZARD;
        pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
        pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
        const int goldBefore = pParty->GetGold();
        openThomasGrey(game);
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &thomas, offerEventId));
        EXPECT_FALSE(pParty->_questBits[QBIT_DIVINE_INTERVENTION_BOOK_ACTIVE]);
        EXPECT_EQ(thomas.dialogue_1_evt_id, offerEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }
}

GAME_TEST(MM7ArchmagePromotion, BookReturnPromotesWizardAndRetainsBook) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    NPCData &thomas = pNPCStats->pNPCData[thomasNpcId];
    thomas.dialogue_1_evt_id = offerEventId;
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->pCharacters[0].classType = CLASS_WIZARD;
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

    openThomasGrey(game);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &thomas, offerEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_DIVINE_INTERVENTION_BOOK_ACTIVE]);
    EXPECT_EQ(thomas.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    leaveDialogue(game);

    // Thomas exposes the return topic immediately, but refuses without the physical spellbook.
    openThomasGrey(game);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_TRUE(selectNpcEvent(game, &thomas, returnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_DIVINE_INTERVENTION_BOOK_ACTIVE]);
    EXPECT_EQ(thomas.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    pickUpDivineInterventionBook(game);
    ASSERT_TRUE(pParty->_questBits[QBIT_DIVINE_INTERVENTION_RETRIEVED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_DIVINE_INTERVENTION_BOOK]);
    openThomasGrey(game);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_SPELLBOOK_DIVINE_INTERVENTION);
    const int goldBeforeReturn = pParty->GetGold();
    ASSERT_TRUE(selectNpcEvent(game, &thomas, returnEventId));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_SPELLBOOK_DIVINE_INTERVENTION);
    EXPECT_FALSE(pParty->_questBits[QBIT_DIVINE_INTERVENTION_BOOK_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_DIVINE_INTERVENTION_BOOK]);
    EXPECT_TRUE(pParty->_questBits[QBIT_DIVINE_INTERVENTION_RETRIEVED]);
    EXPECT_EQ(thomas.dialogue_1_evt_id, 0u);
    EXPECT_EQ(thomas.greetingIndex, 77);
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 10000);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_ARCHAMGE);
    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_PROMOTION_ARCHMAGE);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBefore[0], 80000);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_ARCHMAGE_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBefore[i], 40000);
    }

    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    openThomasGrey(game);
    EXPECT_FALSE(npcEventVisible(&thomas, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &thomas, returnEventId));
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 10000);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}
