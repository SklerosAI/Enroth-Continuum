#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnumFunctions.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/Exception.h"

namespace {

constexpr int sandNpcId = 39;
constexpr int ninjaOfferEventId = 63;
constexpr int ninjaReturnEventId = 64;
constexpr int ninjaGreetingIndex = 55;
constexpr int scrollOfWavesEventId = 197;
constexpr int hiddenTombPuzzleFirstEventId = 402;
constexpr int hiddenTombRevealEventId = 406;
constexpr int hiddenTombEntranceEventId = 508;
constexpr int hiddenTombEntranceCog = 17;
constexpr int sarcophagusEventId = 181;
constexpr int sarcophagusChestId = 6;
constexpr int hiddenTombPuzzleMapVariable = 9;

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

void loadMapAtPartyStart(EngineController &game, MapId map) {
    if (engine->_currentLoadedMapId == map)
        return;
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

void openSandHouse(EngineController &game, NPCData *sand) {
    loadMapAtPartyStart(game, MAP_PIT);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pIndoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_PIT_SAND_RESIDENCE))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_PIT, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE)
            break;
        if (current_screen_type == SCREEN_NPC_DIALOGUE) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_PIT_SAND_RESIDENCE);
    auto entry = std::ranges::find(houseNpcs, sand, &HouseNpcDesc::npc);
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

bool pointAtDecorationEvent(EngineController &game, MapId map, int eventId) {
    auto decoration = std::ranges::find(pLevelDecorations, eventId, &LevelDecoration::uEventID);
    if (decoration == pLevelDecorations.end())
        return false;
    const int decorationId = decoration - pLevelDecorations.begin();
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(250, 0, 0), 180}, std::pair{Vec3f(-250, 0, 0), 0},
             std::pair{Vec3f(0, 250, 0), 270}, std::pair{Vec3f(0, -250, 0), 90},
             std::pair{Vec3f(400, 0, 0), 180}, std::pair{Vec3f(-400, 0, 0), 0}}) {
        game.teleportTo(map, decoration->vPosition + offset, yaw);
        try {
            game.pointMouseAtDecoration(decorationId);
            return true;
        } catch (const Exception &) {
        }
    }
    return false;
}

bool pointAtFaceEvent(EngineController &game, MapId map, int eventId) {
    const bool indoor = isMapIndoor(map);
    const std::span<const BLVFace> faces = indoor ? std::span<const BLVFace>(pIndoor->faces) :
                                                   std::span<const BLVFace>(pOutdoor->faces);
    for (int faceId = 0; faceId < std::ssize(faces); ++faceId) {
        const BLVFace &face = faces[faceId];
        if (face.eventId != eventId)
            continue;
        if (!indoor) {
            for (const auto &[offset, yaw] : std::array{
                     std::pair{Vec3f(200, 0, 50), 180}, std::pair{Vec3f(-200, 0, 50), 0},
                     std::pair{Vec3f(0, 200, 50), 270}, std::pair{Vec3f(0, -200, 50), 90},
                     std::pair{Vec3f(400, 0, 50), 180}, std::pair{Vec3f(-400, 0, 50), 0},
                     std::pair{Vec3f(0, 400, 50), 270}, std::pair{Vec3f(0, -400, 50), 90}}) {
                game.teleportTo(map, face.boundingBox.center() + offset, yaw, -30);
                try {
                    game.pointMouseAtFace(faceId);
                    return true;
                } catch (const Exception &) {
                }
            }
        }
        for (float distance : {100.0f, 175.0f, 300.0f}) {
            for (float direction : {1.0f, -1.0f}) {
                Vec3f position = face.boundingBox.center() + direction * distance * face.facePlane.normal;
                if (indoor) {
                    int sectorId = 0;
                    int floorFaceId = -1;
                    float floorZ = GetIndoorFloorZ(position + Vec3f(0, 0, pParty->radius), &sectorId, &floorFaceId);
                    if (floorZ == -30000 || floorFaceId == -1)
                        continue;
                    position.z = floorZ;
                } else {
                    position.z = face.boundingBox.z1 + 50;
                }
                int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                 -direction * face.facePlane.normal.x) *
                                      180 / std::numbers::pi);
                game.teleportTo(map, position, yaw);
                try {
                    game.pointMouseAtFace(faceId);
                    return true;
                } catch (const Exception &) {
                }
            }
        }
    }
    return false;
}

