#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
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

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) && std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

bool eventGrantsVase(const EvtProgram &program, int eventId) {
    if (!program.hasEvent(eventId))
        return false;
    return std::ranges::any_of(program.function(eventId), [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_VASE);
    });
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

NPCData *findHouseNpc(HouseId houseId) {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [houseId](const NPCData &candidate) {
        return candidate.house == houseId;
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

unsigned int findNpcEvent(NPCData *npc, auto predicate) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
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

} // namespace

GAME_TEST(MM7Promotions, MarkhamVaseRequiresPhysicalPickup) {
    auto activeTape = tapes.questBit(QBIT_MARKHAM_VASE_ACTIVE);
    auto vaseTape = tapes.questBit(QBIT_MARKHAM_VASE_TAKEN);
    auto goldTape = tapes.gold();
    game.startNewGame();
    Character &thief = pParty->pCharacters[0];
    Character &knight = pParty->pCharacters[1];
    thief.classType = CLASS_THIEF;
    knight.classType = CLASS_KNIGHT;
    NPCData *lasker = findHouseNpc(HOUSE_ERATHIAN_SEWERS_MASTER_THIEF);
    ASSERT_NE(lasker, nullptr);
    unsigned int offerEvent = findNpcEvent(lasker, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_MARKHAM_VASE_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);

    openLaskerHouse(game, lasker);
    test.startTaping();
    ASSERT_TRUE(selectNpcEvent(game, lasker, offerEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_MARKHAM_VASE_ACTIVE]);
    unsigned int handInEvent = findNpcEvent(lasker, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_VASE);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);
    openLaskerHouse(game, lasker);
    EXPECT_TRUE(selectNpcEvent(game, lasker, handInEvent)); // The visible return topic refuses the missing vase.
    EXPECT_TRUE(pParty->_questBits[QBIT_MARKHAM_VASE_ACTIVE]);
    EXPECT_EQ(thief.classType, CLASS_THIEF);
    EXPECT_EQ(knight.classType, CLASS_KNIGHT);
    EXPECT_EQ(goldTape.delta(), 0);
    leaveHouse(game);

    game.teleportTo(MAP_LORD_MARKHAMS_MANOR, Vec3f(), 0);
    auto givesVase = [](int eventId) {
        return eventGrantsVase(engine->_localEventMap, eventId) || eventGrantsVase(engine->_globalEventMap, eventId);
    };
    auto vase = std::ranges::find_if(pLevelDecorations, [&givesVase](const LevelDecoration &decoration) {
        return decoration.uEventID != 0 && givesVase(decoration.uEventID);
    });
    ASSERT_NE(vase, pLevelDecorations.end());
    const int vaseId = vase - pLevelDecorations.begin();
    bool pointed = false;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
             std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
        game.teleportTo(MAP_LORD_MARKHAMS_MANOR, vase->vPosition + offset, yaw);
        try {
            game.pointMouseAtDecoration(vaseId);
            pointed = true;
            break;
        } catch (const Exception &) {
        }
    }
    ASSERT_TRUE(pointed);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_MARKHAM_VASE_TAKEN]);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_VASE); // The fixed quest prop remains held for its return event.
    const uint64_t thiefExperience = thief.experience;
    const uint64_t knightExperience = knight.experience;

    openLaskerHouse(game, lasker);
    ASSERT_TRUE(selectNpcEvent(game, lasker, handInEvent));
    EXPECT_EQ(activeTape, tape(true, false));
    EXPECT_EQ(vaseTape, tape(false, true));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_VASE));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(goldTape.delta(), 5000);
    EXPECT_EQ(thief.classType, CLASS_ROGUE);
    EXPECT_EQ(knight.classType, CLASS_KNIGHT);
    EXPECT_TRUE(thief._achievedAwardsBits[AWARD_PROMOTION_ROGUE]);
    EXPECT_TRUE(knight._achievedAwardsBits[AWARD_PROMOTION_ROGUE_HONORARY]);
    EXPECT_EQ(thief.experience - thiefExperience, 30000);
    EXPECT_EQ(knight.experience - knightExperience, 15000);
}
