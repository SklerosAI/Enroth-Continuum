#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <ranges>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int catherineId = 69;
constexpr int parsonId = 70;
constexpr int lorenId = 71;
constexpr int falseLorenId = 72;

enum class WarJobOutcome {
    REAL,
    DECOY,
    EXPIRED,
};

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    for (float direction : {1.0f, -1.0f}) {
        Vec3f position = face.boundingBox.center() + direction * 150 * face.facePlane.normal;
        int sectorId = 0;
        int floorFaceId = -1;
        float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
        if (floorZ == -30000 || floorFaceId == -1)
            continue;
        position.z = floorZ;
        int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                         -direction * face.facePlane.normal.x) *
                              180 / std::numbers::pi);
        return {position, yaw};
    }
    return {face.boundingBox.center(), 0};
}

bool eventChangesQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Add || instruction.opcode == EVENT_Set ||
            instruction.opcode == EVENT_Subtract) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool eventChangesQuestBit(int eventId, QuestBit bit) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [bit](const EvtInstruction &instruction) {
               return eventChangesQuestBit(instruction, bit);
           });
}

unsigned int findNpcEvent(int npcId, auto predicate) {
    const NPCData &npc = pNPCStats->pNPCData[npcId];
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, int npcId, unsigned int eventId) {
    NPCData &npc = pNPCStats->pNPCData[npcId];
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
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
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

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openThroneRoom(EngineController &game, MapId map, HouseId house, int npcId) {
    loadMapAtPartyStart(game, map);
    auto door = std::ranges::find_if(pIndoor->faces, [house](const BLVFace &face) {
        return face.Clickable() && eventOpensHouse(face.eventId, house);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(map, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), house);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", npc - houseNpcs.begin()));
        game.tick();
    }
}

void openCatherine(EngineController &game) {
    openThroneRoom(game, MAP_CASTLE_GRYPHONHEART, HOUSE_CASTLE_GRYPHONHEART_THRONE_ROOM, catherineId);
}

void openParson(EngineController &game) {
    openThroneRoom(game, MAP_CASTLE_NAVAN, HOUSE_CASTLE_NAVAN_THRONE_ROOM, parsonId);
}

void acquireLoren(EngineController &game) {
    loadMapAtPartyStart(game, MAP_TULAREAN_CAVES);
    auto cell = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && eventChangesQuestBit(face.eventId, QBIT_LOREN_STEEL_FREED);
    });
    ASSERT_NE(cell, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*cell);
    game.teleportTo(MAP_TULAREAN_CAVES, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    EXPECT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_FREED]);
    EXPECT_TRUE(pNPCStats->pNPCData[lorenId].Hired());
    if (current_screen_type == SCREEN_NPC_DIALOGUE) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
}

void acquireRiverstridePlans(EngineController &game) {
    loadMapAtPartyStart(game, MAP_FORT_RIVERSTRIDE);
    auto vault = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && eventChangesQuestBit(face.eventId, QBIT_RIVERSTRIDE_VAULT_OPENED);
    });
    ASSERT_NE(vault, pIndoor->faces.end());
    auto [vaultPosition, vaultYaw] = approachFace(*vault);
    game.teleportTo(MAP_FORT_RIVERSTRIDE, vaultPosition, vaultYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick(30);
    ASSERT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_VAULT_OPENED]);

    auto plans = std::ranges::find(pSpriteObjects, ITEM_MESSAGE_RIVERSTRIDE_PLANS,
                                   [](const SpriteObject &object) { return object.containing_item.itemId; });
    ASSERT_NE(plans, pSpriteObjects.end());
    const int spriteId = plans - pSpriteObjects.begin();
    bool taken = false;
    for (auto [offset, yaw] : {std::pair(Vec3f(0, -200, 0), 90), std::pair(Vec3f(-200, 0, 0), 0),
                               std::pair(Vec3f(0, 200, 0), 270), std::pair(Vec3f(200, 0, 0), 180)}) {
        Vec3f position = plans->vPosition + offset;
        int sectorId = 0;
        int floorFaceId = -1;
        float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
        if (floorZ == -30000 || floorFaceId == -1)
            continue;
        position.z = floorZ;
        game.teleportTo(MAP_FORT_RIVERSTRIDE, position, yaw, -10);
        try {
            game.pointMouseAtSprite(spriteId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        taken = pParty->pPickedItem.itemId == ITEM_MESSAGE_RIVERSTRIDE_PLANS ||
                pParty->hasItem(ITEM_MESSAGE_RIVERSTRIDE_PLANS);
        if (taken)
            break;
    }
    ASSERT_TRUE(taken);
    if (pParty->pPickedItem.itemId == ITEM_MESSAGE_RIVERSTRIDE_PLANS)
        pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(ITEM_MESSAGE_RIVERSTRIDE_PLANS));
}