void clickMapEvent(EngineController &game, MapId map, int eventId) {
    SCOPED_TRACE(fmt::format("map={}, event={}", std::to_underlying(map), eventId));
    loadMapAtPartyStart(game, map);
    ASSERT_TRUE(pointAtDecorationEvent(game, map, eventId) || pointAtFaceEvent(game, map, eventId));
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

bool pressOutdoorFaceEvent(EngineController &game, MapId map, int eventId, auto success) {
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.eventId != eventId)
            continue;
        for (float distance : {75.0f, 125.0f, 200.0f, 300.0f}) {
            for (float direction : {1.0f, -1.0f}) {
                Vec3f position = face.boundingBox.center() + direction * distance * face.facePlane.normal;
                int yaw = std::lround(std::atan2(-direction * face.facePlane.normal.y,
                                                 -direction * face.facePlane.normal.x) *
                                      180 / std::numbers::pi);
                for (float z : {face.boundingBox.z1, face.boundingBox.center().z - pParty->eyeLevel,
                                face.boundingBox.center().z}) {
                    position.z = z;
                    for (int pitch : {-30, 0, 30}) {
                        game.teleportTo(map, position, yaw, pitch);
                        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                        game.tick();
                        if (success())
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

void completeHiddenTombPuzzle(EngineController &game) {
    loadMapAtPartyStart(game, MAP_ERATHIA);
    ASSERT_EQ(engine->_persistentVariables.mapVars[hiddenTombPuzzleMapVariable], 0);
    for (int i = 0; i < 5; ++i) {
        const int expected = i + 1;
        if (pointAtDecorationEvent(game, MAP_ERATHIA, hiddenTombPuzzleFirstEventId + i) ||
            pointAtFaceEvent(game, MAP_ERATHIA, hiddenTombPuzzleFirstEventId + i)) {
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
        }
        if (engine->_persistentVariables.mapVars[hiddenTombPuzzleMapVariable] != expected) {
            ASSERT_TRUE(pressOutdoorFaceEvent(game, MAP_ERATHIA, hiddenTombPuzzleFirstEventId + i, [expected] {
                return engine->_persistentVariables.mapVars[hiddenTombPuzzleMapVariable] == expected;
            }));
        }
        EXPECT_EQ(engine->_persistentVariables.mapVars[hiddenTombPuzzleMapVariable], i + 1);
    }
    ASSERT_TRUE(pParty->_questBits[QBIT_HIDDEN_TOMB_CODE_ENTERED]);
}

void enterHiddenTomb(EngineController &game) {
    loadMapAtPartyStart(game, MAP_ERATHIA);
    auto entrance = std::ranges::find(pOutdoor->faces, hiddenTombEntranceEventId, &BLVFace::eventId);
    ASSERT_NE(entrance, pOutdoor->faces.end());
    ASSERT_FALSE(entrance->Invisible());
    // The Hidden Tomb exit returns to this authored arrival point. Face the revealed entrance from that same spot.
    for (int yaw : {512, 1536, 0, 1024}) {
        game.teleportTo(MAP_ERATHIA, Vec3f(14207, -21526, 1), yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_INPUT_BLV)
            break;
    }
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces) && current_screen_type != SCREEN_INPUT_BLV; ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.eventId != hiddenTombEntranceEventId)
            continue;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(200, 0, 50), 180}, std::pair{Vec3f(-200, 0, 50), 0},
                 std::pair{Vec3f(0, 200, 50), 270}, std::pair{Vec3f(0, -200, 50), 90},
                 std::pair{Vec3f(400, 0, 50), 180}, std::pair{Vec3f(-400, 0, 50), 0},
                 std::pair{Vec3f(0, 400, 50), 270}, std::pair{Vec3f(0, -400, 50), 90}}) {
            game.teleportTo(MAP_ERATHIA, face.boundingBox.center() + offset, yaw, -30);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            if (current_screen_type == SCREEN_INPUT_BLV)
                break;
            try {
                game.pointMouseAtFace(faceId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (current_screen_type == SCREEN_INPUT_BLV)
                break;
        }
        if (current_screen_type != SCREEN_INPUT_BLV && face.Pressure_Plate()) {
            Vec3f position = face.boundingBox.center();
            position.z = face.boundingBox.z1 + 50;
            game.teleportTo(MAP_ERATHIA, position, 0);
            game.tick(5);
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HIDDEN_TOMB);
}

void openSarcophagus(EngineController &game) {
    for (Character &character : pParty->pCharacters)
        character.setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(30, MASTERY_GRANDMASTER));
    for (int click = 0; click < 2 && current_screen_type != SCREEN_CHEST; ++click)
        clickMapEvent(game, MAP_HIDDEN_TOMB, sarcophagusEventId);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_TRUE(pParty->_questBits[QBIT_HIDDEN_TOMB_SARCOPHAGUS_OPENED]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void setPartyClasses() {
    pParty->pCharacters[0].classType = CLASS_INITIATE;
    pParty->pCharacters[1].classType = CLASS_INITIATE;
    pParty->pCharacters[2].classType = CLASS_MONK;
    pParty->pCharacters[3].classType = CLASS_KNIGHT;
}

} // namespace

GAME_TEST(MM7NinjaPromotion, SourceContracts) {
    game.startNewGame();
    const NPCData &sand = pNPCStats->pNPCData[sandNpcId];
    EXPECT_EQ(sand.house, HOUSE_PIT_SAND_RESIDENCE);
    EXPECT_EQ(sand.dialogue_1_evt_id, ninjaOfferEventId);

    ASSERT_TRUE(engine->_globalEventMap.hasEvent(ninjaOfferEventId));
    const std::span<const EvtInstruction> offer = engine->_globalEventMap.function(ninjaOfferEventId);
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 4 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value == std::to_underlying(AWARD_PROMOTION_INITIATE);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 4 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value == std::to_underlying(AWARD_PROMOTION_INITIATE_HONORARY);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 4 && instruction.target_step == 8 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_LIGHT_PATH);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 5 && instruction.target_step == 10 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_DARK_PATH);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 11 && instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_ASHWAR_NOG_NOGOTH_TOMB_ACTIVE);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 13 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_MESSAGE_CIPHER);
    }));
    EXPECT_TRUE(std::ranges::any_of(offer, [](const EvtInstruction &instruction) {
        return instruction.step == 12 && instruction.opcode == EVENT_SetNPCTopic &&
               instruction.data.npc_topic_descr.npc_id == sandNpcId &&
               instruction.data.npc_topic_descr.index == 0 &&
               instruction.data.npc_topic_descr.event_id == ninjaReturnEventId;
    }));

    ASSERT_TRUE(engine->_globalEventMap.hasEvent(ninjaReturnEventId));
    const std::span<const EvtInstruction> handIn = engine->_globalEventMap.function(ninjaReturnEventId);
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 7 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HIDDEN_TOMB_SARCOPHAGUS_OPENED);
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.target_step == 5 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HIDDEN_TOMB_CODE_ENTERED);
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 40 && instruction.opcode == EVENT_Subtract &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_ASHWAR_NOG_NOGOTH_TOMB_ACTIVE);
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 41 && instruction.opcode == EVENT_Subtract &&
               instruction.data.variable_descr.type == VAR_ReputationInCurrentLocation &&
               instruction.data.variable_descr.value == 10;
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 43 && instruction.opcode == EVENT_SetNPCTopic &&
               instruction.data.npc_topic_descr.npc_id == sandNpcId &&
               instruction.data.npc_topic_descr.index == 0 && instruction.data.npc_topic_descr.event_id == 0;
    }));
    EXPECT_TRUE(std::ranges::any_of(handIn, [](const EvtInstruction &instruction) {
        return instruction.step == 44 && instruction.opcode == EVENT_SetNPCGreeting &&
               instruction.data.npc_descr.npc_id == sandNpcId &&
               instruction.data.npc_descr.greeting == ninjaGreetingIndex;
    }));

    loadMapAtPartyStart(game, MAP_SCHOOL_OF_SORCERY);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(scrollOfWavesEventId));
    EXPECT_TRUE(std::ranges::any_of(engine->_localEventMap.function(scrollOfWavesEventId), [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_MESSAGE_SCROLL_OF_WAVES);
    }));

    loadMapAtPartyStart(game, MAP_HIDDEN_TOMB);
    ASSERT_TRUE(engine->_localEventMap.hasEvent(sarcophagusEventId));
    const std::span<const EvtInstruction> sarcophagus = engine->_localEventMap.function(sarcophagusEventId);
    EXPECT_TRUE(std::ranges::any_of(sarcophagus, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == sarcophagusChestId;
    }));
    EXPECT_TRUE(std::ranges::any_of(sarcophagus, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HIDDEN_TOMB_SARCOPHAGUS_OPENED);
    }));
}

