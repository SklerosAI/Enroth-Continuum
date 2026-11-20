#include "MM7MalwickCampaign.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <ranges>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "Utility/Exception.h"

#include "MM7CampaignChain.h"

static bool takeCampaignTapestry(EngineController &game) {
    auto taken = [] {
        return pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_TAKEN] &&
               (pParty->hasItem(ITEM_BIG_TAPESTRY) || pParty->pPickedItem.itemId == ITEM_BIG_TAPESTRY);
    };
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != 376 || !face.Clickable())
            continue;
        const int faceId = static_cast<int>(&face - pIndoor->faces.data());
        for (float direction : {1.0f, -1.0f}) {
            Vec3f position = face.boundingBox.center() + direction * 150 * face.facePlane.normal;
            position.z = face.boundingBox.z1;
            int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                            -direction * face.facePlane.normal.x) *
                                  180 / std::numbers::pi);
            game.teleportTo(MAP_CASTLE_LAMBENT, position, yaw);
            try {
                game.pointMouseAtFace(faceId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (taken())
                return true;
        }
    }
    return clickCampaignChainEventFace(game, MAP_CASTLE_LAMBENT, 376, 1, taken);
}

void acceptCampaignMalwickWand(EngineController &game) {
    constexpr int malwickId = 6;
    game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(16154, 8560, 128), 180);
    auto actor = std::ranges::find(pActors, malwickId, &Actor::npcId);
    ASSERT_NE(actor, pActors.end());
    bool pointed = false;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(200, 0, 0), 180}, std::pair{Vec3f(-200, 0, 0), 0},
             std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(0, -200, 0), 90}}) {
        game.teleportTo(MAP_EMERALD_ISLAND, actor->pos + offset, yaw);
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

    NPCData &malwick = pNPCStats->pNPCData[malwickId];
    auto selectEvent = [&](auto predicate) {
        std::array eventIds = {malwick.dialogue_1_evt_id, malwick.dialogue_2_evt_id, malwick.dialogue_3_evt_id,
                               malwick.dialogue_4_evt_id, malwick.dialogue_5_evt_id, malwick.dialogue_6_evt_id};
        auto event = std::ranges::find_if(eventIds, [&](unsigned int id) {
            return id != 0 && engine->_globalEventMap.hasEvent(id) &&
                   std::ranges::any_of(engine->_globalEventMap.function(id), predicate);
        });
        ASSERT_NE(event, eventIds.end());
        if (event == eventIds.end())
            return;
        DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
        auto topics = prepareScriptedNPCDialogueTopics(&malwick);
        auto option = std::ranges::find(topics, line);
        ASSERT_NE(option, topics.end());
        if (option == topics.end())
            return;
        game.pressGuiButton(fmt::format("NpcDialogue_Option{}", option - topics.begin()));
        game.tick();
    };
    selectEvent([](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == malwickId &&
               instruction.data.npc_topic_descr.event_id == 19;
    });
    selectEvent([](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_MALWICK_FIREBALL_WAND_ACCEPTED);
    });
    ASSERT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    ASSERT_TRUE(pParty->hasItem(ITEM_ALACORN_WAND_OF_FIREBALLS) || pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS);
    if (pParty->pPickedItem.itemId == ITEM_ALACORN_WAND_OF_FIREBALLS)
        pParty->placeHeldItemInInventoryOrDrop();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.teleportTo(MAP_HARMONDALE, Vec3f(-17331, 12547, 465), 0);
}

void completeCampaignMalwickTapestry(EngineController &game) {
    constexpr int nilesId = 99;
    ASSERT_TRUE(pParty->_questBits[QBIT_MALWICK_FIREBALL_WAND_ACCEPTED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    ASSERT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    const Time startedAt = pParty->PartyTimes.CounterEventValues[4];
    ASSERT_TRUE(startedAt.isValid());

    // The clock advances to the exact six-week boundary; map entry runs the production trigger.
    game.teleportTo(MAP_TATALIA, Vec3f(), 0);
    pParty->GetPlayingTime() = startedAt + Duration::fromHours(1008);
    game.teleportTo(MAP_HARMONDALE, Vec3f(-17331, 12547, 465), 0);
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_VISIT_ACTIVE]);
    ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_FROM_MR_STANTLEY) ||
                pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[4], pParty->GetPlayingTime());
    if (pParty->pPickedItem.itemId == ITEM_MESSAGE_FROM_MR_STANTLEY)
        pParty->placeHeldItemInInventoryOrDrop();
    if (current_screen_type == SCREEN_NPC_DIALOGUE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    NPCData &niles = pNPCStats->pNPCData[nilesId];
    auto talkToNiles = [&] {
        game.teleportTo(MAP_TATALIA, Vec3f(17920, 16652, 3120), 90);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    };
    auto selectEvent = [&](auto predicate) {
        std::array eventIds = {niles.dialogue_1_evt_id, niles.dialogue_2_evt_id, niles.dialogue_3_evt_id,
                               niles.dialogue_4_evt_id, niles.dialogue_5_evt_id, niles.dialogue_6_evt_id};
        auto event = std::ranges::find_if(eventIds, [&](unsigned int id) {
            return id != 0 && engine->_globalEventMap.hasEvent(id) &&
                   std::ranges::any_of(engine->_globalEventMap.function(id), predicate);
        });
        ASSERT_NE(event, eventIds.end());
        if (event == eventIds.end())
            return;
        DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
        auto topics = prepareScriptedNPCDialogueTopics(&niles);
        auto option = std::ranges::find(topics, line);
        ASSERT_NE(option, topics.end());
        if (option == topics.end())
            return;
        game.pressGuiButton(fmt::format("NpcDialogue_Option{}", option - topics.begin()));
        game.tick();
    };
    auto leaveNiles = [&] {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
    };

    talkToNiles();
    selectEvent([](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OnCanShowDialogItemCmp &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_LIGHT_PATH);
    });
    ASSERT_TRUE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    ASSERT_EQ(pParty->PartyTimes.CounterEventValues[4], pParty->GetPlayingTime());
    leaveNiles();

    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    ASSERT_EQ(engine->_persistentVariables.mapVars[4], 0);
    ASSERT_TRUE(takeCampaignTapestry(game));
    if (pParty->pPickedItem.itemId == ITEM_BIG_TAPESTRY)
        pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(ITEM_BIG_TAPESTRY));

    talkToNiles();
    const int reputation = pParty->GetPartyReputation();
    const int gold = pParty->uNumGold;
    const int bankGold = pParty->uNumGoldInBank;
    selectEvent([](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_BIG_TAPESTRY);
    });
    EXPECT_FALSE(pParty->hasItem(ITEM_BIG_TAPESTRY));
    EXPECT_TRUE(pParty->_questBits[QBIT_MALWICK_QUESTS_FINISHED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_TAPESTRY_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_MERCENARY_GUILD_QUEST_FAILED]);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation + 5);
    EXPECT_EQ(pParty->uNumGold, gold);
    EXPECT_EQ(pParty->uNumGoldInBank, bankGold);
    leaveNiles();

    // Keep the C5 predecessor's authored starting map while saving the completed side quest.
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
}
