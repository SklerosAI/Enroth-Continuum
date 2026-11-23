#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include "Testing/Game/GameTest.h"
#include "Engine/Mods/PrivateMods.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int frederickNpcId = 43;
constexpr int cavalierOfferEventId = 73;
constexpr int cavalierReturnEventId = 74;
constexpr int blackKnightOfferEventId = 75;
constexpr int blackKnightReturnEventId = 76;
constexpr int treasuryEventId = 376;
constexpr int treasuryChestEventId = 180;
constexpr int treasuryChestId = 5;

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

void openFrederickDialogue(EngineController &game, NPCData *frederick) {
    game.teleportTo(MAP_ERATHIA, Vec3f(), 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_ERATHIA_ORG_HOUSE))
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
            game.teleportTo(MAP_ERATHIA, position, yaw);
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
    ASSERT_EQ(frederick->house, HOUSE_ERATHIA_ORG_HOUSE);
    auto entry = std::ranges::find(houseNpcs, frederick, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

int pointAtIndoorEventFace(EngineController &game, int eventId) {
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
                game.teleportTo(MAP_CASTLE_NAVAN, position, yaw);
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

void killTreasuryGuard(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_NAVAN);
    constexpr Vec3f guardAnchor(2500, 9300, -1536);
    auto guard = pActors.end();
    float bestDistance = 1000000;
    for (auto candidate = pActors.begin(); candidate != pActors.end(); ++candidate) {
        float distance = (candidate->pos - guardAnchor).length();
        if (candidate->group == 5 && !candidate->IsNotAlive() && distance < bestDistance) {
            guard = candidate;
            bestDistance = distance;
        }
    }
    ASSERT_NE(guard, pActors.end());
    ASSERT_EQ(guard->group, 5u);
    ASSERT_FALSE(guard->IsNotAlive());
    const int guardId = guard - pActors.begin();

    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character.uLevel = 100;
    }

    game.teleportTo(MAP_CASTLE_NAVAN, guard->pos + Vec3f(0, -250, 0), 90);
    game.pointMouseAtActor(guardId);
    for (int attack = 0; attack < 300 && !guard->IsNotAlive(); ++attack) {
        if (attack && attack % 10 == 0) {
            game.teleportTo(MAP_CASTLE_NAVAN, guard->pos + Vec3f(0, -250, 0), 90);
            game.pointMouseAtActor(guardId);
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(5);
    }
    ASSERT_TRUE(guard->IsNotAlive()) << "remaining HP: " << guard->hp;
}

void walkIntoTreasury(EngineController &game) {
    ASSERT_FALSE(pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]);
    // Setup teleport: beside a treasury chest, just short of the authored event-376 region.
    game.teleportTo(MAP_CASTLE_NAVAN, Vec3f(2224, 12128, -2344), 90);
    ASSERT_FALSE(pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]);
    game.pressKey(PlatformKey::KEY_UP);
    for (int frame = 0; frame < 160 && !pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]; ++frame)
        game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_TRUE(pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]);
}

void lootTreasuryChest(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_NAVAN);
    ASSERT_GT(vChests[treasuryChestId].inventory.size(), 0);
    const int itemsBefore = vChests[treasuryChestId].inventory.size();
    ASSERT_NE(pointAtIndoorEventFace(game, treasuryChestEventId), -1);
    for (int click = 0; click < 2 && current_screen_type != SCREEN_CHEST; ++click) {
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_NE(pGUIWindow_CurrentChest, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), treasuryChestId);
    for (int item = 0; item < 20 && vChests[treasuryChestId].inventory.size() == itemsBefore; ++item) {
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_LT(vChests[treasuryChestId].inventory.size(), itemsBefore);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void preparePromotionParty() {
    const std::array classes = {CLASS_CAVALIER, CLASS_CAVALIER, CLASS_KNIGHT, CLASS_PALADIN};
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = classes[i];
        pParty->pCharacters[i]._achievedAwardsBits.set(i < 2 ? AWARD_PROMOTION_CHEVALIER :
                                                       AWARD_PROMOTION_CHEVALIER_HONORARY);
    }
}

}  // namespace

