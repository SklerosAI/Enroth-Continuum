#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MM7CombatHarness.h"
#include "Testing/Game/GameTest.h"

#include "Core/Time/Time.h"

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
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int sowerNpcId = 45;
constexpr int hunterCompletionEventId = 83;
constexpr int offerEventId = 81;
constexpr int returnEventId = 82;
constexpr int sowerGreetingId = 67;
constexpr int requiredBountyGold = 10000;
constexpr MonsterId bountyMonster = MONSTER_DRAGON_C;
constexpr MapId bountyTownMap = MAP_HARMONDALE;
constexpr HouseId bountyTownHall = HOUSE_TOWN_HALL_HARMONDALE;

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
    // Setup teleport: the door is resolved and clicked through the loaded map.
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

void openSowerDialogue(EngineController &game, NPCData *sower) {
    openOutdoorHouse(game, MAP_TULAREAN_FOREST, HOUSE_TULAREAN_FOREST_SOWER_RESIDENCE, sower);
}

void prepareHunterOutcome(bool honoraryPrerequisite, bool lightPath, bool darkPath) {
    const std::array classes = {CLASS_HUNTER, CLASS_KNIGHT, CLASS_HUNTER, CLASS_SORCERER};
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = classes[i];
        pParty->pCharacters[i]._achievedAwardsBits.set(
            classes[i] == CLASS_HUNTER ? AWARD_PROMOTION_HUNTER : AWARD_PROMOTION_HUNTER_HONORARY);
    }
    if (honoraryPrerequisite) {
        pParty->pCharacters[0].classType = CLASS_KNIGHT;
        pParty->pCharacters[0]._achievedAwardsBits.reset(AWARD_PROMOTION_HUNTER);
        pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_HUNTER_HONORARY);
    }
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
    // Fixture only the already-covered Ranger-to-Hunter completion topic; the adjacent control proves that route.
    pNPCStats->pNPCData[sowerNpcId].dialogue_1_evt_id = offerEventId;
}

void prepareBountyTarget() {
    pParty->PartyTimes.bountyHuntNextGenTime[bountyTownHall] =
        Time::fromMonths(pParty->GetPlayingTime().toMonths() + 1);
    pParty->monster_id_for_hunting[bountyTownHall] = bountyMonster;
    pParty->monster_for_hunting_killed[bountyTownHall] = false;
}

bool approachActorForMelee(EngineController &game, int actorId) {
    Vec3f fallbackPosition;
    int fallbackYaw = 0;
    bool hasFallback = false;
    for (float distance : {150.0f, 250.0f, 400.0f}) {
        for (const auto &[direction, yaw] : std::array{
                 std::pair{Vec3f(0, -1, 0), 90}, std::pair{Vec3f(-1, 0, 0), 0},
                 std::pair{Vec3f(0, 1, 0), 270}, std::pair{Vec3f(1, 0, 0), 180}}) {
            Vec3f position = pActors[actorId].pos + distance * direction;
            int sectorId = 0;
            int floorFaceId = -1;
            float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius),
                                           &sectorId, &floorFaceId);
            if (floorZ == -30000 || floorFaceId == -1)
                continue;
            position.z = floorZ;
            if (!hasFallback) {
                fallbackPosition = position;
                fallbackYaw = yaw;
                hasFallback = true;
            }
            for (int pitch : {-20, 0, 20}) {
                game.teleportTo(MAP_DRAGON_CAVES, position, yaw, pitch);
                try {
                    game.pointMouseAtActor(actorId);
                    return true;
                } catch (const Exception &) {
                }
            }
        }
    }
    if (!hasFallback)
        return false;
    // The object picker can reject a partially occluded dragon even though a forward melee swing still reaches it.
    game.teleportTo(MAP_DRAGON_CAVES, fallbackPosition, fallbackYaw);
    return true;
}

void killBountyTargetWithAttackInput(EngineController &game) {
    // Setup teleport: the target remains an authored Dragon Caves actor and is killed by attack input below.
    game.teleportTo(MAP_DRAGON_CAVES, Vec3f(), 0);
    auto target = std::ranges::find_if(pActors, [](const Actor &actor) {
        return actor.monsterInfo.id == bountyMonster;
    });
    ASSERT_NE(target, pActors.end());
    const int actorId = target->id;
    for (Actor &actor : pActors) {
        if (actor.id != actorId)
            actor.aiState = Disabled;
    }
    target->aiState = Standing;
    target->attributes &= ~ACTOR_UNKNOW11;
    target->attributes &= ~ACTOR_FLEEING;
    target->attributes |= ACTOR_VISIBLE | ACTOR_ACTIVE | ACTOR_NEARBY | ACTOR_FULL_AI_STATE;
    target->moveSpeed = 1;
    target->hp = 1; // Combat fixture: the promotion test proves the attack-input kill and bounty transaction.
    for (Character &character : pParty->pCharacters) {
        character._stats[ATTRIBUTE_SPEED] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character._melee_dmg_bonus = 100;
        character.health = 100000;
    }
    ASSERT_TRUE(approachActorForMelee(game, actorId));
    target->buffs[ACTOR_BUFF_PARALYZED].Apply(
        pParty->GetPlayingTime() + Duration::fromDays(1), MASTERY_GRANDMASTER, 0, 0, 0);
    target->aiState = Paralyzed;
    target->attributes &= ~ACTOR_FLEEING;
    // The point established above is kept stable by the paralysis fixture; no targeting teleport occurs in combat.
    mm7::testing::CombatResult result = mm7::testing::fightActorWithAttackInput(
        game, actorId, [] { return true; }, 20, 20, 2);
    const std::string diagnostic = mm7::testing::describeCombatResult(result);
    EXPECT_TRUE(result.targetAcquired) << diagnostic;
    EXPECT_TRUE(result.observedHpDecrease) << diagnostic;
    EXPECT_TRUE(result.reachedTerminalDeath) << diagnostic;
    EXPECT_TRUE(result.reachedDeadState) << diagnostic;
    EXPECT_TRUE(pParty->monster_for_hunting_killed[bountyTownHall]);
}

