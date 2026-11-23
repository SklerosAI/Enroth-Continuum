#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"
#include "Engine/Mods/PrivateMods.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int ledaNpcId = 42;
constexpr int offerEventId = 71;
constexpr int returnEventId = 72;

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
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

void openLedaDialogue(EngineController &game, NPCData *leda) {
    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(), 0);
    std::vector<int> doorIds;
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_BRACADA_DESERT_ROWANS_HOUSE))
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
        ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_BRACADA_DESERT_ROWANS_HOUSE);
    }
    auto entry = std::ranges::find(houseNpcs, leda, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (current_screen_type == SCREEN_HOUSE && houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void prepareChampionParty(AwardId prerequisite, bool lightPath, bool darkPath) {
    pParty->pCharacters[0].classType = CLASS_CAVALIER;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_KNIGHT;
    if (prerequisite != AWARD_INVALID)
        pParty->pCharacters[0]._achievedAwardsBits.set(prerequisite);
    pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
    pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
}

void runSuccessfulPromotion(EngineController &game, int knightWins, int lordWins, AwardId prerequisite) {
    engine->config->debug.NoActors.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    prepareChampionParty(prerequisite, true, false);
    pParty->uNumArenaWins[ARENA_LEVEL_KNIGHT] = knightWins;
    pParty->uNumArenaWins[ARENA_LEVEL_LORD] = lordWins;

    NPCData &leda = pNPCStats->pNPCData[ledaNpcId];
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array classBefore = {pParty->pCharacters[0].classType, pParty->pCharacters[1].classType,
                                    pParty->pCharacters[2].classType, pParty->pCharacters[3].classType};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};
    const int goldBefore = pParty->GetGold();

    openLedaDialogue(game, &leda);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &leda, offerEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_ARENA_FIVE_WINS_ACTIVE]);
    EXPECT_EQ(leda.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    leaveDialogue(game);

    openLedaDialogue(game, &leda);
    ASSERT_TRUE(selectNpcEvent(game, &leda, returnEventId));
    EXPECT_FALSE(pParty->_questBits[QBIT_ARENA_FIVE_WINS_ACTIVE]);
    EXPECT_EQ(leda.dialogue_1_evt_id, 0u);
    EXPECT_EQ(leda.greetingIndex, 61);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -10);
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_CHAMPION);
    auto expectedAwards = awardsBefore[0];
    expectedAwards.set(AWARD_PROMOTION_CHAMPION);
    EXPECT_EQ(pParty->pCharacters[0]._achievedAwardsBits, expectedAwards);
    EXPECT_EQ(pParty->pCharacters[0].experience - experienceBefore[0], 80000);
    for (int i = 1; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, classBefore[i]);
        expectedAwards = awardsBefore[i];
        expectedAwards.set(AWARD_PROMOTION_CHAMPION_HONORARY);
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
    openLedaDialogue(game, &leda);
    EXPECT_FALSE(npcEventVisible(&leda, returnEventId));
    EXPECT_FALSE(selectNpcEvent(game, &leda, returnEventId));
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation() - reputationBefore, -10);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceAfter[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsAfter[i]);
    }
    leaveDialogue(game);
}

void runReturnRefusal(EngineController &game, int pageWins, int squireWins, int knightWins, int lordWins) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    prepareChampionParty(AWARD_PROMOTION_CHEVALIER, true, false);
    pParty->uNumArenaWins[ARENA_LEVEL_PAGE] = pageWins;
    pParty->uNumArenaWins[ARENA_LEVEL_SQUIRE] = squireWins;
    pParty->uNumArenaWins[ARENA_LEVEL_KNIGHT] = knightWins;
    pParty->uNumArenaWins[ARENA_LEVEL_LORD] = lordWins;

    NPCData &leda = pNPCStats->pNPCData[ledaNpcId];
    const std::array experienceBefore = {pParty->pCharacters[0].experience, pParty->pCharacters[1].experience,
                                         pParty->pCharacters[2].experience, pParty->pCharacters[3].experience};
    const std::array awardsBefore = {pParty->pCharacters[0]._achievedAwardsBits,
                                     pParty->pCharacters[1]._achievedAwardsBits,
                                     pParty->pCharacters[2]._achievedAwardsBits,
                                     pParty->pCharacters[3]._achievedAwardsBits};
    const int goldBefore = pParty->GetGold();

    openLedaDialogue(game, &leda);
    const int reputationBefore = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, &leda, offerEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_ARENA_FIVE_WINS_ACTIVE]);
    ASSERT_EQ(leda.dialogue_1_evt_id, returnEventId);
    leaveDialogue(game);

    openLedaDialogue(game, &leda);
    ASSERT_TRUE(selectNpcEvent(game, &leda, returnEventId));
    EXPECT_TRUE(pParty->_questBits[QBIT_ARENA_FIVE_WINS_ACTIVE]);
    EXPECT_EQ(leda.dialogue_1_evt_id, returnEventId);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experienceBefore[i]);
        EXPECT_EQ(pParty->pCharacters[i]._achievedAwardsBits, awardsBefore[i]);
    }
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_CAVALIER);
    leaveDialogue(game);
}

} // namespace

