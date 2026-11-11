#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
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

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) && std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

bool eventSetsQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return instruction.opcode == EVENT_Set && instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

NPCData *findDavrik() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array eventIds = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                               candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return candidate.house == HOUSE_TAVERN_HARMONDALE && std::ranges::any_of(eventIds, [](unsigned int eventId) {
            return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                   std::ranges::any_of(engine->_globalEventMap.function(eventId), [](const EvtInstruction &instruction) {
                       return eventSetsQuestBit(instruction, QBIT_DAVRIKS_SIGNET_RING_ACTIVE);
                   });
        });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
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

void openDavrikTavern(EngineController &game, NPCData *davrik) {
    game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_TAVERN_HARMONDALE))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_HARMONDALE, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == HOUSE_TAVERN_HARMONDALE)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    auto entry = std::ranges::find(houseNpcs, davrik, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

bool lootSignetRing(EngineController &game) {
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId) ||
            !std::ranges::any_of(engine->_localEventMap.function(face.eventId), [](const EvtInstruction &instruction) {
                return instruction.opcode == EVENT_OpenChest;
            }))
            continue;
        const int faceId = &face - pIndoor->faces.data();
        for (float distance : {150.0f, 250.0f, 350.0f}) {
            for (auto [direction, yaw] : {std::pair(Vec3f(1, 0, 0), 180), std::pair(Vec3f(-1, 0, 0), 0),
                                          std::pair(Vec3f(0, 1, 0), 270), std::pair(Vec3f(0, -1, 0), 90)}) {
                Vec3f position = face.boundingBox.center() + distance * direction;
                const int sector = pIndoor->GetSector(position);
                int floorFace = -1;
                const float floor = sector ? BLV_GetFloorLevel(position, sector, &floorFace) : -30000;
                if (floorFace == -1 || floor > face.boundingBox.z2 || floor < face.boundingBox.z1 - 200)
                    continue;
                position.z = floor;
                game.teleportTo(MAP_BANDIT_CAVES, position, yaw, -30);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                for (int click = 0; click < 2 && current_screen_type != SCREEN_CHEST; ++click) {
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                }
                if (current_screen_type != SCREEN_CHEST)
                    continue;
                for (int i = 0; i < 40 && !pParty->hasItem(ITEM_QUEST_SIGNET_RING) && pParty->pPickedItem.itemId == ITEM_NULL; ++i) {
                    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                    game.tick();
                }
                if (pParty->hasItem(ITEM_QUEST_SIGNET_RING) || pParty->pPickedItem.itemId == ITEM_QUEST_SIGNET_RING)
                    return true;
                game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
                game.tick();
            }
        }
    }
    return false;
}

int totalExperience() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.experience;
    return result;
}

} // namespace

GAME_TEST(MM7SideQuests, DavriksSignetRingRequiresBanditCavesChest) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    NPCData *davrik = findDavrik();
    ASSERT_NE(davrik, nullptr);
    const unsigned int acceptEvent = findNpcEvent(davrik, [](const EvtInstruction &instruction) {
        return eventSetsQuestBit(instruction, QBIT_DAVRIKS_SIGNET_RING_ACTIVE);
    });
    ASSERT_NE(acceptEvent, 0u);

    const int gold = pParty->uNumGold;
    const int experience = totalExperience();
    openDavrikTavern(game, davrik);
    ASSERT_TRUE(selectNpcEvent(game, davrik, acceptEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_TAKEN]);
    leaveHouse(game);

    openDavrikTavern(game, davrik);
    // After acceptance the resident replaces the offer with one report topic.
    // Its event ID is data-driven, so follow the first visible scripted option.
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_TAKEN]);
    EXPECT_EQ(pParty->uNumGold, gold);
    EXPECT_EQ(totalExperience(), experience);
    leaveHouse(game);

    game.teleportTo(MAP_BANDIT_CAVES, Vec3f(), 0);
    ASSERT_TRUE(lootSignetRing(game));
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_SIGNET_RING) || pParty->pPickedItem.itemId == ITEM_QUEST_SIGNET_RING);
    EXPECT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_ACTIVE]);

    openDavrikTavern(game, davrik);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_TAKEN]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_SIGNET_RING));
    EXPECT_NE(pParty->pPickedItem.itemId, ITEM_QUEST_SIGNET_RING);
    EXPECT_EQ(pParty->uNumGold, gold + 5000);
    EXPECT_EQ(totalExperience(), experience + 4 * 5000);
    for (const Character &character : pParty->pCharacters)
        EXPECT_TRUE(character._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_DAVRIKS_SIGNET_RING]);
}
