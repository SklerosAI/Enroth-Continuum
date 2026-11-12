#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <ranges>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int malwickId = 6;
constexpr int nilesId = 99;
constexpr int malwickCounterIndex = 4;

enum class MalwickDeadline {
    VISIT_NILES,
    RETURN_TAPESTRY,
};

bool eventWritesQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add ||
            instruction.opcode == EVENT_Subtract) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

bool eventSpeaksNpc(int eventId, int npcId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [npcId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakNPC && instruction.data.npc_descr.npc_id == npcId;
           });
}

unsigned int findNpcEvent(const NPCData &npc, auto predicate) {
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, NPCData &npc, unsigned int eventId) {
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(&npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    const char *prefix = current_screen_type == SCREEN_NPC_DIALOGUE ? "NpcDialogue_Option" : "HouseNpcDialogue_Option";
    game.pressGuiButton(fmt::format("{}{}", prefix, option - topics.begin()));
    game.tick();
    return true;
}

void loadMapAtPartyStart(EngineController &game, MapId map) {
    game.runGameRoutine([map] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_SKIP_NEXT_WORLD_UPDATE;
        uGameState = GAME_STATE_CHANGE_LOCATION;
        onMapLeave();
    });
    game.tick();
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, map);
}

void leaveDialogue(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openOutdoorNpc(EngineController &game, MapId map, int npcId) {
    loadMapAtPartyStart(game, map);
    auto actor = std::ranges::find(pActors, npcId, &Actor::npcId);
    if (actor != pActors.end()) {
        bool pointed = false;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(200, 0, 0), 180}, std::pair{Vec3f(-200, 0, 0), 0},
                 std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(0, -200, 0), 90}}) {
            game.teleportTo(map, actor->pos + offset, yaw);
            try {
                game.pointMouseAtActor(actor->id);
                pointed = true;
                break;
            } catch (const Exception &) {
            }
        }
        ASSERT_TRUE(pointed);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
        return;
    }
    auto decoration = std::ranges::find_if(pLevelDecorations, [npcId](const LevelDecoration &candidate) {
        return candidate.uEventID != 0 && eventSpeaksNpc(candidate.uEventID, npcId);
    });
    if (decoration != pLevelDecorations.end()) {
        const int decorationId = decoration - pLevelDecorations.begin();
        bool pointed = false;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(map, decoration->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
                pointed = true;
                break;
            } catch (const Exception &) {
            }
        }
        ASSERT_TRUE(pointed);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
    } else {
        auto face = std::ranges::find_if(pOutdoor->faces, [npcId](const BLVFace &candidate) {
            return candidate.Clickable() && eventSpeaksNpc(candidate.eventId, npcId);
        });
        ASSERT_NE(face, pOutdoor->faces.end());
        for (float direction : {1.0f, -1.0f}) {
            Vec3f position = face->boundingBox.center() + direction * 150 * face->facePlane.normal;
            position.z = face->boundingBox.z1;
            int yaw = std::lround(std::atan2(-direction * face->facePlane.normal.y,
                                            -direction * face->facePlane.normal.x) *
                                  180 / std::numbers::pi);
            game.teleportTo(map, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            if (current_screen_type == SCREEN_NPC_DIALOGUE)
                break;
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
}

void returnToHarmondale(EngineController &game) {
    loadMapAtPartyStart(game, MAP_TATALIA);
    loadMapAtPartyStart(game, MAP_HARMONDALE);
}

void triggerNilesLetter(EngineController &game) {
    pParty->_questBits.set(QBIT_MALWICK_FIREBALL_WAND_ACCEPTED);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->PartyTimes.CounterEventValues[malwickCounterIndex] = pParty->GetPlayingTime();
    pParty->GetPlayingTime() += Duration::fromHours(1008);
    loadMapAtPartyStart(game, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());
    ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY) ||
                pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY);
    if (pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY)
        pParty->placeHeldItemInInventoryOrDrop();
    if (current_screen_type == SCREEN_NPC_DIALOGUE)
        leaveDialogue(game);
}

void acceptNilesTapestryJob(EngineController &game) {
    loadMapAtPartyStart(game, MAP_TATALIA);
    game.teleportTo(MAP_TATALIA, Vec3f(17920, 16652, 3120), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    NPCData &niles = pNPCStats->pNPCData[nilesId];
    const unsigned int tapestryOffer = findNpcEvent(niles, [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE) &&
               instruction.opcode != EVENT_Subtract;
    });
    ASSERT_TRUE(selectNpcEvent(game, niles, tapestryOffer));
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());
    leaveDialogue(game);
}

}  // namespace

GAME_TEST(MM7MalwickNiles, RefusalKeepsMalwickProposalAvailable) {
    game.startNewGame();
    NPCData &malwick = pNPCStats->pNPCData[malwickId];

    openOutdoorNpc(game, MAP_EMERALD_ISLAND, malwickId);
    const unsigned int proposal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 19;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, proposal));
    const unsigned int refusal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 18;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, refusal));
    EXPECT_FALSE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    EXPECT_FALSE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS));
    EXPECT_NE(pParty->pPickedItem.itemId, ITEM_ALACORN_WAND_OF_FIREBALLS);

    const unsigned int repeatedProposal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 19;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, repeatedProposal));
    const unsigned int acceptance = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MALWICK_FIREBALL_WAND_ACCEPTED) &&
               instruction.opcode != EVENT_Subtract;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, acceptance));
    EXPECT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    EXPECT_TRUE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS) ||
                pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS);
}

