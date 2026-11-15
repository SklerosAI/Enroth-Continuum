#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <string_view>
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
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"
#include "Utility/String/Ascii.h"

#include "MM7CombatHarness.h"

namespace {

constexpr int humeNpcId = 38;
constexpr int firstPromotionOfferEventId = 58;
constexpr int masterOfferEventId = 61;
constexpr int masterReturnEventId = 62;
constexpr int templeMapLeaveEventId = 2;
constexpr int highPriestActorId = 34;
constexpr int masterGreetingId = 52;

bool eventChangesVariable(const EvtProgram &program, int eventId, int step, int targetStep,
                          EvtOpcode opcode, EvtVariable variable, int value) {
    return program.hasEvent(eventId) &&
           std::ranges::any_of(program.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.step == step && instruction.target_step == targetStep &&
                      instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
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

bool eventMovesTo(int eventId, std::string_view destination) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [=](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_MoveToMap && ascii::noCaseEquals(instruction.str, destination);
           });
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [=](const EvtInstruction &instruction) {
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

void openHumeDialogue(EngineController &game, NPCData *hume) {
    game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_HARMONDALE_HUME_RESIDENCE))
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
            game.teleportTo(MAP_HARMONDALE, position, yaw);
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
        ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_HARMONDALE_HUME_RESIDENCE);
    }
    auto entry = std::ranges::find(houseNpcs, hume, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void prepareFirstPromotionOutcome(bool lightPath, bool darkPath) {
    pParty->pCharacters[0].classType = CLASS_INITIATE;
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_INITIATE);
    for (int i = 1; i < 4; ++i) {
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
        pParty->pCharacters[i]._achievedAwardsBits.set(AWARD_PROMOTION_INITIATE_HONORARY);
    }
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
    pNPCStats->pNPCData[humeNpcId].dialogue_1_evt_id = masterOfferEventId;
}

void enterTempleOfBaa(EngineController &game) {
    game.teleportTo(MAP_AVLEE, Vec3f(), 0);
    std::vector<int> entrances;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventMovesTo(face.eventId, "d04.blv"))
            entrances.push_back(faceId);
    }
    ASSERT_FALSE(entrances.empty());

    for (int faceId : entrances) {
        const BLVFace &face = pOutdoor->faces[faceId];
        for (float distance : {150.0f, 250.0f, -150.0f, -250.0f}) {
            Vec3f position = face.boundingBox.center() + distance * face.facePlane.normal;
            bool onWater = false;
            int floorFaceId = -1;
            position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
            int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) *
                                  180 / std::numbers::pi);
            if (distance < 0)
                yaw += 180;
            game.teleportTo(MAP_AVLEE, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            if (current_screen_type == SCREEN_INPUT_BLV)
                break;
        }
        if (current_screen_type == SCREEN_INPUT_BLV)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TEMPLE_OF_BAA);
}

void leaveTempleOfBaa(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TEMPLE_OF_BAA);
    std::vector<int> exits;
    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.Clickable() && eventMovesTo(face.eventId, "out14.odm"))
            exits.push_back(faceId);
    }
    ASSERT_FALSE(exits.empty());

    for (int faceId : exits) {
        const BLVFace &face = pIndoor->faces[faceId];
        for (float distance : {100.0f, 180.0f, -100.0f, -180.0f}) {
            Vec3f position = face.boundingBox.center() + distance * face.facePlane.normal;
            int sectorId = 0;
            int floorFaceId = -1;
            float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
            if (floorZ == -30000 || floorFaceId == -1)
                continue;
            position.z = floorZ;
            int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) *
                                  180 / std::numbers::pi);
            if (distance < 0)
                yaw += 180;
            game.teleportTo(MAP_TEMPLE_OF_BAA, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            if (current_screen_type == SCREEN_INPUT_BLV)
                break;
        }
        if (current_screen_type == SCREEN_INPUT_BLV)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_AVLEE);
}