int claimBountyAtTownHall(EngineController &game) {
    ASSERT_EQ(pParty->monster_id_for_hunting[bountyTownHall], bountyMonster);
    ASSERT_TRUE(pParty->monster_for_hunting_killed[bountyTownHall]);
    const int goldBefore = pParty->GetGold();
    const int counterBefore = pParty->uNumBountiesCollected;
    const int reward = 100 * pMonsterStats->infos[bountyMonster].level;
    openOutdoorHouse(game, bountyTownMap, bountyTownHall);
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("HouseDialogue_Option0");
    game.tick();
    EXPECT_EQ(pParty->GetGold(), goldBefore + reward);
    EXPECT_EQ(pParty->uNumBountiesCollected, counterBefore + reward);
    EXPECT_EQ(pParty->monster_id_for_hunting[bountyTownHall], MONSTER_INVALID);
    EXPECT_FALSE(pParty->monster_for_hunting_killed[bountyTownHall]);
    leaveDialogue(game);
    return reward;
}

bool hasGoldOrItemMutation(const EvtInstruction &instruction) {
    if (instruction.opcode != EVENT_Add && instruction.opcode != EVENT_Set &&
        instruction.opcode != EVENT_Subtract)
        return false;
    return instruction.data.variable_descr.type == VAR_FixedGold ||
           instruction.data.variable_descr.type == VAR_RandomGold ||
           instruction.data.variable_descr.type == VAR_PlayerItemInHands;
}

} // namespace

GAME_TEST(MM7BountyHunterPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &sower = pNPCStats->pNPCData[sowerNpcId];
    EXPECT_EQ(sower.house, HOUSE_TULAREAN_FOREST_SOWER_RESIDENCE);
    EXPECT_EQ(sower.greetingIndex, 66);
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, hunterCompletionEventId, 40,
                                 sowerNpcId, 0, offerEventId));

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 0, 4, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_LIGHT_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 1, 6, EVENT_Compare,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_DARK_PATH)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, offerEventId, 7, 0, EVENT_Set,
                                     VAR_QBits_QuestsDone, std::to_underlying(QBIT_TOWN_HALL_BOUNTIES_ACTIVE)));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, 8, sowerNpcId, 0, returnEventId));

    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, 0, 3, EVENT_Compare,
                                     VAR_NumBounties, requiredBountyGold));
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Compare,
                                       VAR_Class, std::to_underlying(CLASS_HUNTER)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Set,
                                       VAR_Class, std::to_underlying(CLASS_BOUNTY_HUNTER)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add,
                                       VAR_Award, std::to_underlying(AWARD_PROMOTION_BOUNTY_HUNTER)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add,
                                       VAR_Award, std::to_underlying(AWARD_PROMOTION_BOUNTY_HUNTER_HONORARY)), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add,
                                       VAR_Experience, 80000), 4);
    EXPECT_EQ(countVariableInstruction(engine->_globalEventMap, returnEventId, EVENT_Add,
                                       VAR_Experience, 40000), 4);
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, 36, 0, EVENT_Subtract,
                                     VAR_QBits_QuestsDone,
                                     std::to_underlying(QBIT_TOWN_HALL_BOUNTIES_ACTIVE)));
    EXPECT_TRUE(eventChangesVariable(engine->_globalEventMap, returnEventId, 37, 0, EVENT_Subtract,
                                     VAR_ReputationInCurrentLocation, 10));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, returnEventId, 39, sowerNpcId, 0, 0));
    EXPECT_TRUE(eventSetsNpcGreeting(engine->_globalEventMap, returnEventId, 40,
                                    sowerNpcId, sowerGreetingId));
    EXPECT_TRUE(std::ranges::none_of(engine->_globalEventMap.function(returnEventId), hasGoldOrItemMutation));
    EXPECT_EQ(pMonsterStats->infos[bountyMonster].level, 100);
}