GAME_TEST(MM7NinjaPromotion, OfferRequiresInitiateAndDarkPath) {
    struct Case {
        bool initiateAward;
        bool honoraryAward;
        bool lightPath;
        bool darkPath;
        int expectedText;
        bool accepted;
    };
    for (const Case &testCase : std::array{
             Case{false, false, false, true, 137, false},
             Case{true, false, true, false, 138, false},
             Case{true, false, false, false, 139, false},
             Case{false, true, false, true, 136, true}}) {
        SCOPED_TRACE(fmt::format("initiate={}, honorary={}, light={}, dark={}", testCase.initiateAward,
                                 testCase.honoraryAward, testCase.lightPath, testCase.darkPath));
        test.prepareForNextTest();
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        NPCData *sand = &pNPCStats->pNPCData[sandNpcId];
        setPartyClasses();
        pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_INITIATE, testCase.initiateAward);
        pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_INITIATE_HONORARY, testCase.honoraryAward);
        pParty->_questBits.set(QBIT_LIGHT_PATH, testCase.lightPath);
        pParty->_questBits.set(QBIT_DARK_PATH, testCase.darkPath);
        const int gold = pParty->GetGold();
        const int reputation = pParty->GetPartyReputation();
        openSandHouse(game, sand);

        ASSERT_TRUE(selectNpcEvent(game, sand, ninjaOfferEventId));

        EXPECT_EQ(current_npc_text, pNPCTopics[testCase.expectedText - 1].pText);
        EXPECT_EQ(pParty->_questBits[QBIT_ASHWAR_NOG_NOGOTH_TOMB_ACTIVE], testCase.accepted);
        EXPECT_EQ(sand->dialogue_1_evt_id, testCase.accepted ? ninjaReturnEventId : ninjaOfferEventId);
        EXPECT_EQ(pParty->GetGold(), gold);
        EXPECT_EQ(pParty->GetPartyReputation(), reputation);
        EXPECT_EQ(pParty->pPickedItem.itemId, testCase.accepted ? ITEM_MESSAGE_CIPHER : ITEM_NULL);
        leaveHouse(game);
    }
}