GAME_TEST(MM7BlackKnightPromotion, DragoonNoKnightDarkPromotion) {
    if (!mm7::mods::dragoonClassEnabled(engine->gameTitle()))
        GTEST_SKIP() << "Requires ENROTH_MM7_DRAGOON_CLASS=1";
    game.startNewGame();
    preparePromotionParty();
    pParty->pCharacters[0].classType = CLASS_LANCER;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_CLERIC;
    pParty->_questBits.set(QBIT_DARK_PATH);
    NPCData &frederick = pNPCStats->pNPCData[frederickNpcId];
    frederick.dialogue_1_evt_id = blackKnightOfferEventId;
    openFrederickDialogue(game, &frederick);
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightOfferEventId));
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightReturnEventId));
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_LANCER);
    pParty->_questBits.set(QBIT_ELVEN_TREASURY_ROBBED);
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightReturnEventId));
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_DREAD_LANCER);
    for (int i = 1; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_CLERIC);
}

GAME_TEST(MM7BlackKnightPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &frederick = pNPCStats->pNPCData[frederickNpcId];
    EXPECT_EQ(frederick.house, HOUSE_ERATHIA_ORG_HOUSE);
    EXPECT_EQ(frederick.dialogue_1_evt_id, cavalierOfferEventId);
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, cavalierReturnEventId, frederickNpcId, 0,
                                  blackKnightOfferEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, blackKnightOfferEventId, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_DARK_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, blackKnightOfferEventId, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_ELVEN_TREASURY_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, blackKnightOfferEventId, frederickNpcId, 0,
                                  blackKnightReturnEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, blackKnightReturnEventId, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_ELVEN_TREASURY_ROBBED)));

    game.teleportTo(MAP_CASTLE_NAVAN, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(treasuryEventId));
    EXPECT_TRUE(eventChangesVariable(engine->_localEventMap, treasuryEventId, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_ELVEN_TREASURY_ROBBED)));
    EXPECT_TRUE(std::ranges::any_of(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == treasuryEventId && face.Pressure_Plate();
    }));
    ASSERT_TRUE(engine->_localEventMap.hasEvent(treasuryChestEventId));
    EXPECT_TRUE(std::ranges::any_of(engine->_localEventMap.function(treasuryChestEventId),
                                    [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == treasuryChestId;
    }));
    EXPECT_TRUE(std::ranges::any_of(pActors, [](Actor &actor) {
        return actor.group == 5 && !actor.IsNotAlive();
    }));
}

GAME_TEST(MM7BlackKnightPromotion, OfferRequiresCavalierProgressionAndDarkPath) {
    game.startNewGame();
    NPCData &frederick = pNPCStats->pNPCData[frederickNpcId];
    pParty->pCharacters[0].classType = CLASS_KNIGHT;
    pParty->pCharacters[1].classType = CLASS_KNIGHT;

    openFrederickDialogue(game, &frederick);
    EXPECT_FALSE(npcEventVisible(&frederick, blackKnightOfferEventId));
    ASSERT_TRUE(selectNpcEvent(game, &frederick, cavalierOfferEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE]);
    EXPECT_EQ(frederick.dialogue_1_evt_id, cavalierReturnEventId);

    ASSERT_TRUE(selectNpcEvent(game, &frederick, cavalierReturnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE]);
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_KNIGHT);
    EXPECT_EQ(frederick.dialogue_1_evt_id, cavalierReturnEventId);

    pParty->_questBits.set(QBIT_HAUNTED_HOUSE_CLEARED);
    ASSERT_TRUE(selectNpcEvent(game, &frederick, cavalierReturnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE]);
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_CAVALIER);
    EXPECT_EQ(pParty->pCharacters[1].classType, CLASS_CAVALIER);
    EXPECT_TRUE(pParty->pCharacters[0]._achievedAwardsBits[AWARD_PROMOTION_CHEVALIER]);
    EXPECT_TRUE(npcEventVisible(&frederick, blackKnightOfferEventId));
    EXPECT_EQ(frederick.dialogue_1_evt_id, blackKnightOfferEventId);

    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightOfferEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ELVEN_TREASURY_ACTIVE]);
    EXPECT_EQ(frederick.dialogue_1_evt_id, blackKnightOfferEventId);

    pParty->_questBits.set(QBIT_LIGHT_PATH);
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightOfferEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ELVEN_TREASURY_ACTIVE]);
    EXPECT_EQ(frederick.dialogue_1_evt_id, blackKnightOfferEventId);

    pParty->_questBits.reset(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_DARK_PATH);
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightOfferEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_ELVEN_TREASURY_ACTIVE]);
    EXPECT_EQ(frederick.dialogue_1_evt_id, blackKnightReturnEventId);
    leaveDialogue(game);
}

