#include <array>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
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

bool opensKerridHouse(int eventId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse &&
                      instruction.data.house_id == HOUSE_TULAREAN_FOREST_KERRID_RESIDENCE;
           });
}

bool offersKerridLetter(const EvtInstruction &instruction) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(QBIT_FAERIE_KING_LETTER_ACTIVE);
}

NPCData *findKerrid() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array events = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                             candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return candidate.house == HOUSE_TULAREAN_FOREST_KERRID_RESIDENCE &&
               std::ranges::any_of(events, [](unsigned int eventId) {
                   return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                          std::ranges::any_of(engine->_globalEventMap.function(eventId), offersKerridLetter);
               });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

unsigned int letterOfferEvent(const NPCData *kerrid) {
    std::array events = {kerrid->dialogue_1_evt_id, kerrid->dialogue_2_evt_id, kerrid->dialogue_3_evt_id,
                         kerrid->dialogue_4_evt_id, kerrid->dialogue_5_evt_id, kerrid->dialogue_6_evt_id};
    auto event = std::ranges::find_if(events, [](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), offersKerridLetter);
    });
    EXPECT_NE(event, events.end());
    return event == events.end() ? 0 : *event;
}

void openKerridHouse(EngineController &game, NPCData *kerrid) {
    game.teleportTo(MAP_TULAREAN_FOREST, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces)
        if (face.Clickable() && opensKerridHouse(face.eventId))
            doors.push_back(approachFace(face));
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_TULAREAN_FOREST, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE &&
            window_SpeakInHouse->houseId() == HOUSE_TULAREAN_FOREST_KERRID_RESIDENCE)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    auto entry = std::ranges::find(houseNpcs, kerrid, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

bool selectOffer(EngineController &game, NPCData *kerrid, unsigned int eventId) {
    std::array events = {kerrid->dialogue_1_evt_id, kerrid->dialogue_2_evt_id, kerrid->dialogue_3_evt_id,
                         kerrid->dialogue_4_evt_id, kerrid->dialogue_5_evt_id, kerrid->dialogue_6_evt_id};
    auto event = std::ranges::find(events, eventId);
    if (event == events.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - events.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(kerrid);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

} // namespace

GAME_TEST(MM7SideQuests, KerridLetterIsReceivedThroughRealHouseDialogue) {
    game.startNewGame();
    NPCData *kerrid = findKerrid();
    ASSERT_NE(kerrid, nullptr);
    unsigned int event = letterOfferEvent(kerrid);
    ASSERT_NE(event, 0u);
    openKerridHouse(game, kerrid);
    ASSERT_TRUE(selectOffer(game, kerrid, event));
    EXPECT_TRUE(pParty->_questBits[QBIT_FAERIE_KING_LETTER_ACTIVE]);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_LETTER_FROM_JOHANN_KERRID) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_LETTER_FROM_JOHANN_KERRID);
}
