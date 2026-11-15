#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

namespace {

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool opensHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    const auto &function = engine->_localEventMap.function(eventId);
    return std::ranges::any_of(function, [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

void openAdvisorHouse(EngineController &game) {
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && opensHouse(face.eventId, HOUSE_CELESTE_HOSTEL_4);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CELESTE_HOSTEL_4);
}

void selectAdvisorPortrait(EngineController &game) {
    auto advisor = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) {
        return npc.npc == &pNPCStats->pNPCData[83];
    });
    ASSERT_NE(advisor, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", advisor - houseNpcs.begin()));
        game.tick();
    }
}

bool selectAdvisorEvent(EngineController &game, unsigned int eventId) {
    NPCData &advisor = pNPCStats->pNPCData[83];
    std::array eventIds = {advisor.dialogue_1_evt_id, advisor.dialogue_2_evt_id, advisor.dialogue_3_evt_id,
                           advisor.dialogue_4_evt_id, advisor.dialogue_5_evt_id, advisor.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(&advisor);
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

bool hasVariableInstruction(const std::vector<EvtInstruction> &function, EvtOpcode opcode, EvtVariable variable,
                            std::optional<int> value = std::nullopt) {
    return std::ranges::any_of(function, [=](const EvtInstruction &instruction) {
        return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
               (!value || instruction.data.variable_descr.value == *value);
    });
}

bool hasNpcTopicInstruction(const std::vector<EvtInstruction> &function, int npcId, int index, int eventId) {
    return std::ranges::any_of(function, [=](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == npcId &&
               instruction.data.npc_topic_descr.index == index && instruction.data.npc_topic_descr.event_id == eventId;
    });
}

}  // namespace

GAME_TEST(MM7LightAssassination, RobertDoorRequiresAdvisors) {
    for (bool allAdvisorsFinished : {false, true}) {
        SCOPED_TRACE(fmt::format("allAdvisorsFinished={}", allAdvisorsFinished));
        test.prepareForNextTest();
        game.startNewGame();
        engine->config->debug.NoDamage.setValue(true);
        pParty->_questBits.set(QBIT_LIGHT_PATH);
        pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
        pParty->_questBits.set(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
        pParty->_questBits.set(QBIT_WINE_CELLAR_FINISHED);
        if (allAdvisorsFinished)
            pParty->_questBits.set(QBIT_SOUL_JARS_LIGHT_FINISHED);
        auto movedTape = tapes.questBit(QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB);
        test.startTaping();

        game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
        auto robertDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
            return face.eventId == 376 && face.Clickable();
        });
        ASSERT_NE(robertDoor, pIndoor->faces.end());
        auto [position, yaw] = approachFace(*robertDoor);
        game.teleportTo(MAP_CELESTE, position, yaw);
        ASSERT_FALSE(pParty->_questBits[QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB]);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        EXPECT_EQ(pParty->_questBits[QBIT_ARCHIBALD_MOVED_TO_CLANKERS_LAB], allAdvisorsFinished);
        EXPECT_EQ(movedTape, allAdvisorsFinished ? tape(false, true) : tape(false));
        test.stopTaping();
    }
}

GAME_TEST(MM7LightAssassination, AdvisorPortraitIndex) {
    game.startNewGame();
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pNPCStats->pNPCData[80].house = HOUSE_CELESTE_HOSTEL_4;
    pNPCStats->pNPCData[83].house = HOUSE_CELESTE_HOSTEL_4;

    openAdvisorHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    auto advisor = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) {
        return npc.npc == &pNPCStats->pNPCData[83];
    });
    ASSERT_NE(advisor, houseNpcs.end());
    ASSERT_GE(houseNpcs.size(), 2u);
    const auto index = advisor - houseNpcs.begin();
    ASSERT_EQ(index, 1);
    game.pressGuiButton(fmt::format("House_Npc{}", index));
    game.tick();
    ASSERT_GE(currentHouseNpc, 0);
    EXPECT_EQ(houseNpcs[currentHouseNpc].npc, &pNPCStats->pNPCData[83]);
}

GAME_TEST(MM7LightAssassination, RewardSourceDeclaresFullTransaction) {
    game.startNewGame();
    const std::vector<EvtInstruction> &reward = engine->_globalEventMap.function(156);
    const EvtVariable history21 = static_cast<EvtVariable>(std::to_underlying(VAR_History_0) + 21);

    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Compare, VAR_PlayerItemInHands,
                                       std::to_underlying(ITEM_QUEST_CONTROL_CUBE)));
    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Subtract, VAR_PlayerItemInHands,
                                       std::to_underlying(ITEM_QUEST_CONTROL_CUBE)));
    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Add, history21));
    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Add, VAR_Experience, 250000));
    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Subtract, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_TOLBERTI_ASSASSINATION_ACTIVE)));
    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Add, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_TOLBERTI_ASSASSINATED)));
    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Add, VAR_Award, std::to_underlying(AWARD_CELESTIA_KILLED_TOLBERTI)));
    EXPECT_EQ(std::ranges::count_if(reward, [](const EvtInstruction &instruction) {
                  return instruction.opcode == EVENT_Set && instruction.data.variable_descr.type == VAR_BlasterSkill &&
                         instruction.data.variable_descr.value == 1;
              }),
              4);
    EXPECT_TRUE(hasVariableInstruction(reward, EVENT_Subtract, VAR_ReputationInCurrentLocation, 5));
    EXPECT_TRUE(hasNpcTopicInstruction(reward, 80, 3, 223));
    EXPECT_TRUE(hasNpcTopicInstruction(reward, 81, 1, 222));
    EXPECT_TRUE(hasNpcTopicInstruction(reward, 82, 1, 221));
    EXPECT_TRUE(hasNpcTopicInstruction(reward, 83, 0, 0));
    EXPECT_TRUE(std::ranges::any_of(reward, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCGreeting && instruction.data.npc_descr.npc_id == 83 &&
               instruction.data.npc_descr.greeting == 0;
    }));
    EXPECT_TRUE(std::ranges::any_of(reward, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_MoveNPC && instruction.data.npc_move_descr.npc_id == 80 &&
               instruction.data.npc_move_descr.location_id == HOUSE_CASTLE_LAMBENT_THRONE_ROOM;
    }));
}