GAME_TEST(MM7MalwickNiles, SixWeekTriggerAndMissingTapestryRefusal) {
    game.startNewGame();
    NPCData &malwick = pNPCStats->pNPCData[malwickId];
    NPCData &niles = pNPCStats->pNPCData[nilesId];

    openOutdoorNpc(game, MAP_EMERALD_ISLAND, malwickId);
    const unsigned int proposal = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 19;
    });
    ASSERT_TRUE(selectNpcEvent(game, malwick, proposal));
    const unsigned int wandOffer = findNpcEvent(malwick, [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MALWICK_FIREBALL_WAND_ACCEPTED) &&
               instruction.opcode != EVENT_Subtract;
    });
    const Time acceptedAt = pParty->GetPlayingTime();
    ASSERT_TRUE(selectNpcEvent(game, malwick, wandOffer));
    ASSERT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    ASSERT_TRUE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS) ||
                pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS);
    if (pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS)
        pParty->placeHeldItemInInventoryOrDrop();
    leaveDialogue(game);

    // The production Light/Dark path choice starts this counter; that route is covered by MM7CampaignChain C3.
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->PartyTimes.CounterEventValues[malwickCounterIndex] = pParty->GetPlayingTime();
    loadMapAtPartyStart(game, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->PartyTimes.CounterEventValues[malwickCounterIndex].isValid());
    const Time countdownStartedAt = pParty->PartyTimes.CounterEventValues[malwickCounterIndex];
    EXPECT_LE(countdownStartedAt, acceptedAt);

    pParty->GetPlayingTime() = countdownStartedAt + Duration::fromHours(1007);
    returnToHarmondale(game);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY));
    EXPECT_NE(pParty->pPickedItem.itemId, ITEM_MESSAGE_FROM_MR_STANTLEY);

    pParty->GetPlayingTime() += Duration::fromHours(1);
    returnToHarmondale(game);
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());
    ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY) ||
                pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY);
    if (pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY)
        pParty->placeHeldItemInInventoryOrDrop();
    if (current_screen_type == SCREEN_NPC_DIALOGUE)
        leaveDialogue(game);

    loadMapAtPartyStart(game, MAP_TATALIA);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(502));
    game.teleportTo(MAP_TATALIA, Vec3f(17920, 16652, 3120), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);

    const unsigned int tapestryOffer = findNpcEvent(niles, [](const EvtInstruction &instruction) {
        return eventWritesQuestBit(instruction, QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE) &&
               instruction.opcode != EVENT_Subtract;
    });
    ASSERT_TRUE(selectNpcEvent(game, niles, tapestryOffer));
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[malwickCounterIndex], pParty->GetPlayingTime());

    const int reputationBefore = pParty->GetPartyReputation();
    const unsigned int handIn = niles.dialogue_1_evt_id;
    ASSERT_TRUE(selectNpcEvent(game, niles, handIn));
    EXPECT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    EXPECT_EQ(pParty->GetPartyReputation(), reputationBefore);
    EXPECT_FALSE(pParty->hasItem(ITEM_BIG_TAPESTRY));
}

GAME_TEST(MM7MalwickNiles, MissedDeadlinesStartRaidAndCloseHarmondaleServices) {
    for (MalwickDeadline deadline : {MalwickDeadline::VISIT_NILES, MalwickDeadline::RETURN_TAPESTRY}) {
        SCOPED_TRACE(fmt::format("deadline={}", std::to_underlying(deadline)));
        test.prepareForNextTest();
        game.startNewGame();
        triggerNilesLetter(game);

        int deadlineHours = 336;
        QuestBit activeBit = QBIT_MERCENARY_GUILD_VISIT_ACTIVE;
        if (deadline == MalwickDeadline::RETURN_TAPESTRY) {
            acceptNilesTapestryJob(game);
            deadlineHours = 672;
            activeBit = QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE;
        }
        const Time deadlineStartedAt = pParty->PartyTimes.CounterEventValues[malwickCounterIndex];
        pParty->uNumGoldInBank = 777;

        pParty->GetPlayingTime() = deadlineStartedAt + Duration::fromHours(deadlineHours - 1);
        loadMapAtPartyStart(game, MAP_HARMONDALE);
        ASSERT_TRUE(pParty->_questBits[activeBit]);
        ASSERT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
        ASSERT_EQ(pParty->uNumGoldInBank, 777);

        pParty->GetPlayingTime() += Duration::fromHours(1);
        loadMapAtPartyStart(game, MAP_HARMONDALE);
        ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
        EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
        EXPECT_EQ(pParty->uNumGoldInBank, 0);
        EXPECT_TRUE(std::ranges::any_of(pActors, [](Actor &actor) {
            return actor.group == 9 && !actor.IsNotAlive();
        }));
        if (current_screen_type == SCREEN_NPC_DIALOGUE)
            leaveDialogue(game);

        game.teleportTo(MAP_HARMONDALE, Vec3f(-6472, 14750, 32), 90);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        EXPECT_EQ(current_screen_type, SCREEN_GAME);
    }
}