GAME_TEST(MM7BlackKnightPromotion, RobsTreasuryAndPromotes) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    preparePromotionParty();
    pParty->_questBits.set(QBIT_DARK_PATH);
    NPCData &frederick = pNPCStats->pNPCData[frederickNpcId];
    frederick.dialogue_1_evt_id = blackKnightOfferEventId;  // Event 74 establishes this endpoint.

    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};

    openFrederickDialogue(game, &frederick);
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightOfferEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_ELVEN_TREASURY_ACTIVE]);
    ASSERT_EQ(frederick.dialogue_1_evt_id, blackKnightReturnEventId);

    const int goldBeforeRefusal = pParty->GetGold();
    const int reputationBeforeRefusal = pParty->GetPartyReputation();
    const std::array experienceBeforeRefusal = {
        pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience,
    };
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightReturnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_ELVEN_TREASURY_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]);
    EXPECT_EQ(frederick.dialogue_1_evt_id, blackKnightReturnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBeforeRefusal);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBeforeRefusal);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBeforeRefusal[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    game.teleportTo(MAP_CASTLE_NAVAN, Vec3f(), 0);  // Setup teleport: Castle Navan party start.
    killTreasuryGuard(game);
    walkIntoTreasury(game);
    lootTreasuryChest(game);
    ASSERT_TRUE(pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]);

    openFrederickDialogue(game, &frederick);
    const int goldBeforeReturn = pParty->GetGold();
    const int reputationBeforeReturn = pParty->GetPartyReputation();
    const std::array experienceBeforeReturn = {
        pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience,
    };
    ASSERT_TRUE(selectNpcEvent(game, &frederick, blackKnightReturnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ELVEN_TREASURY_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]);
    EXPECT_EQ(frederick.dialogue_1_evt_id, 0u);
    EXPECT_EQ(frederick.greetingIndex, 63);
    EXPECT_EQ(pParty->GetGold(), goldBeforeReturn);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, 10);
    for (int i = 0; i < 4; ++i) {
        const bool qualifying = classBefore[i] == CLASS_CAVALIER;
        EXPECT_EQ(pParty->pCharacters[i].classType, qualifying ? CLASS_BLACK_KNIGHT : classBefore[i]);
        auto expectedAwards = awardsBefore[i];
        expectedAwards.set(qualifying ? AWARD_PROMOTION_BLACK_KNIGHT : AWARD_PROMOTION_BLACK_KNIGHT_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBeforeReturn[i], qualifying ? 80000 : 40000);
    }

    leaveDialogue(game);
    const std::array classAfter = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                   pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    const int goldAfter = pParty->GetGold();
    const int reputationAfter = pParty->GetPartyReputation();
    openFrederickDialogue(game, &frederick);
    EXPECT_FALSE(npcEventVisible(&frederick, blackKnightReturnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &frederick, blackKnightReturnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ELVEN_TREASURY_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ELVEN_TREASURY_ROBBED]);
    EXPECT_EQ(frederick.dialogue_1_evt_id, 0u);
    EXPECT_EQ(frederick.greetingIndex, 63);
    EXPECT_EQ(pParty->GetGold(), goldAfter);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationAfter);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}