GAME_TEST(MM7LightAssassination, RefusalFullRewardAndDuplicateProtection) {
    game.startNewGame();
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_LIGHT_GUILD_JOINED);
    pParty->_questBits.set(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
    pParty->_questBits.set(QBIT_WINE_CELLAR_FINISHED);
    pParty->_questBits.set(QBIT_SOUL_JARS_LIGHT_FINISHED);
    pNPCStats->pNPCData[83].house = HOUSE_CELESTE_HOSTEL_4;

    std::array initialSkills = {CombinedSkillValue::none(), CombinedSkillValue::novice(),
                                CombinedSkillValue(7, MASTERY_EXPERT), CombinedSkillValue(10, MASTERY_GRANDMASTER)};
    for (int i = 0; i < 4; ++i)
        pParty->pCharacters[i].setSkillValue(SKILL_BLASTER, initialSkills[i]);

    openAdvisorHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    selectAdvisorPortrait(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectAdvisorEvent(game, 155));
    ASSERT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);

    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; ++i)
        experience[i] = pParty->pCharacters[i].experience;
    const int reputation = pParty->GetPartyReputation();
    const HouseId resurectraHouse = pNPCStats->pNPCData[80].house;
    const unsigned int resurectraFinalTopic = pNPCStats->pNPCData[80].dialogue_4_evt_id;
    const unsigned int wineAdvisorFinalTopic = pNPCStats->pNPCData[81].dialogue_2_evt_id;
    const unsigned int soulJarAdvisorFinalTopic = pNPCStats->pNPCData[82].dialogue_2_evt_id;
    const int robertGreeting = pNPCStats->pNPCData[83].greetingIndex;
    ASSERT_FALSE(pParty->PartyTimes.HistoryEventTimes[21].isValid());

    ASSERT_TRUE(selectAdvisorEvent(game, 156));
    EXPECT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation);
    EXPECT_EQ(pNPCStats->pNPCData[80].house, resurectraHouse);
    EXPECT_EQ(pNPCStats->pNPCData[80].dialogue_4_evt_id, resurectraFinalTopic);
    EXPECT_EQ(pNPCStats->pNPCData[81].dialogue_2_evt_id, wineAdvisorFinalTopic);
    EXPECT_EQ(pNPCStats->pNPCData[82].dialogue_2_evt_id, soulJarAdvisorFinalTopic);
    EXPECT_EQ(pNPCStats->pNPCData[83].greetingIndex, robertGreeting);
    EXPECT_EQ(pNPCStats->pNPCData[83].dialogue_1_evt_id, 156u);
    EXPECT_FALSE(pParty->PartyTimes.HistoryEventTimes[21].isValid());
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_CELESTIA_KILLED_TOLBERTI]);
        EXPECT_EQ(pParty->pCharacters[i].getSkillValue(SKILL_BLASTER), initialSkills[i]);
    }

    // MM7PitCubeAcquisition.KillLootAndHandIn proves this item comes from Tolberti through combat and corpse looting.
    // Seed that already-proven physical reward here so this focused test can exhaustively inspect the UI transaction.
    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_CONTROL_CUBE));
    ASSERT_TRUE(selectAdvisorEvent(game, 156));

    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_CONTROL_CUBE));
    EXPECT_FALSE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);
    EXPECT_TRUE(pParty->PartyTimes.HistoryEventTimes[21].isValid());
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 5);
    EXPECT_EQ(pNPCStats->pNPCData[80].house, HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    EXPECT_EQ(pNPCStats->pNPCData[80].dialogue_4_evt_id, 223u);
    EXPECT_EQ(pNPCStats->pNPCData[81].dialogue_2_evt_id, 222u);
    EXPECT_EQ(pNPCStats->pNPCData[82].dialogue_2_evt_id, 221u);
    EXPECT_EQ(pNPCStats->pNPCData[83].greetingIndex, 0);
    EXPECT_EQ(pNPCStats->pNPCData[83].dialogue_1_evt_id, 0u);
    std::array expectedSkills = {CombinedSkillValue::novice(), CombinedSkillValue::novice(), initialSkills[2], initialSkills[3]};
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 250000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_CELESTIA_KILLED_TOLBERTI]);
        EXPECT_EQ(pParty->pCharacters[i].getSkillValue(SKILL_BLASTER), expectedSkills[i]);
    }

    const Time historyTime = pParty->PartyTimes.HistoryEventTimes[21];
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_CONTROL_CUBE));
    openAdvisorHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    selectAdvisorPortrait(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    EXPECT_FALSE(selectAdvisorEvent(game, 156));
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_CONTROL_CUBE));
    EXPECT_EQ(pParty->PartyTimes.HistoryEventTimes[21], historyTime);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 5);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 250000);
        EXPECT_EQ(pParty->pCharacters[i].getSkillValue(SKILL_BLASTER), expectedSkills[i]);
    }
}
