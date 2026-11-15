#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <tuple>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int laskerNpcId = 15;
constexpr int rogueOfferEventId = 44;
constexpr int spyOfferEventId = 46;
constexpr int spyReturnEventId = 47;
constexpr int spyGreetingIndex = 39;
constexpr int counterweightEventId = 376;

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) *
                          180 / std::numbers::pi);
    return {position, yaw};
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

void openLaskerHouse(EngineController &game, NPCData *lasker) {
    game.teleportTo(MAP_ERATHIAN_SEWERS, Vec3f(), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && eventOpensHouse(face.eventId, HOUSE_ERATHIAN_SEWERS_MASTER_THIEF);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_ERATHIAN_SEWERS, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_ERATHIAN_SEWERS_MASTER_THIEF);
    auto entry = std::ranges::find(houseNpcs, lasker, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

int pointAtCounterweightFace(EngineController &game) {
    if (engine->_currentLoadedMapId != MAP_WATCHTOWER_6)
        game.teleportTo(MAP_WATCHTOWER_6, Vec3f(), 0);

    for (int faceId = 0; faceId < std::ssize(pIndoor->faces); ++faceId) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.eventId != counterweightEventId)
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
                game.teleportTo(MAP_WATCHTOWER_6, position, yaw);
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

void moveCounterweight(EngineController &game) {
    ASSERT_NE(pointAtCounterweightFace(game), -1);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_PLACED]);
}

void setSpyPromotionSetup(NPCData *lasker) {
    lasker->dialogue_1_evt_id = spyOfferEventId;
    pParty->pCharacters[0].classType = CLASS_ROGUE;
    pParty->pCharacters[1].classType = CLASS_ROGUE;
    pParty->pCharacters[2].classType = CLASS_KNIGHT;
    pParty->pCharacters[3].classType = CLASS_THIEF;
}

} // namespace

GAME_TEST(MM7SpyPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &lasker = pNPCStats->pNPCData[laskerNpcId];
    EXPECT_EQ(lasker.house, HOUSE_ERATHIAN_SEWERS_MASTER_THIEF);
    EXPECT_EQ(lasker.dialogue_1_evt_id, rogueOfferEventId);

    ASSERT_TRUE(engine->_globalEventMap.hasEvent(spyOfferEventId));
    const std::span<const EvtInstruction> offer = engine->_globalEventMap.function(spyOfferEventId);
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 8 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_LIGHT_PATH);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.target_step == 4 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_DARK_PATH);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 9 && instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_WATCHTOWER_6_WEIGHT_ACTIVE);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 10 && instruction.opcode == EVENT_SetNPCTopic &&
               instruction.data.npc_topic_descr.npc_id == laskerNpcId &&
               instruction.data.npc_topic_descr.index == 0 &&
               instruction.data.npc_topic_descr.event_id == spyReturnEventId;
    }));

    ASSERT_TRUE(engine->_globalEventMap.hasEvent(spyReturnEventId));
    const std::span<const EvtInstruction> handIn = engine->_globalEventMap.function(spyReturnEventId);
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 10 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_WATCHTOWER_6_WEIGHT_PLACED);
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 43 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_FixedGold &&
               instruction.data.variable_descr.value == 15000;
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 46 && instruction.opcode == EVENT_SetNPCTopic &&
               instruction.data.npc_topic_descr.npc_id == laskerNpcId &&
               instruction.data.npc_topic_descr.index == 0 && instruction.data.npc_topic_descr.event_id == 0;
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 47 && instruction.opcode == EVENT_SetNPCGreeting &&
               instruction.data.npc_descr.npc_id == laskerNpcId &&
               instruction.data.npc_descr.greeting == spyGreetingIndex;
    }));

    game.teleportTo(MAP_WATCHTOWER_6, Vec3f(), 0);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(counterweightEventId));
    const std::span<const EvtInstruction> counterweight = engine->_localEventMap.function(counterweightEventId);
    EXPECT_TRUE(std::ranges::any_of(counterweight, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_WATCHTOWER_6_WEIGHT_PLACED);
    }));
}