bool actorCarriesItem(const Actor &actor, ItemId itemId) {
    return actor.carriedItemId == itemId || std::ranges::any_of(actor.items, [=](const Item &item) {
        return item.itemId == itemId;
    });
}

bool partyHasItem(ItemId itemId) {
    return pParty->hasItem(itemId) || pParty->pPickedItem.itemId == itemId;
}

bool approachHighPriest(EngineController &game) {
    if (highPriestActorId >= std::ssize(pActors))
        return false;
    const Actor &priest = pActors[highPriestActorId];
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(0, -250, 0), 90}, std::pair{Vec3f(-250, 0, 0), 0},
             std::pair{Vec3f(0, 250, 0), 270}, std::pair{Vec3f(250, 0, 0), 180}}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(MAP_TEMPLE_OF_BAA, priest.pos + offset, yaw, pitch);
            try {
                game.pointMouseAtActor(highPriestActorId);
                return true;
            } catch (const Exception &) {
            }
        }
    }
    return false;
}

void prepareCombatParty() {
    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character.uLevel = 100;
    }
}

void lootHighPriestCloak(EngineController &game) {
    ASSERT_LT(highPriestActorId, std::ssize(pActors));
    ASSERT_EQ(pActors[highPriestActorId].aiState, Dead);
    auto viewport = std::ranges::find(pPrimaryWindow->vButtons, "Game_Viewport", &GUIButton::id);
    ASSERT_NE(viewport, pPrimaryWindow->vButtons.end());

    bool corpseVisible = false;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(0, -250, 0), 90}, std::pair{Vec3f(-250, 0, 0), 0},
             std::pair{Vec3f(0, 250, 0), 270}, std::pair{Vec3f(250, 0, 0), 180}}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(MAP_TEMPLE_OF_BAA, pActors[highPriestActorId].pos + offset, yaw, pitch);
            for (int y = 10; y < 470 && !corpseVisible; y += 10) {
                for (int x = 10; x < 630 && !corpseVisible; x += 10) {
                    Pointi point(x, y);
                    if (!(*viewport)->Contains(point))
                        continue;
                    if (engine->PickMouse(engine->config->gameplay.MouseInteractionDepth.value(), x, y,
                                          &vis_anything_filter, &vis_face_filter).pid !=
                        Pid::actor(highPriestActorId))
                        continue;
                    game.moveMouse(point);
                    game.tick();
                    corpseVisible = engine->PickMouseForInteraction().pid == Pid::actor(highPriestActorId);
                }
            }
            if (corpseVisible)
                break;
        }
        if (corpseVisible)
            break;
    }
    ASSERT_TRUE(corpseVisible);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    ASSERT_TRUE(partyHasItem(ITEM_ARTIFACT_CLOAK_OF_THE_SHEEP));
}

} // namespace

