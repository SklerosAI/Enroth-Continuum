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

namespace {

constexpr int seknitNpcId = 16;
constexpr int ladyCarmineNpcId = 92;
constexpr int offerEventId = 49;
constexpr int returnEventId = 50;
constexpr int celesteMapLeaveEventId = 378;

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

void openSeknitDialogue(EngineController &game, NPCData *seknit) {
    game.teleportTo(MAP_DEYJA, Vec3f(), 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_DEYJA_UNDERSHADOWS_HOME))
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
            game.teleportTo(MAP_DEYJA, position, yaw);
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
    ASSERT_EQ(seknit->house, HOUSE_DEYJA_UNDERSHADOWS_HOME);
    auto entry = std::ranges::find(houseNpcs, seknit, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void prepareAssassinParty(AwardId prerequisite, bool lightPath, bool darkPath) {
    pParty->pCharacters[0].classType = CLASS_ROGUE;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
    if (prerequisite != AWARD_INVALID)
        pParty->pCharacters[0]._achievedAwardsBits.set(prerequisite);
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
}

bool carriesDagger(const Actor &actor) {
    if (actor.carriedItemId == ITEM_SPECIAL_LADY_CARMINES_DAGGER)
        return true;
    return std::ranges::any_of(actor.items, [](const Item &item) {
        return item.itemId == ITEM_SPECIAL_LADY_CARMINES_DAGGER;
    });
}

bool partyHasDagger() {
    return pParty->hasItem(ITEM_SPECIAL_LADY_CARMINES_DAGGER) ||
           pParty->pPickedItem.itemId == ITEM_SPECIAL_LADY_CARMINES_DAGGER;
}

void killLadyCarmineWithPartyInput(EngineController &game) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);
    ASSERT_FALSE(pActors.empty());
    Actor &lady = pActors[0];
    ASSERT_EQ(lady.npcId, ladyCarmineNpcId);
    ASSERT_TRUE(carriesDagger(lady));
    ASSERT_FALSE(lady.IsNotAlive());
    ASSERT_TRUE(lady.ActorFriend());
    ASSERT_EQ(lady.GetActorsRelation(nullptr), HOSTILITY_FRIENDLY);

    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character.uLevel = 100;
    }

    const int initialHp = lady.hp;
    bool becameAggressor = false;
    game.teleportTo(MAP_CELESTE, lady.pos + Vec3f(0, -250, 0), 90);
    game.pointMouseAtActor(0);
    for (int attack = 0; attack < 400 && !lady.IsNotAlive(); ++attack) {
        SCOPED_TRACE(fmt::format("attack {}", attack));
        if (attack && attack % 10 == 0) {
            game.teleportTo(MAP_CELESTE, lady.pos + Vec3f(0, -250, 0), 90);
            game.pointMouseAtActor(0);
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(5);
        becameAggressor |= lady.ActorEnemy();
    }
    EXPECT_LT(lady.hp, initialHp);
    EXPECT_TRUE(becameAggressor);
    ASSERT_TRUE(lady.IsNotAlive()) << "remaining HP: " << lady.hp;
    for (int frame = 0; frame < 120 && lady.aiState != Dead; ++frame)
        game.tick();
    ASSERT_EQ(lady.aiState, Dead);
}

void lootLadyCarmineWithMouse(EngineController &game) {
    ASSERT_EQ(pActors[0].aiState, Dead);
    auto viewport = std::ranges::find(pPrimaryWindow->vButtons, "Game_Viewport", &GUIButton::id);
    ASSERT_NE(viewport, pPrimaryWindow->vButtons.end());
    bool corpseVisible = false;
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -250, 0), 90), std::pair(Vec3f(-250, 0, 0), 0),
                               std::pair(Vec3f(0, 250, 0), 270), std::pair(Vec3f(250, 0, 0), 180)}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(MAP_CELESTE, pActors[0].pos + offset, yaw, pitch);
            for (int y = 10; y < 470 && !corpseVisible; y += 10) {
                for (int x = 10; x < 630; x += 10) {
                    Pointi point(x, y);
                    if (!(*viewport)->Contains(point))
                        continue;
                    if (engine->PickMouse(engine->config->gameplay.MouseInteractionDepth.value(), x, y,
                                          &vis_anything_filter, &vis_face_filter).pid == Pid::actor(0)) {
                        game.moveMouse(point);
                        game.tick();
                        corpseVisible = engine->PickMouseForInteraction().pid == Pid::actor(0);
                        if (corpseVisible)
                            break;
                    }
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
    EXPECT_FALSE(carriesDagger(pActors[0]));
    ASSERT_TRUE(partyHasDagger());
}

} // namespace

