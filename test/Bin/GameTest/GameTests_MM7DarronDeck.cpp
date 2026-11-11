#include <algorithm>
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
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool opensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) && std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

bool setsDarronQuest(const EvtInstruction &instruction) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(QBIT_DARRONS_BROTHER_ACTIVE);
}

NPCData *findDarron() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array ids = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                          candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return candidate.house == HOUSE_HARMONDALE_TEMPER_RESIDENCE && std::ranges::any_of(ids, [](unsigned int id) {
            return id != 0 && engine->_globalEventMap.hasEvent(id) &&
                   std::ranges::any_of(engine->_globalEventMap.function(id), setsDarronQuest);
        });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

unsigned int offerEvent(NPCData *npc) {
    std::array ids = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                      npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find_if(ids, [](unsigned int id) {
        return id != 0 && engine->_globalEventMap.hasEvent(id) &&
               std::ranges::any_of(engine->_globalEventMap.function(id), setsDarronQuest);
    });
    EXPECT_NE(event, ids.end());
    return event == ids.end() ? 0 : *event;
}

void openDarronHouse(EngineController &game, NPCData *darron) {
    game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces)
        if (face.Clickable() && opensHouse(face.eventId, HOUSE_HARMONDALE_TEMPER_RESIDENCE))
            doors.push_back(approachFace(face));
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_HARMONDALE, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == HOUSE_HARMONDALE_TEMPER_RESIDENCE)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    auto entry = std::ranges::find(houseNpcs, darron, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

bool selectOffer(EngineController &game, NPCData *npc, unsigned int eventId) {
    std::array ids = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                      npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(ids, eventId);
    if (event == ids.end())
        return false;
    const DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - ids.begin()));
    const auto topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

int deckSprite() {
    auto sprite = std::ranges::find_if(pSpriteObjects, [](const SpriteObject &object) {
        return object.uObjectDescID != 0 && object.containing_item.itemId == ITEM_QUEST_ARCOMAGE_DECK;
    });
    return sprite == pSpriteObjects.end() ? -1 : sprite - pSpriteObjects.begin();
}

bool pickUpDeck(EngineController &game, int spriteId) {
    const Vec3f deckPosition = pSpriteObjects[spriteId].vPosition;
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -200, 0), 90), std::pair(Vec3f(-200, 0, 0), 0),
                               std::pair(Vec3f(0, 200, 0), 270), std::pair(Vec3f(200, 0, 0), 180)}) {
        game.teleportTo(MAP_WHITE_CLIFF_CAVE, deckPosition + offset, yaw, -10);
        try {
            game.pointMouseAtSprite(spriteId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        if (pParty->hasItem(ITEM_QUEST_ARCOMAGE_DECK) || pParty->pPickedItem.itemId == ITEM_QUEST_ARCOMAGE_DECK)
            return true;
    }
    return false;
}

} // namespace

GAME_TEST(MM7SideQuests, DarronsDeckIsPhysicallyAcquiredAfterAcceptance) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    game.teleportTo(MAP_WHITE_CLIFF_CAVE, Vec3f(), 0);
    ASSERT_GE(deckSprite(), 0);

    NPCData *darron = findDarron();
    ASSERT_NE(darron, nullptr);
    const unsigned int event = offerEvent(darron);
    ASSERT_NE(event, 0u);
    openDarronHouse(game, darron);
    ASSERT_TRUE(selectOffer(game, darron, event));
    ASSERT_TRUE(pParty->_questBits[QBIT_DARRONS_BROTHER_ACTIVE]);

    game.teleportTo(MAP_WHITE_CLIFF_CAVE, Vec3f(), 0);
    const int sprite = deckSprite();
    ASSERT_GE(sprite, 0);
    EXPECT_TRUE(pSpriteObjects[sprite].containing_item.itemId == ITEM_QUEST_ARCOMAGE_DECK);
    ASSERT_TRUE(pickUpDeck(game, sprite));
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_ARCOMAGE_DECK) || pParty->pPickedItem.itemId == ITEM_QUEST_ARCOMAGE_DECK);
    EXPECT_EQ(deckSprite(), -1);
}