int totalExperience() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.experience;
    return result;
}

void prepareWarJobs() {
    pParty->_questBits.set(QBIT_HARMONDALE_REBUILT);
    pParty->uNumGoldInBank = 1000;
    for (int i = 0; i < 4; ++i)
        pParty->pCharacters[i].experience = 1000 + i * 100;
}

} // namespace

GAME_TEST(MM7WarAlternatives, LorenRealDecoyAndExpiredOutcomes) {
    for (WarJobOutcome outcome : {WarJobOutcome::REAL, WarJobOutcome::DECOY, WarJobOutcome::EXPIRED}) {
        SCOPED_TRACE(fmt::format("outcome={}", std::to_underlying(outcome)));
        test.prepareForNextTest();
        game.startNewGame();
        prepareWarJobs();
        const int initialExperience = totalExperience();
        openCatherine(game);
        const unsigned int offer = findNpcEvent(catherineId, [](const EvtInstruction &instruction) {
            return eventChangesQuestBit(instruction, QBIT_LOREN_STEEL_RESCUE_ACTIVE) &&
                   instruction.opcode != EVENT_Subtract;
        });
        ASSERT_TRUE(selectNpcEvent(game, catherineId, offer));
        ASSERT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUE_ACTIVE]);
        ASSERT_TRUE(pParty->_questBits[QBIT_TALKED_TO_CATHERINE]);
        leaveHouse(game);

        if (outcome == WarJobOutcome::REAL) {
            acquireLoren(game);
        } else if (outcome == WarJobOutcome::DECOY) {
            openParson(game);
            const unsigned int decoy = findNpcEvent(parsonId, [](const EvtInstruction &instruction) {
                return eventChangesQuestBit(instruction, QBIT_FALSE_LOREN_ACTIVE) &&
                       instruction.opcode != EVENT_Subtract;
            });
            ASSERT_TRUE(selectNpcEvent(game, parsonId, decoy));
            ASSERT_TRUE(pParty->_questBits[QBIT_FALSE_LOREN_ACTIVE]);
            ASSERT_TRUE(pNPCStats->pNPCData[falseLorenId].Hired());
            leaveHouse(game);
        } else {
            pParty->GetPlayingTime() += Duration::fromDays(29);
        }

        openCatherine(game);
        const unsigned int completion = pNPCStats->pNPCData[catherineId].dialogue_1_evt_id;
        ASSERT_TRUE(selectNpcEvent(game, catherineId, completion));
        EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUE_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_ACTIVE]);
        EXPECT_FALSE(pNPCStats->pNPCData[lorenId].Hired());
        EXPECT_FALSE(pNPCStats->pNPCData[falseLorenId].Hired());

        if (outcome == WarJobOutcome::REAL) {
            EXPECT_TRUE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 6000);
            EXPECT_EQ(totalExperience(), initialExperience + 40000);
        } else if (outcome == WarJobOutcome::DECOY) {
            EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
            EXPECT_TRUE(pParty->_questBits[QBIT_FALSE_LOREN_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        } else {
            EXPECT_FALSE(pParty->_questBits[QBIT_LOREN_STEEL_RESCUED]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_LOREN_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        }
        for (const Character &character : pParty->pCharacters) {
            EXPECT_EQ(character._achievedAwardsBits[AWARD_ERATHIA_RESCUED_LOREN_STEEL],
                      outcome == WarJobOutcome::REAL);
            EXPECT_EQ(character._achievedAwardsBits[AWARD_AVLEE_BROUGHT_FAKE_LOREN_STEEL_TO_ERATHIA],
                      outcome == WarJobOutcome::DECOY);
        }
    }
}

GAME_TEST(MM7WarAlternatives, RiverstrideRealDecoyAndExpiredOutcomes) {
    for (WarJobOutcome outcome : {WarJobOutcome::REAL, WarJobOutcome::DECOY, WarJobOutcome::EXPIRED}) {
        SCOPED_TRACE(fmt::format("outcome={}", std::to_underlying(outcome)));
        test.prepareForNextTest();
        game.startNewGame();
        prepareWarJobs();
        const int initialExperience = totalExperience();
        openParson(game);
        const unsigned int offer = findNpcEvent(parsonId, [](const EvtInstruction &instruction) {
            return eventChangesQuestBit(instruction, QBIT_RIVERSTRIDE_PLANS_ACTIVE) &&
                   instruction.opcode != EVENT_Subtract;
        });
        ASSERT_TRUE(selectNpcEvent(game, parsonId, offer));
        ASSERT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_ACTIVE]);
        ASSERT_TRUE(pParty->_questBits[QBIT_TALKED_TO_ELF_KING]);
        leaveHouse(game);

        if (outcome == WarJobOutcome::REAL) {
            acquireRiverstridePlans(game);
        } else if (outcome == WarJobOutcome::DECOY) {
            openCatherine(game);
            const unsigned int decoy = findNpcEvent(catherineId, [](const EvtInstruction &instruction) {
                return eventChangesQuestBit(instruction, QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE) &&
                       instruction.opcode != EVENT_Subtract;
            });
            ASSERT_TRUE(selectNpcEvent(game, catherineId, decoy));
            ASSERT_TRUE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE]);
            ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS);
            pParty->placeHeldItemInInventoryOrDrop();
            leaveHouse(game);
        } else {
            pParty->GetPlayingTime() += Duration::fromDays(29);
        }

        openParson(game);
        const unsigned int completion = pNPCStats->pNPCData[parsonId].dialogue_1_evt_id;
        ASSERT_TRUE(selectNpcEvent(game, parsonId, completion));
        EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_ACTIVE]);
        EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_ACTIVE]);
        EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_RIVERSTRIDE_PLANS));
        EXPECT_FALSE(pParty->hasItem(ITEM_MESSAGE_FALSE_RIVERSTRIDE_PLANS));

        if (outcome == WarJobOutcome::REAL) {
            EXPECT_TRUE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 6000);
            EXPECT_EQ(totalExperience(), initialExperience + 40000);
        } else if (outcome == WarJobOutcome::DECOY) {
            EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_TRUE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        } else {
            EXPECT_FALSE(pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_FALSE(pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN]);
            EXPECT_EQ(pParty->uNumGoldInBank, 1000);
            EXPECT_EQ(totalExperience(), initialExperience);
        }
        for (const Character &character : pParty->pCharacters) {
            EXPECT_EQ(character._achievedAwardsBits[AWARD_AVLEE_BROUGHT_FORT_RIVERSTRIDE_PLANS],
                      outcome == WarJobOutcome::REAL);
            EXPECT_EQ(character._achievedAwardsBits[AWARD_ERATHIA_DELIVERED_FAKE_PLANS_TO_ELVES],
                      outcome == WarJobOutcome::DECOY);
        }
    }
}