GAME_TEST(MM7AssassinPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &seknit = pNPCStats->pNPCData[seknitNpcId];
    EXPECT_EQ(seknit.house, HOUSE_DEYJA_UNDERSHADOWS_HOME);
    EXPECT_EQ(seknit.dialogue_2_evt_id, offerEventId);
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, offerEventId, EVENT_Set,
                                     QBIT_LADY_CARMINE_ACTIVE));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, seknitNpcId, 1, returnEventId));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, returnEventId, EVENT_Subtract,
                                     QBIT_LADY_CARMINE_ACTIVE));
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, returnEventId, EVENT_Subtract,
                                     QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER));

    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(celesteMapLeaveEventId));
    const std::span<const EvtInstruction> leave = engine->_localEventMap.function(celesteMapLeaveEventId);
    auto killed = std::ranges::find(leave, EVENT_IsActorKilled, &EvtInstruction::opcode);
    ASSERT_NE(killed, leave.end());
    EXPECT_EQ(killed->data.actor_descr.policy, KILL_CHECK_GROUPID);
    EXPECT_EQ(killed->data.actor_descr.param, 1);
    EXPECT_EQ(killed->data.actor_descr.num, 0);
    EXPECT_TRUE(eventChangesQuestBit(engine->_localEventMap, celesteMapLeaveEventId, EVENT_Set,
                                     QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER));

    ASSERT_FALSE(pActors.empty());
    Actor &lady = pActors[0];
    EXPECT_EQ(lady.npcId, ladyCarmineNpcId);
    EXPECT_EQ(lady.monsterInfo.id, MONSTER_PEASANT_HUMAN2_FEMALE_A_C);
    EXPECT_EQ(lady.group, 22u);
    EXPECT_TRUE(lady.ActorFriend());
    EXPECT_EQ(lady.GetActorsRelation(nullptr), HOSTILITY_FRIENDLY);
    EXPECT_EQ(lady.carriedItemId, ITEM_SPECIAL_LADY_CARMINES_DAGGER);
}

GAME_TEST(MM7AssassinPromotion, LivingLadyDoesNotUnlockOracleDagger) {
    game.startNewGame();
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    ASSERT_FALSE(pActors.empty());
    ASSERT_FALSE(pActors[0].IsNotAlive());
    ASSERT_EQ(pActors[0].npcId, ladyCarmineNpcId);
    ASSERT_EQ(pActors[0].carriedItemId, ITEM_SPECIAL_LADY_CARMINES_DAGGER);
    ASSERT_TRUE(std::ranges::none_of(pActors, [](const Actor &actor) { return actor.group == 1; }));
    ASSERT_TRUE(Actor::isActorKilled(KILL_CHECK_GROUPID, 1, 0));

    engine->_persistentVariables.mapVars[0] = 1;
    pParty->_questBits.reset(QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER);
    runEvent(EvtContext(EVT_SOURCE_MAP, celesteMapLeaveEventId), 1);

    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER]);
    EXPECT_FALSE(pParty->hasItem(ITEM_SPECIAL_LADY_CARMINES_DAGGER));
    EXPECT_NE(pParty->pPickedItem.itemId, ITEM_SPECIAL_LADY_CARMINES_DAGGER);
}

GAME_TEST(MM7AssassinPromotion, KilledLadyUnlocksOracleDagger) {
    game.startNewGame();
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    ASSERT_EQ(pActors[0].npcId, ladyCarmineNpcId);
    pParty->_questBits.reset(QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER);

    Actor::Die(0);
    ASSERT_TRUE(pActors[0].IsNotAlive());
    engine->_persistentVariables.mapVars[0] = 0;
    runEvent(EvtContext(EVT_SOURCE_MAP, celesteMapLeaveEventId), 1);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER]);

    engine->_persistentVariables.mapVars[0] = 1;
    runEvent(EvtContext(EVT_SOURCE_MAP, celesteMapLeaveEventId), 1);

    EXPECT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER]);
}