GAME_TEST(MM7ChampionPromotion, DragoonNoKnightLightPromotion) {
    if (!mm7::mods::dragoonClassEnabled(engine->gameTitle()))
        GTEST_SKIP() << "Requires ENROTH_MM7_DRAGOON_CLASS=1";
    game.startNewGame();
    prepareChampionParty(AWARD_PROMOTION_CHEVALIER, true, false);
    pParty->pCharacters[0].classType = CLASS_LANCER;
    for (int i = 1; i < 4; ++i)
        pParty->pCharacters[i].classType = CLASS_CLERIC;
    pParty->uNumArenaWins[ARENA_LEVEL_KNIGHT] = 5;
    NPCData &leda = pNPCStats->pNPCData[ledaNpcId];
    openLedaDialogue(game, &leda);
    ASSERT_TRUE(selectNpcEvent(game, &leda, offerEventId));
    ASSERT_TRUE(selectNpcEvent(game, &leda, returnEventId));
    EXPECT_EQ(pParty->pCharacters[0].classType, CLASS_DRAGON_KNIGHT);
    for (int i = 1; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_CLERIC);
    leaveDialogue(game);
}

GAME_TEST(MM7ChampionPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &leda = pNPCStats->pNPCData[ledaNpcId];
    EXPECT_EQ(leda.house, HOUSE_BRACADA_DESERT_ROWANS_HOUSE);
    EXPECT_EQ(leda.dialogue_1_evt_id, offerEventId);
    EXPECT_TRUE(eventChangesQuestBit(engine->_globalEventMap, offerEventId, EVENT_Set,
                                     QBIT_ARENA_FIVE_WINS_ACTIVE));
    EXPECT_TRUE(eventSetsNpcTopic(engine->_globalEventMap, offerEventId, ledaNpcId, 0, returnEventId));

    ASSERT_TRUE(engine->_globalEventMap.hasEvent(returnEventId));
    const std::span<const EvtInstruction> event = engine->_globalEventMap.function(returnEventId);
    ASSERT_FALSE(event.empty());
    EXPECT_EQ(event.front().step, 0);
    EXPECT_EQ(event.front().opcode, EVENT_Compare);
    EXPECT_EQ(event.front().data.variable_descr.type, VAR_ArenaWinsKnight);
    EXPECT_EQ(event.front().data.variable_descr.value, 5);
}

GAME_TEST(MM7ChampionPromotion, CombinedKnightAndLordWinsPromote) {
    const std::array cases = {
        std::tuple{5, 0, AWARD_PROMOTION_CHEVALIER},
        std::tuple{0, 5, AWARD_PROMOTION_CHEVALIER_HONORARY},
        std::tuple{4, 1, AWARD_PROMOTION_CHEVALIER},
    };
    for (const auto &[knightWins, lordWins, prerequisite] : cases) {
        SCOPED_TRACE(fmt::format("knight={}, lord={}", knightWins, lordWins));
        test.prepareForNextTest();
        runSuccessfulPromotion(game, knightWins, lordWins, prerequisite);
    }
}

GAME_TEST(MM7ChampionPromotion, InsufficientAndLowerTierWinsRefuse) {
    const std::array cases = {
        std::tuple{0, 0, 2, 2},
        std::tuple{5, 5, 0, 0},
    };
    for (const auto &[pageWins, squireWins, knightWins, lordWins] : cases) {
        SCOPED_TRACE(fmt::format("page={}, squire={}, knight={}, lord={}",
                                 pageWins, squireWins, knightWins, lordWins));
        test.prepareForNextTest();
        runReturnRefusal(game, pageWins, squireWins, knightWins, lordWins);
    }
}

GAME_TEST(MM7ChampionPromotion, OfferRequiresCavalierAwardAndLightPath) {
    const std::array cases = {
        std::tuple{AWARD_INVALID, true, false},
        std::tuple{AWARD_PROMOTION_CHEVALIER, false, false},
        std::tuple{AWARD_PROMOTION_CHEVALIER, false, true},
    };
    for (const auto &[prerequisite, lightPath, darkPath] : cases) {
        SCOPED_TRACE(fmt::format("award={}, light={}, dark={}", std::to_underlying(prerequisite),
                                 lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        prepareChampionParty(prerequisite, lightPath, darkPath);
        NPCData &leda = pNPCStats->pNPCData[ledaNpcId];
        const int goldBefore = pParty->GetGold();
        openLedaDialogue(game, &leda);
        const int reputationBefore = pParty->GetPartyReputation();
        ASSERT_TRUE(selectNpcEvent(game, &leda, offerEventId));
        EXPECT_FALSE(pParty->_questBits[QBIT_ARENA_FIVE_WINS_ACTIVE]);
        EXPECT_EQ(leda.dialogue_1_evt_id, offerEventId);
        EXPECT_EQ(pParty->GetGold(), goldBefore);
        EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
        leaveDialogue(game);
    }
}