GAME_TEST(MM7MasterPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &hume = pNPCStats->pNPCData[humeNpcId];
    EXPECT_EQ(hume.house, HOUSE_HARMONDALE_HUME_RESIDENCE);
    EXPECT_EQ(hume.dialogue_1_evt_id, firstPromotionOfferEventId);

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterOfferEventId, 0, 4, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_DARK_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterOfferEventId, 1, 6, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_LIGHT_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterOfferEventId, 7, 0, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HIGH_PRIEST_OF_BAA_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, masterOfferEventId, 8, humeNpcId, 0,
                                 masterReturnEventId));
    EXPECT_TRUE(std::ranges::none_of(engine->_globalEventMap.function(masterOfferEventId),
                                    [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare && instruction.data.variable_descr.type == VAR_Award;
    })); // The Initiate prerequisite is the authored topic transition from the first promotion, not a second gate.

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 1, 4, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HIGH_PRIEST_OF_BAA_KILLED)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 6, 10, EVENT_Compare,
                                     VAR_Class, std::to_underlying(CLASS_INITIATE)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 7, 0, EVENT_Add,
                                     VAR_Award, std::to_underlying(AWARD_PROMOTION_MASTER_HONORARY)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 8, 0, EVENT_Add,
                                     VAR_Experience, 40000));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 10, 0, EVENT_Set,
                                     VAR_Class, std::to_underlying(CLASS_MASTER)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 11, 0, EVENT_Add,
                                     VAR_Award, std::to_underlying(AWARD_PROMOTION_MASTER)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 12, 0, EVENT_Add,
                                     VAR_Experience, 80000));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 37, 0, EVENT_Add,
                                     VAR_FixedGold, 7500));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 38, 0, EVENT_Subtract,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HIGH_PRIEST_OF_BAA_ACTIVE)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, masterReturnEventId, 39, 0, EVENT_Subtract,
                                     VAR_ReputationInCurrentLocation, 10));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, masterReturnEventId, 41, humeNpcId, 0, 0));
    EXPECT_TRUE(std::ranges::any_of(engine->_globalEventMap.function(masterReturnEventId),
                                   [](const EvtInstruction &instruction) {
        return instruction.step == 42 && instruction.opcode == EVENT_SetNPCGreeting &&
               instruction.data.npc_descr.npc_id == humeNpcId &&
               instruction.data.npc_descr.greeting == masterGreetingId;
    }));
    EXPECT_TRUE(std::ranges::none_of(engine->_globalEventMap.function(masterReturnEventId),
                                    [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Compare || instruction.opcode == EVENT_Subtract) &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands;
    }));

    game.teleportTo(MAP_TEMPLE_OF_BAA, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(templeMapLeaveEventId));
    const std::span<const EvtInstruction> mapLeave = engine->_localEventMap.function(templeMapLeaveEventId);
    EXPECT_TRUE(std::ranges::any_of(mapLeave, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.opcode == EVENT_OnMapLeave;
    }));
    EXPECT_TRUE(std::ranges::any_of(mapLeave, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 3 &&
               instruction.opcode == EVENT_IsActorKilled &&
               instruction.data.actor_descr.policy == KILL_CHECK_ACTORID &&
               instruction.data.actor_descr.param == highPriestActorId && instruction.data.actor_descr.num == 1;
    }));
    EXPECT_TRUE(eventChangesVariable(engine->_localEventMap, templeMapLeaveEventId, 3, 0, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_HIGH_PRIEST_OF_BAA_KILLED)));
    ASSERT_LT(highPriestActorId, std::ssize(pActors));
    EXPECT_EQ(pActors[highPriestActorId].monsterInfo.id, MONSTER_CLERIC_MOON_C);
    EXPECT_EQ(pActors[highPriestActorId].uniqueNameIndex, 3);
    EXPECT_TRUE(actorCarriesItem(pActors[highPriestActorId], ITEM_ARTIFACT_CLOAK_OF_THE_SHEEP));
}

GAME_TEST(MM7MasterPromotion, OfferRequiresFirstPromotionTopicAndLightPath) {
    for (const auto &[hasFirstPromotion, lightPath, darkPath] : std::array{
             std::tuple{false, true, false}, std::tuple{true, false, false}, std::tuple{true, false, true}}) {
        SCOPED_TRACE(fmt::format("first={}, light={}, dark={}", hasFirstPromotion, lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        NPCData &hume = pNPCStats->pNPCData[humeNpcId];
        pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
        pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
        if (hasFirstPromotion)
            prepareFirstPromotionOutcome(lightPath, darkPath);

        openHumeDialogue(game, &hume);
        if (!hasFirstPromotion) {
            EXPECT_EQ(hume.dialogue_1_evt_id, firstPromotionOfferEventId);
            EXPECT_FALSE(npcEventVisible(&hume, masterOfferEventId));
            EXPECT_FALSE(selectNpcEvent(game, &hume, masterOfferEventId));
        } else {
            ASSERT_TRUE(npcEventVisible(&hume, masterOfferEventId));
            ASSERT_TRUE(selectNpcEvent(game, &hume, masterOfferEventId));
            EXPECT_EQ(hume.dialogue_1_evt_id, masterOfferEventId);
        }
        EXPECT_FALSE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_KILLED]);
        EXPECT_FALSE(npcEventVisible(&hume, masterReturnEventId));
        leaveDialogue(game);
    }
}

