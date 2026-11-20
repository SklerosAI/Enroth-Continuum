#include "MM7MalwickCampaign.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "Utility/Exception.h"

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