GAME_TEST(MM7BountyHunterPromotion, OfferRequiresHunterOutcomeAndDarkPath) {
    game.startNewGame();
    NPCData &sower = pNPCStats->pNPCData[sowerNpcId];
    openSowerDialogue(game, &sower);
    EXPECT_FALSE(npcEventVisible(&sower, offerEventId));
    EXPECT_FALSE(selectNpcEvent(game, &sower, offerEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_TOWN_HALL_BOUNTIES_ACTIVE]);
    leaveDialogue(game);

    for (const auto &[honoraryPrerequisite, lightPath, darkPath, accepted] : std::array{
             std::tuple{false, false, false, false},
             std::tuple{false, true, false, false},
             std::tuple{true, false, true, true}}) {
        SCOPED_TRACE(fmt::format("honorary={}, light={}, dark={}", honoraryPrerequisite,
                                 lightPath, darkPath));
        test.prepareForNextTest();
        game.startNewGame();
        prepareHunterOutcome(honoraryPrerequisite, lightPath, darkPath);
        NPCData &loopSower = pNPCStats->pNPCData[sowerNpcId];
        const int goldBefore = pParty->GetGold();
        const int reputationBefore = pParty->GetPartyReputation();
        openSowerDialogue(game, &loopSower);
        ASSERT_TRUE(selectNpcEvent(game, &loopSower, offerEventId));
        EXPECT_EQ(pParty->_questBits[QBIT_TOWN_HALL_BOUNTIES_ACTIVE], accepted);
        EXPECT_EQ(loopSower.dialogue_1_evt_id, accepted ? returnEventId : offerEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        EXPECT_EQ(pParty->uNumBountiesCollected, 0);
        leaveDialogue(game);
    }
}

GAME_TEST(MM7BountyHunterPromotion, PhysicalBountyPromotesOnce) {
    game.startNewGame();
    prepareHunterOutcome(false, false, true);
    NPCData &sower = pNPCStats->pNPCData[sowerNpcId];
    const std::array initialClasses = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                       pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array initialExperience = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                          pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array initialAwards = {pParty->pCharacters[0]._achievedAwardsBits,
                                      pParty->pCharacters[1]._achievedAwardsBits,
                                      pParty->pCharacters[2]._achievedAwardsBits,
                                      pParty->pCharacters[3]._achievedAwardsBits};
    const int initialGold = pParty->GetGold();

    openSowerDialogue(game, &sower);
    ASSERT_TRUE(selectNpcEvent(game, &sower, offerEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_TOWN_HALL_BOUNTIES_ACTIVE]);
    ASSERT_EQ(sower.dialogue_1_evt_id, returnEventId);
    pParty->uNumBountiesCollected = requiredBountyGold - 1;
    ASSERT_TRUE(selectNpcEvent(game, &sower, returnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_TOWN_HALL_BOUNTIES_ACTIVE]);
    EXPECT_EQ(sower.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), initialGold);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, initialExperience[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, initialAwards[i]);
    }
    pParty->uNumBountiesCollected = 0;
    leaveDialogue(game);

    prepareBountyTarget();
    killBountyTargetWithAttackInput(game);
    const int reward = claimBountyAtTownHall(game);
    ASSERT_EQ(reward, requiredBountyGold);
    ASSERT_EQ(pParty->uNumBountiesCollected, requiredBountyGold);
    EXPECT_EQ(pParty->GetGold(), initialGold + reward);
    for (const Character &character : pParty->pCharacters)
        EXPECT_TRUE(character._achievedAwardsBits[AWARD_BOUNTIES_COLLECTED]);

    const std::array experienceBeforePromotion = {
        pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsBeforePromotion = {pParty->pCharacters[0]._achievedAwardsBits,
                                              pParty->pCharacters[1]._achievedAwardsBits,
                                              pParty->pCharacters[2]._achievedAwardsBits,
                                              pParty->pCharacters[3]._achievedAwardsBits};
    openSowerDialogue(game, &sower);
    const int reputationBeforePromotion = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &sower, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_TOWN_HALL_BOUNTIES_ACTIVE]);
    EXPECT_EQ(pParty->uNumBountiesCollected, requiredBountyGold);
    EXPECT_EQ(sower.dialogue_1_evt_id, 0u);
    EXPECT_EQ(sower.greetingIndex, sowerGreetingId);
    EXPECT_EQ(pParty->GetGold(), initialGold + reward);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBeforePromotion - 10);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);

    for (int i : {0, 2}) {
        auto expectedAwards = awardsBeforePromotion[i];
        expectedAwards.set(AWARD_PROMOTION_BOUNTY_HUNTER);
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_BOUNTY_HUNTER);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBeforePromotion[i] + 80000);
    }
    for (int i : {1, 3}) {
        auto expectedAwards = awardsBeforePromotion[i];
        expectedAwards.set(AWARD_PROMOTION_BOUNTY_HUNTER_HONORARY);
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
    openSowerDialogue(game, &sower);
    EXPECT_FALSE(npcEventVisible(&sower, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &sower, returnEventId));
    EXPECT_EQ(pParty->uNumBountiesCollected, requiredBountyGold);
    EXPECT_EQ(pParty->GetGold(), finalGold);
    EXPECT_EQ(pParty->GetPartyReputation(), finalReputation);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, finalExperience[i]);
        EXPECT_EQ(pParty->pCharacters[i].classType, finalClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, finalAwards[i]);
    }
    leaveDialogue(game);
}