GAME_TEST(MM7AssassinPromotion, OfferRequiresRogueAwardAndDarkPath) {
    const std::array cases = {
        std::tuple{AWARD_INVALID, false, true},
        std::tuple{AWARD_PROMOTION_ROGUE, false, false},
        std::tuple{AWARD_PROMOTION_ROGUE_HONORARY, true, false},
    };
    for (const auto &[prerequisite, lightPath, darkPath] : cases) {
        SCOPED_TRACE(fmt::format("award={}, light={}, dark={}", std::to_underlying(prerequisite),
                                 lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        prepareAssassinParty(prerequisite, lightPath, darkPath);
        NPCData &seknit = pNPCStats->pNPCData[seknitNpcId];
        const int goldBefore = pParty->GetGold();
        openSeknitDialogue(game, &seknit);
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &seknit, offerEventId));
        EXPECT_FALSE(pParty->_questBits[QBIT_LADY_CARMINE_ACTIVE]);
        EXPECT_EQ(seknit.dialogue_2_evt_id, offerEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }
}

GAME_TEST(MM7AssassinPromotion, KillLootAndReturnPromotesRogue) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    prepareAssassinParty(AWARD_PROMOTION_ROGUE, false, true);
    NPCData &seknit = pNPCStats->pNPCData[seknitNpcId];
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};

    openSeknitDialogue(game, &seknit);
    const int offerGold = pParty->GetGold();
    const int offerReputation = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &seknit, offerEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_LADY_CARMINE_ACTIVE]);
    EXPECT_EQ(seknit.dialogue_2_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), offerGold);
    EXPECT_EQ(pParty->GetPartyReputation(), offerReputation);
    leaveDialogue(game);

    openSeknitDialogue(game, &seknit);
    ASSERT_FALSE(partyHasDagger());
    ASSERT_TRUE(selectNpcEvent(game, &seknit, returnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_LADY_CARMINE_ACTIVE]);
    EXPECT_EQ(seknit.dialogue_2_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), offerGold);
    EXPECT_EQ(pParty->GetPartyReputation(), offerReputation);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    leaveDialogue(game);

    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    killLadyCarmineWithPartyInput(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    engine->_persistentVariables.mapVars[0] = 1;
    pParty->_questBits.reset(QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER);
    runEvent(EvtContext(EVT_SOURCE_MAP, celesteMapLeaveEventId), 1);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER]);
    lootLadyCarmineWithMouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    openSeknitDialogue(game, &seknit);
    const int goldBeforeReturn = pParty->GetGold();
    const int reputationBeforeReturn = pParty->GetPartyReputation();
    const std::array experienceBeforeReturn = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                               pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    ASSERT_TRUE(selectNpcEvent(game, &seknit, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_LADY_CARMINE_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_LADY_CARMINES_DAGGER]);
    EXPECT_EQ(seknit.dialogue_2_evt_id, 0u);
    EXPECT_EQ(seknit.greetingIndex, 42);
    EXPECT_EQ(pParty->GetGold() - goldBeforeReturn, 15000);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, 10);
    EXPECT_TRUE(partyHasDagger());

    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_ASSASSIN);
    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_PROMOTION_ASSASSIN);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBeforeReturn[0], 80000);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_ASSASSIN_HONORARY);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, expectedAwards);
        EXPECT_EQ(pParty->pCharacters[i].experience - experienceBeforeReturn[i], 40000);
    }

    leaveDialogue(game);
    const std::array experienceAfter = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                        pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsAfter = {pParty->pCharacters[0]._achievedAwardsBits,
                                    pParty->pCharacters[1]._achievedAwardsBits,
                                    pParty->pCharacters[2]._achievedAwardsBits,
                                    pParty->pCharacters[3]._achievedAwardsBits};
    const int goldAfter = pParty->GetGold();
    openSeknitDialogue(game, &seknit);
    EXPECT_FALSE(npcEventVisible(&seknit, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &seknit, returnEventId));
    EXPECT_EQ(pParty->GetGold(), goldAfter);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBeforeReturn, 10);
    EXPECT_TRUE(partyHasDagger());
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}