GAME_TEST(MM7NinjaPromotion, PhysicalCipherTombAndSarcophagusPromoteOnce) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    NPCData *sand = &pNPCStats->pNPCData[sandNpcId];
    setPartyClasses();
    pParty->pCharacters[0]._achievedAwardsBits.set(AWARD_PROMOTION_INITIATE);
    pParty->pCharacters[1]._achievedAwardsBits.set(AWARD_PROMOTION_INITIATE_HONORARY);
    pParty->_questBits.set(QBIT_DARK_PATH);
    const std::array initialClasses = {CLASS_INITIATE, CLASS_INITIATE, CLASS_MONK, CLASS_KNIGHT};
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; ++i)
        experience[i] = pParty->pCharacters[i].experience;
    const int gold = pParty->GetGold();

    openSandHouse(game, sand);
    const int reputation = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, sand, ninjaOfferEventId));
    EXPECT_EQ(current_npc_text, pNPCTopics[135].pText);
    ASSERT_TRUE(pParty->_questBits[QBIT_ASHWAR_NOG_NOGOTH_TOMB_ACTIVE]);
    ASSERT_EQ(sand->dialogue_1_evt_id, ninjaReturnEventId);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_CIPHER);

    ASSERT_TRUE(selectNpcEvent(game, sand, ninjaReturnEventId));
    EXPECT_EQ(current_npc_text, pNPCTopics[140].pText); // The cipher has not been solved yet.
    EXPECT_TRUE(pParty->_questBits[QBIT_ASHWAR_NOG_NOGOTH_TOMB_ACTIVE]);
    leaveHouse(game);

    loadMapAtPartyStart(game, MAP_SCHOOL_OF_SORCERY);
    clickMapEvent(game, MAP_SCHOOL_OF_SORCERY, scrollOfWavesEventId);
    EXPECT_TRUE(pParty->hasItem(ITEM_MESSAGE_CIPHER));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_SCROLL_OF_WAVES);
    EXPECT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_SCROLL_OF_WAVES]);
    completeHiddenTombPuzzle(game);
    EXPECT_TRUE(pParty->hasItem(ITEM_MESSAGE_CIPHER));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_MESSAGE_SCROLL_OF_WAVES);

    openSandHouse(game, sand);
    ASSERT_TRUE(selectNpcEvent(game, sand, ninjaReturnEventId));
    EXPECT_EQ(current_npc_text, pNPCTopics[139].pText); // The code is entered, but the sarcophagus is unopened.
    EXPECT_TRUE(pParty->_questBits[QBIT_ASHWAR_NOG_NOGOTH_TOMB_ACTIVE]);
    leaveHouse(game);

    enterHiddenTomb(game);
    openSarcophagus(game);

    openSandHouse(game, sand);
    ASSERT_TRUE(selectNpcEvent(game, sand, ninjaReturnEventId));

    EXPECT_EQ(current_npc_text, pNPCTopics[141].pText);
    EXPECT_FALSE(pParty->_questBits[QBIT_ASHWAR_NOG_NOGOTH_TOMB_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_HIDDEN_TOMB_CODE_ENTERED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_HIDDEN_TOMB_SARCOPHAGUS_OPENED]);
    EXPECT_EQ(pParty->GetGold(), gold);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 10);
    EXPECT_EQ(sand->dialogue_1_evt_id, 0u);
    EXPECT_EQ(sand->greetingIndex, ninjaGreetingIndex);
    EXPECT_TRUE(pParty->hasItem(ITEM_MESSAGE_CIPHER));
    EXPECT_TRUE(pParty->hasItem(ITEM_MESSAGE_SCROLL_OF_WAVES));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    for (int i = 0; i < 4; ++i) {
        if (initialClasses[i] == CLASS_INITIATE) {
            EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_NINJA);
            EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 80000);
            EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_NINJA]);
            EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_NINJA_HONORARY]);
        } else {
            EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
            EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 40000);
            EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_NINJA]);
            EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_NINJA_HONORARY]);
        }
    }

    std::array<uint64_t, 4> promotedExperience;
    for (int i = 0; i < 4; ++i)
        promotedExperience[i] = pParty->pCharacters[i].experience;
    leaveHouse(game);
    openSandHouse(game, sand);
    EXPECT_FALSE(selectNpcEvent(game, sand, ninjaReturnEventId));
    EXPECT_EQ(pParty->GetGold(), gold);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 10);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, promotedExperience[i]);
    leaveHouse(game);

    loadMapAtPartyStart(game, MAP_ERATHIA);
    auto entrance = std::ranges::find(pOutdoor->faces, hiddenTombEntranceEventId, &BLVFace::eventId);
    ASSERT_NE(entrance, pOutdoor->faces.end());
    ASSERT_FALSE(entrance->Invisible());
    EvtInstruction customExit(EVENT_Exit);
    customExit.step = 14;
    engine->_localEventMap.add(hiddenTombRevealEventId, customExit);
    setFacesBit(hiddenTombEntranceCog, FACE_IsInvisible, true);
    ASSERT_TRUE(entrance->Invisible());

    runEvent(EvtContext(EVT_SOURCE_MAP, 1, Pid(), /* duringMapLoad = */ true), 1);

    EXPECT_TRUE(entrance->Invisible()); // A custom reveal shape remains authoritative.
}