GAME_TEST(MM7SpyPromotion, OfferRequiresLightPath) {
    for (const auto &[lightPath, darkPath, expectedText] : std::array{
             std::tuple{false, false, 59}, std::tuple{false, true, 58}}) {
        SCOPED_TRACE(fmt::format("light={}, dark={}", lightPath, darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        NPCData *lasker = &pNPCStats->pNPCData[laskerNpcId];
        setSpyPromotionSetup(lasker);
        pParty->_questBits.set(QBIT_LIGHT_PATH, lightPath);
        pParty->_questBits.set(QBIT_DARK_PATH, darkPath);
        const int gold = pParty->GetGold();
        const int reputation = pParty->GetPartyReputation();
        openLaskerHouse(game, lasker);

        ASSERT_TRUE(selectNpcEvent(game, lasker, spyOfferEventId));

        EXPECT_EQ(current_npc_text, pNPCTopics[expectedText - 1].pText);
        EXPECT_FALSE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_ACTIVE]);
        EXPECT_EQ(lasker->dialogue_1_evt_id, spyOfferEventId);
        EXPECT_EQ(pParty->GetGold(), gold);
        EXPECT_EQ(pParty->GetPartyReputation(), reputation);
        leaveHouse(game);
    }
}

GAME_TEST(MM7SpyPromotion, ReturnTopicRefusesMissingCounterweight) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    NPCData *lasker = &pNPCStats->pNPCData[laskerNpcId];
    setSpyPromotionSetup(lasker);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    const int gold = pParty->GetGold();
    const int reputation = pParty->GetPartyReputation();
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; ++i)
        experience[i] = pParty->pCharacters[i].experience;
    openLaskerHouse(game, lasker);
    ASSERT_TRUE(selectNpcEvent(game, lasker, spyOfferEventId));
    ASSERT_TRUE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_ACTIVE]);
    ASSERT_EQ(lasker->dialogue_1_evt_id, spyReturnEventId);

    ASSERT_TRUE(selectNpcEvent(game, lasker, spyReturnEventId));

    EXPECT_EQ(current_npc_text, pNPCTopics[62].pText);
    EXPECT_TRUE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_PLACED]);
    EXPECT_EQ(lasker->dialogue_1_evt_id, spyReturnEventId);
    EXPECT_EQ(pParty->GetGold(), gold);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_SPY]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_SPY_HONORARY]);
    }
    leaveHouse(game);
}

GAME_TEST(MM7SpyPromotion, PhysicalCounterweightPromotesRoguesAndHonorsCompanionsOnce) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    NPCData *lasker = &pNPCStats->pNPCData[laskerNpcId];
    setSpyPromotionSetup(lasker);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    std::array initialClasses = {CLASS_ROGUE, CLASS_ROGUE, CLASS_KNIGHT, CLASS_THIEF};
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; ++i)
        experience[i] = pParty->pCharacters[i].experience;
    const int gold = pParty->GetGold();
    const int reputation = pParty->GetPartyReputation();

    openLaskerHouse(game, lasker);
    ASSERT_TRUE(selectNpcEvent(game, lasker, spyOfferEventId));
    EXPECT_EQ(current_npc_text, pNPCTopics[59].pText);
    ASSERT_TRUE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_ACTIVE]);
    ASSERT_EQ(lasker->dialogue_1_evt_id, spyReturnEventId);
    leaveHouse(game);

    game.teleportTo(MAP_WATCHTOWER_6, Vec3f(), 0);
    ASSERT_FALSE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_PLACED]);
    moveCounterweight(game);

    openLaskerHouse(game, lasker);
    ASSERT_TRUE(selectNpcEvent(game, lasker, spyReturnEventId));

    EXPECT_EQ(current_npc_text, pNPCTopics[63].pText);
    EXPECT_FALSE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WATCHTOWER_6_WEIGHT_PLACED]);
    EXPECT_EQ(pParty->GetGold(), gold + 15000);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation);
    EXPECT_EQ(lasker->dialogue_1_evt_id, 0u);
    EXPECT_EQ(lasker->greetingIndex, spyGreetingIndex);
    EXPECT_EQ(pNPCStats->pNPCGreetings[spyGreetingIndex].pGreeting1,
              "Greetings Spies, how may I be of service?");
    EXPECT_EQ(pNPCStats->pNPCGreetings[spyGreetingIndex].pGreeting2,
              "Greetings Spies, how may I be of service?");
    for (int i = 0; i < 4; ++i) {
        if (initialClasses[i] == CLASS_ROGUE) {
            EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_SPY);
            EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 80000);
            EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_SPY]);
            EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_SPY_HONORARY]);
        } else {
            EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
            EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 40000);
            EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_SPY]);
            EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_SPY_HONORARY]);
        }
    }

    std::array<uint64_t, 4> promotedExperience;
    for (int i = 0; i < 4; ++i)
        promotedExperience[i] = pParty->pCharacters[i].experience;
    leaveHouse(game);
    openLaskerHouse(game, lasker);
    EXPECT_FALSE(selectNpcEvent(game, lasker, spyReturnEventId));
    EXPECT_EQ(pParty->GetGold(), gold + 15000);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, promotedExperience[i]);
    leaveHouse(game);
}