GAME_TEST(MM7MasterPromotion, HighPriestKillCloakAndReturnPromoteOnce) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    prepareFirstPromotionOutcome(true, false);
    NPCData &hume = pNPCStats->pNPCData[humeNpcId];
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classesBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                      pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};
    const int goldBefore = pParty->GetGold();

    openHumeDialogue(game, &hume);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &hume, masterOfferEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_ACTIVE]);
    EXPECT_EQ(hume.dialogue_1_evt_id, masterReturnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    leaveDialogue(game);

    openHumeDialogue(game, &hume);
    ASSERT_TRUE(selectNpcEvent(game, &hume, masterReturnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_KILLED]);
    EXPECT_EQ(hume.dialogue_1_evt_id, masterReturnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    leaveDialogue(game);

    enterTempleOfBaa(game);
    ASSERT_LT(highPriestActorId, std::ssize(pActors));
    Actor &highPriest = pActors[highPriestActorId];
    ASSERT_TRUE(actorCarriesItem(highPriest, ITEM_ARTIFACT_CLOAK_OF_THE_SHEEP));
    ASSERT_FALSE(highPriest.IsNotAlive());
    prepareCombatParty();
    mm7::testing::CombatResult combat = mm7::testing::fightActorWithAttackInput(
        game, highPriestActorId, [&] { return approachHighPriest(game); }, 400, 10, 5, 120);
    ASSERT_TRUE(combat.observedHpDecrease) << mm7::testing::describeCombatResult(combat);
    ASSERT_TRUE(combat.reachedTerminalDeath) << mm7::testing::describeCombatResult(combat);
    ASSERT_TRUE(combat.reachedDeadState) << mm7::testing::describeCombatResult(combat);
    lootHighPriestCloak(game);
    ASSERT_TRUE(partyHasItem(ITEM_ARTIFACT_CLOAK_OF_THE_SHEEP));
    EXPECT_FALSE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_KILLED]);

    leaveTempleOfBaa(game);
    ASSERT_TRUE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_KILLED]);
    ASSERT_TRUE(partyHasItem(ITEM_ARTIFACT_CLOAK_OF_THE_SHEEP));
    const std::array experienceBeforeReturn = {
        pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(experienceBeforeReturn[i] - experienceBefore[i], 300);
    const int goldBeforeReturn = pParty->GetGold();
    EXPECT_GE(goldBeforeReturn, goldBefore);

    openHumeDialogue(game, &hume);
    ASSERT_TRUE(selectNpcEvent(game, &hume, masterReturnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_HIGH_PRIEST_OF_BAA_KILLED]);
    EXPECT_EQ(hume.dialogue_1_evt_id, 0u);
    EXPECT_EQ(hume.greetingIndex, masterGreetingId);
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 7500);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -10);
    EXPECT_TRUE(partyHasItem(ITEM_ARTIFACT_CLOAK_OF_THE_SHEEP));

    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_PROMOTION_MASTER);
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_MASTER);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBeforeReturn[0], 80000);
    for (int i = 1; i < 4; ++i) {
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_MASTER_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i].classType, classesBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBeforeReturn[i], 40000);
    }

    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classesAfter = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                     pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    const int goldAfter = pParty->GetGold();
    const int reputationAfter = pParty->GetPartyReputation();
    leaveDialogue(game);
    openHumeDialogue(game, &hume);
    EXPECT_FALSE(npcEventVisible(&hume, masterReturnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &hume, masterReturnEventId));
    EXPECT_EQ(pParty->GetGold(), goldAfter);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationAfter);
    EXPECT_TRUE(partyHasItem(ITEM_ARTIFACT_CLOAK_OF_THE_SHEEP));
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i].classType, classesAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}
