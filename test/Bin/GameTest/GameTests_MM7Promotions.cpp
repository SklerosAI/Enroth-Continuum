#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "MM7CampaignChain.h"
#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Exception.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> checkpointExperienceBaseline = {3000, 3100, 3200, 3300};
constexpr int checkpointGoldBaseline = 4000;

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

NPCData *findHouseNpcEvent(HouseId houseId, auto predicate) {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [houseId, predicate](const NPCData &candidate) {
        std::array eventIds = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                               candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return candidate.house == houseId && std::ranges::any_of(eventIds, [predicate](unsigned int eventId) {
            return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                   std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
        });
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
    const char *prefix = current_screen_type == SCREEN_NPC_DIALOGUE ? "NpcDialogue_Option" : "HouseNpcDialogue_Option";
    game.pressGuiButton(fmt::format("{}{}", prefix, option - topics.begin()));
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

void openTarinHouse(EngineController &game, NPCData *tarin) {
    game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_TEMPLE_HARMONDALE))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_HARMONDALE, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE)
            break;
        if (current_screen_type == SCREEN_NPC_DIALOGUE) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE); // A street NPC can intercept the door interaction.
            game.tick();
        }
    }
    ASSERT_TRUE(current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE);
    if (current_screen_type != SCREEN_HOUSE)
        return; // A single resident NPC opens its dialogue immediately.
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_TEMPLE_HARMONDALE);
    auto entry = std::ranges::find(houseNpcs, tarin, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openMazimHouse(EngineController &game, NPCData *mazim) {
    game.teleportTo(MAP_MOUNT_NIGHON, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_MOUNT_NIGHON_DUSKS_HOME))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_MOUNT_NIGHON, position, yaw);
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
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_MOUNT_NIGHON_DUSKS_HOME);
    auto entry = std::ranges::find(houseNpcs, mazim, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openThrushHouse(EngineController &game, NPCData *thrush) {
    game.teleportTo(MAP_ERATHIA, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_ERATHIA_THRUSH_RESIDENCE))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_ERATHIA, position, yaw);
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
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_ERATHIA_THRUSH_RESIDENCE);
    auto entry = std::ranges::find(houseNpcs, thrush, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openOrgHouse(EngineController &game, NPCData *org) {
    game.teleportTo(MAP_ERATHIA, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces)
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_ERATHIA_ORG_HOUSE))
            doors.push_back(approachFace(face));
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_ERATHIA, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == HOUSE_ERATHIA_ORG_HOUSE)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_ERATHIA_ORG_HOUSE);
    auto entry = std::ranges::find(houseNpcs, org, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void leaveEmeraldIsland(EngineController &game) {
    engine->config->debug.NoMargaret.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(16154, 8560, 128), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_EMERALD_ISLAND_MARKHAMS_HEADQUARTERS);
    game.pressGuiButton("House_Npc1"); // Thomas the Judge.
    game.tick();
    for (ItemId item : std::array{ITEM_POTION_CURE_WOUNDS, ITEM_QUEST_SEASHELL, ITEM_LONGBOW,
                                  ITEM_QUEST_FLOOR_TILE_W_MOON_INSIGNIA, ITEM_QUEST_LUTE,
                                  ITEM_QUEST_WEALTHY_HAT}) {
        pParty->setHoldingItem(Item(item));
        game.pressGuiButton("HouseNpcDialogue_Option1");
        game.tick();
    }
    game.pressGuiButton("HouseNpcDialogue_Option1"); // Complete the contest after the last hand-in.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton("House_Npc0"); // Lord Markham.
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0"); // Congratulations.
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0"); // Your ship.
    game.tick();
    leaveHouse(game);
    game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(11008, 34, 193), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0"); // Cast off.
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
}

void openMarkhamManor(EngineController &game, NPCData *markham) {
    game.teleportTo(MAP_LORD_MARKHAMS_MANOR, Vec3f(), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.Clickable() && eventOpensHouse(face.eventId, HOUSE_LORD_MARKHAMS_MANOR_LORD_MARKHAMS_CHAMBER);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_LORD_MARKHAMS_MANOR, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_LORD_MARKHAMS_MANOR_LORD_MARKHAMS_CHAMBER);
    auto entry = std::ranges::find(houseNpcs, markham, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openSowerHouse(EngineController &game, NPCData *sower) {
    game.teleportTo(MAP_TULAREAN_FOREST, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces)
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_TULAREAN_FOREST_SOWER_RESIDENCE))
            doors.push_back(approachFace(face));
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_TULAREAN_FOREST, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE &&
            window_SpeakInHouse->houseId() == HOUSE_TULAREAN_FOREST_SOWER_RESIDENCE)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_TULAREAN_FOREST_SOWER_RESIDENCE);
    auto entry = std::ranges::find(houseNpcs, sower, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void openSnickHouse(EngineController &game, NPCData *snick) {
    game.teleportTo(MAP_AVLEE, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces)
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_AVLEE_SNICK_HOUSE))
            doors.push_back(approachFace(face));
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_AVLEE, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == HOUSE_AVLEE_SNICK_HOUSE)
            break;
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_AVLEE_SNICK_HOUSE);
    auto entry = std::ranges::find(houseNpcs, snick, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

bool openFaerieKingDialogue(EngineController &game, NPCData *faerieKing) {
    const int faerieKingId = faerieKing - pNPCStats->pNPCData.data();
    game.teleportTo(MAP_HALL_UNDER_THE_HILL, Vec3f(), 0);
    auto audience = std::ranges::find_if(pIndoor->faces, [faerieKingId](const BLVFace &face) {
        return face.eventId != 0 && engine->_localEventMap.hasEvent(face.eventId) &&
               std::ranges::any_of(engine->_localEventMap.function(face.eventId),
                                   [faerieKingId](const EvtInstruction &instruction) {
                                       return instruction.opcode == EVENT_SpeakNPC &&
                                              instruction.data.npc_descr.npc_id == faerieKingId;
                                   });
    });
    ASSERT_NE(audience, pIndoor->faces.end());
    ASSERT_TRUE(audience->Pressure_Plate());
    const int audienceEvent = audience->eventId;
    game.runGameRoutine([audienceEvent] { runEvent(EvtContext(EVT_SOURCE_MAP, audienceEvent)); });
    game.tick();
    return current_screen_type == SCREEN_NPC_DIALOGUE;
}

int countVariableInstruction(const std::vector<EvtInstruction> &function, EvtOpcode opcode, EvtVariable variable,
                             int value) {
    return std::ranges::count_if(function, [=](const EvtInstruction &instruction) {
        return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
               instruction.data.variable_descr.value == value;
    });
}

bool hasNpcTopicInstruction(const std::vector<EvtInstruction> &function, int npcId, int index, int eventId) {
    return std::ranges::any_of(function, [=](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SetNPCTopic && instruction.data.npc_topic_descr.npc_id == npcId &&
               instruction.data.npc_topic_descr.index == index && instruction.data.npc_topic_descr.event_id == eventId;
    });
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

} // namespace

GAME_TEST(MM7Promotions, HunterPromotionSourceContract) {
    game.startNewGame();
    const std::vector<EvtInstruction> &request = engine->_globalEventMap.function(79);
    const std::vector<EvtInstruction> &promotion = engine->_globalEventMap.function(83);

    EXPECT_EQ(countVariableInstruction(request, EVENT_Set, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_FAERIE_MOUND_ACTIVE)),
              1);
    EXPECT_TRUE(hasNpcTopicInstruction(request, 45, 0, 80));
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_OnCanShowDialogItemCmp, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_FAERIE_MOUND_ACTIVE)),
              1);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Compare, VAR_Class, std::to_underlying(CLASS_RANGER)), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Set, VAR_Class, std::to_underlying(CLASS_HUNTER)), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Award,
                                       std::to_underlying(AWARD_PROMOTION_HUNTER)),
              4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Experience, 30000), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Award,
                                       std::to_underlying(AWARD_PROMOTION_HUNTER_HONORARY)),
              4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Experience, 15000), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Subtract, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_FAERIE_MOUND_ACTIVE)),
              1);
    EXPECT_TRUE(hasNpcTopicInstruction(promotion, 45, 0, 81));
    EXPECT_TRUE(hasNpcTopicInstruction(promotion, 52, 0, 0));
}

GAME_TEST(MM7Promotions, WarriorMagePromotionSourceContract) {
    game.startNewGame();
    const std::vector<EvtInstruction> &request = engine->_globalEventMap.function(67);
    const std::vector<EvtInstruction> &promotion = engine->_globalEventMap.function(68);

    EXPECT_EQ(countVariableInstruction(request, EVENT_Add, VAR_PlayerItemInHands,
                                       std::to_underlying(ITEM_QUEST_WORN_BELT)),
              1);
    EXPECT_EQ(countVariableInstruction(request, EVENT_Add, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_RED_DWARF_MINES_LIFT_ACTIVE)),
              1);
    EXPECT_EQ(countVariableInstruction(request, EVENT_Add, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_ORACLE_RECOVERS_WORN_BELT)),
              1);
    EXPECT_TRUE(hasNpcTopicInstruction(request, 41, 0, 68));
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Compare, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_RED_DWARF_MINES_LIFT_SABOTAGED)),
              1);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Compare, VAR_Class, std::to_underlying(CLASS_ARCHER)), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Set, VAR_Class, std::to_underlying(CLASS_WARRIOR_MAGE)), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Award,
                                       std::to_underlying(AWARD_PROMOTION_WARRIOR_MAGE)),
              4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Experience, 30000), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Award,
                                       std::to_underlying(AWARD_PROMOTION_WARRIOR_MAGE_HONORARY)),
              4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Experience, 15000), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Subtract, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_RED_DWARF_MINES_LIFT_ACTIVE)),
              1);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_FixedGold, 7500), 1);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Subtract, VAR_ReputationInCurrentLocation, 5), 1);
    EXPECT_TRUE(hasNpcTopicInstruction(promotion, 41, 0, 69));

    game.teleportTo(MAP_RED_DWARF_MINES, Vec3f(), 0);
    const std::vector<EvtInstruction> &sabotage = engine->_localEventMap.function(383);
    EXPECT_EQ(countVariableInstruction(sabotage, EVENT_Compare, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_RED_DWARF_MINES_LIFT_ACTIVE)),
              1);
    EXPECT_EQ(countVariableInstruction(sabotage, EVENT_Compare, VAR_PlayerItemInHands,
                                       std::to_underlying(ITEM_QUEST_WORN_BELT)),
              1);
    EXPECT_EQ(countVariableInstruction(sabotage, EVENT_Set, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_RED_DWARF_MINES_LIFT_SABOTAGED)),
              1);
    EXPECT_EQ(countVariableInstruction(sabotage, EVENT_Subtract, VAR_PlayerItemInHands,
                                       std::to_underlying(ITEM_QUEST_WORN_BELT)),
              1);
    EXPECT_EQ(countVariableInstruction(sabotage, EVENT_Subtract, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_ORACLE_RECOVERS_WORN_BELT)),
              1);
}

GAME_TEST(MM7Promotions, CavalierPromotionSourceContract) {
    game.startNewGame();
    const std::vector<EvtInstruction> &request = engine->_globalEventMap.function(73);
    const std::vector<EvtInstruction> &promotion = engine->_globalEventMap.function(74);

    EXPECT_EQ(countVariableInstruction(request, EVENT_Set, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE)),
              1);
    EXPECT_TRUE(hasNpcTopicInstruction(request, 43, 0, 74));
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Compare, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_HAUNTED_HOUSE_CLEARED)),
              1);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Compare, VAR_Class, std::to_underlying(CLASS_KNIGHT)), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Set, VAR_Class, std::to_underlying(CLASS_CAVALIER)), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Award,
                                       std::to_underlying(AWARD_PROMOTION_CHEVALIER)),
              4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Experience, 30000), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Award,
                                       std::to_underlying(AWARD_PROMOTION_CHEVALIER_HONORARY)),
              4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Add, VAR_Experience, 15000), 4);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Subtract, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE)),
              1);
    EXPECT_EQ(std::ranges::count_if(promotion, [](const EvtInstruction &instruction) {
                  return (instruction.opcode == EVENT_Add || instruction.opcode == EVENT_Set ||
                          instruction.opcode == EVENT_Subtract) &&
                         (instruction.data.variable_descr.type == VAR_FixedGold ||
                          instruction.data.variable_descr.type == VAR_RandomGold);
              }),
              0);
    EXPECT_EQ(countVariableInstruction(promotion, EVENT_Subtract, VAR_ReputationInCurrentLocation, 5), 1);
    EXPECT_TRUE(hasNpcTopicInstruction(promotion, 43, 0, 75));

    game.teleportTo(MAP_HAUNTED_MANSION, Vec3f(), 0);
    const std::vector<EvtInstruction> &mapLeave = engine->_localEventMap.function(2);
    EXPECT_EQ(std::ranges::count(mapLeave, EVENT_OnMapLeave, &EvtInstruction::opcode), 1);
    auto allActorsKilled = std::ranges::find(mapLeave, EVENT_IsActorKilled, &EvtInstruction::opcode);
    ASSERT_NE(allActorsKilled, mapLeave.end());
    EXPECT_EQ(allActorsKilled->data.actor_descr.policy, KILL_CHECK_ANY);
    EXPECT_EQ(allActorsKilled->data.actor_descr.param, 0);
    EXPECT_EQ(allActorsKilled->data.actor_descr.num, 0);
    EXPECT_EQ(countVariableInstruction(mapLeave, EVENT_Set, VAR_QBits_QuestsDone,
                                       std::to_underlying(QBIT_HAUNTED_HOUSE_CLEARED)),
              1);
}

GAME_TEST(MM7Promotions, KnightCavalierPhysicalPromotionPacket) {
    game.startNewGame();
    NPCData *org = &pNPCStats->pNPCData[43];
    ASSERT_EQ(org->house, HOUSE_ERATHIA_ORG_HOUSE);
    ASSERT_EQ(org->dialogue_1_evt_id, 73u);

    std::array initialClasses = {CLASS_KNIGHT, CLASS_ARCHER, CLASS_KNIGHT, CLASS_SORCERER};
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = initialClasses[i];
        experience[i] = pParty->pCharacters[i].experience;
    }
    const int gold = pParty->GetGold();

    openOrgHouse(game, org);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    const int reputation = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, org, 73));
    ASSERT_TRUE(pParty->_questBits[QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_HAUNTED_HOUSE_CLEARED]);
    ASSERT_EQ(org->dialogue_1_evt_id, 74u);

    ASSERT_TRUE(selectNpcEvent(game, org, 74)); // The return topic refuses promotion before the mansion is cleared.
    EXPECT_TRUE(pParty->_questBits[QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_HAUNTED_HOUSE_CLEARED]);
    EXPECT_EQ(pParty->GetGold(), gold);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_CHEVALIER]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_CHEVALIER_HONORARY]);
    }
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_HAUNTED_MANSION, Vec3f(), 0);
    ASSERT_FALSE(pActors.empty());
    ASSERT_FALSE(Actor::isActorKilled(KILL_CHECK_ANY, 0, 0));
    useCampaignChainTransition(game, MAP_HAUNTED_MANSION, 501);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_DOWNS);
    EXPECT_FALSE(pParty->_questBits[QBIT_HAUNTED_HOUSE_CLEARED]);

    game.teleportTo(MAP_HAUNTED_MANSION, Vec3f(), 0);
    for (Actor &actor : pActors)
        if (!actor.IsNotAlive())
            Actor::Die(actor.id); // Fixture the combat outcome; event 2 and the physical exit still own quest completion.
    ASSERT_TRUE(Actor::isActorKilled(KILL_CHECK_ANY, 0, 0));
    useCampaignChainTransition(game, MAP_HAUNTED_MANSION, 501);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_DOWNS);
    ASSERT_TRUE(pParty->_questBits[QBIT_HAUNTED_HOUSE_CLEARED]);

    openOrgHouse(game, org);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, org, 74));
    EXPECT_FALSE(pParty->_questBits[QBIT_HAUNTED_HOUSE_UNDEAD_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_HAUNTED_HOUSE_CLEARED]);
    EXPECT_EQ(org->dialogue_1_evt_id, 75u);
    EXPECT_EQ(pParty->GetGold(), gold); // The authored Cavalier promotion has no gold reward.
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 5);
    for (int i : {0, 2}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_CAVALIER);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 30000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_CHEVALIER]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_CHEVALIER_HONORARY]);
    }
    for (int i : {1, 3}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 15000);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_CHEVALIER]);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_CHEVALIER_HONORARY]);
    }

    std::array<uint64_t, 4> promotedExperience;
    for (int i = 0; i < 4; ++i)
        promotedExperience[i] = pParty->pCharacters[i].experience;
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    openOrgHouse(game, org);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    EXPECT_FALSE(selectNpcEvent(game, org, 74));
    EXPECT_EQ(pParty->GetGold(), gold);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 5);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, promotedExperience[i]);
}

GAME_TEST(MM7Promotions, ArcherWarriorMagePhysicalPromotionPacket) {
    game.startNewGame();
    NPCData *snick = &pNPCStats->pNPCData[41];
    ASSERT_EQ(snick->house, HOUSE_AVLEE_SNICK_HOUSE);
    ASSERT_EQ(snick->dialogue_1_evt_id, 67u);

    std::array initialClasses = {CLASS_ARCHER, CLASS_KNIGHT, CLASS_ARCHER, CLASS_SORCERER};
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = initialClasses[i];
        experience[i] = pParty->pCharacters[i].experience;
    }
    const int gold = pParty->GetGold();

    openSnickHouse(game, snick);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    const int reputation = pParty->GetPartyReputation();
    ASSERT_TRUE(selectNpcEvent(game, snick, 67));
    ASSERT_TRUE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_SABOTAGED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_WORN_BELT]);
    ASSERT_EQ(snick->dialogue_1_evt_id, 68u);
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_WORN_BELT) || pParty->pPickedItem.itemId == ITEM_QUEST_WORN_BELT);

    ASSERT_TRUE(selectNpcEvent(game, snick, 68)); // The return topic refuses promotion before the physical sabotage.
    EXPECT_TRUE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_SABOTAGED]);
    EXPECT_EQ(pParty->GetGold(), gold);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_WARRIOR_MAGE]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_WARRIOR_MAGE_HONORARY]);
    }
    if (pParty->pPickedItem.itemId == ITEM_QUEST_WORN_BELT)
        ASSERT_TRUE(pParty->pCharacters[0].inventory.add(pParty->takeHoldingItem()));
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    game.teleportTo(MAP_RED_DWARF_MINES, Vec3f(), 0);
    ASSERT_FALSE(pParty->PartyTimes.CounterEventValues[3].isValid());
    ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_RED_DWARF_MINES, 383, 1, [] {
        return pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_SABOTAGED];
    }));
    EXPECT_TRUE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_SABOTAGED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_WORN_BELT]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_WORN_BELT));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_TRUE(pParty->PartyTimes.CounterEventValues[3].isValid());

    openSnickHouse(game, snick);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, snick, 68));
    EXPECT_FALSE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_RED_DWARF_MINES_LIFT_SABOTAGED]);
    EXPECT_EQ(snick->dialogue_1_evt_id, 69u);
    EXPECT_EQ(pParty->GetGold(), gold + 7500);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 5);
    for (int i : {0, 2}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_WARRIOR_MAGE);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 30000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_WARRIOR_MAGE]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_WARRIOR_MAGE_HONORARY]);
    }
    for (int i : {1, 3}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 15000);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_WARRIOR_MAGE]);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_WARRIOR_MAGE_HONORARY]);
    }

    std::array<uint64_t, 4> promotedExperience;
    for (int i = 0; i < 4; ++i)
        promotedExperience[i] = pParty->pCharacters[i].experience;
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    openSnickHouse(game, snick);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    EXPECT_FALSE(selectNpcEvent(game, snick, 68));
    EXPECT_EQ(pParty->GetGold(), gold + 7500);
    EXPECT_EQ(pParty->GetPartyReputation(), reputation - 5);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, promotedExperience[i]);
}

GAME_TEST(MM7Promotions, RangerHunterRealDialoguePacket) {
    game.startNewGame();
    NPCData *sower = &pNPCStats->pNPCData[45];
    NPCData *faerieKing = &pNPCStats->pNPCData[52];
    ASSERT_EQ(sower->house, HOUSE_TULAREAN_FOREST_SOWER_RESIDENCE);
    ASSERT_EQ(sower->dialogue_1_evt_id, 79u);
    ASSERT_EQ(faerieKing->dialogue_1_evt_id, 83u);

    std::array initialClasses = {CLASS_RANGER, CLASS_KNIGHT, CLASS_RANGER, CLASS_SORCERER};
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; ++i) {
        pParty->pCharacters[i].classType = initialClasses[i];
        experience[i] = pParty->pCharacters[i].experience;
    }
    const int gold = pParty->GetGold();

    // The durable Kerrid route already proves the Hall Under the Hill audience face and Faerie King dialogue.
    // Exercise that real dialogue here without repeating the letter, food, pipe, and return transaction.
    pParty->_questBits.set(QBIT_FAERIE_PIPES_ACTIVE); // A nearby but wrong prerequisite must not unlock promotion 83.
    EXPECT_FALSE(openFaerieKingDialogue(game, faerieKing));
    EXPECT_FALSE(pParty->_questBits[QBIT_FAERIE_MOUND_ACTIVE]);
    EXPECT_EQ(pParty->GetGold(), gold);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_HUNTER]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_HUNTER_HONORARY]);
    }
    openSowerHouse(game, sower);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, sower, 79));
    ASSERT_TRUE(pParty->_questBits[QBIT_FAERIE_MOUND_ACTIVE]);
    ASSERT_EQ(sower->dialogue_1_evt_id, 80u);
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());

    ASSERT_TRUE(openFaerieKingDialogue(game, faerieKing));
    ASSERT_TRUE(selectNpcEvent(game, faerieKing, 83));
    EXPECT_FALSE(pParty->_questBits[QBIT_FAERIE_MOUND_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_FAERIE_PIPES_ACTIVE]);
    EXPECT_EQ(sower->dialogue_1_evt_id, 81u);
    EXPECT_EQ(faerieKing->dialogue_1_evt_id, 0u);
    EXPECT_EQ(pParty->GetGold(), gold);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    for (int i : {0, 2}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, CLASS_HUNTER);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 30000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_HUNTER]);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_HUNTER_HONORARY]);
    }
    for (int i : {1, 3}) {
        EXPECT_EQ(pParty->pCharacters[i].classType, initialClasses[i]);
        EXPECT_EQ(pParty->pCharacters[i].experience, experience[i] + 15000);
        EXPECT_FALSE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_HUNTER]);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_PROMOTION_HUNTER_HONORARY]);
    }

    std::array<uint64_t, 4> promotedExperience;
    for (int i = 0; i < 4; ++i)
        promotedExperience[i] = pParty->pCharacters[i].experience;
    leaveHouse(game);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    EXPECT_FALSE(openFaerieKingDialogue(game, faerieKing));
    EXPECT_EQ(pParty->GetGold(), gold);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, promotedExperience[i]);
}

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

GAME_TEST(MM7SideQuests, LanternOfLightRequiresBarrowChest) {
    auto activeTape = tapes.questBit(QBIT_LANTERN_OF_LIGHT_ACTIVE);
    auto goldTape = tapes.gold();
    game.startNewGame();
    NPCData *tarin = findHouseNpcEvent(HOUSE_TEMPLE_HARMONDALE, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_LANTERN_OF_LIGHT_ACTIVE);
    });
    ASSERT_NE(tarin, nullptr);
    const unsigned int offerEvent = findNpcEvent(tarin, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_LANTERN_OF_LIGHT_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);

    openTarinHouse(game, tarin);
    test.startTaping();
    ASSERT_TRUE(selectNpcEvent(game, tarin, offerEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_LANTERN_OF_LIGHT_ACTIVE]);
    const unsigned int handInEvent = findNpcEvent(tarin, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_LANTERN_OF_LIGHT);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);
    openTarinHouse(game, tarin);
    ASSERT_TRUE(selectNpcEvent(game, tarin, handInEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_LANTERN_OF_LIGHT_ACTIVE]);
    EXPECT_EQ(goldTape.delta(), 0);
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT]);
    leaveHouse(game);

    game.teleportTo(MAP_BARROW_II, Vec3f(), 0);
    auto chest = std::ranges::find_if(vChests, [](const Chest &candidate) {
        return !!candidate.inventory.find(ITEM_QUEST_LANTERN_OF_LIGHT);
    });
    ASSERT_NE(chest, vChests.end());
    const int chestId = chest - vChests.begin();
    auto chestFace = std::ranges::find_if(pIndoor->faces, [chestId](const BLVFace &face) {
        return face.Clickable() && engine->_localEventMap.hasEvent(face.eventId) &&
               std::ranges::any_of(engine->_localEventMap.function(face.eventId), [chestId](const EvtInstruction &instruction) {
                   return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == chestId;
               });
    });
    ASSERT_NE(chestFace, pIndoor->faces.end());
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; attempt++) {
        Chest::open(chestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (int i = 0; i < 40 && !pParty->hasItem(ITEM_QUEST_LANTERN_OF_LIGHT) &&
                        pParty->pPickedItem.itemId != ITEM_QUEST_LANTERN_OF_LIGHT; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_LANTERN_OF_LIGHT) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_LANTERN_OF_LIGHT);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; i++)
        experience[i] = pParty->pCharacters[i].experience;

    openTarinHouse(game, tarin);
    ASSERT_TRUE(selectNpcEvent(game, tarin, handInEvent));
    EXPECT_EQ(activeTape, tape(true, false));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_LANTERN_OF_LIGHT));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(goldTape.delta(), 1000);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT]);
        EXPECT_EQ(pParty->pCharacters[i].experience - experience[i], 5000);
    }
}

GAME_TEST(MM7SideQuests, HaldarsRemainsRequireMazeChest) {
    auto activeTape = tapes.questBit(QBIT_HALDARS_REMAINS_ACTIVE);
    auto goldTape = tapes.gold();
    game.startNewGame();
    NPCData *mazim = findHouseNpcEvent(HOUSE_MOUNT_NIGHON_DUSKS_HOME, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HALDARS_REMAINS_ACTIVE);
    });
    ASSERT_NE(mazim, nullptr);
    const unsigned int offerEvent = findNpcEvent(mazim, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HALDARS_REMAINS_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);

    openMazimHouse(game, mazim);
    test.startTaping();
    ASSERT_TRUE(selectNpcEvent(game, mazim, offerEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_ACTIVE]);
    const unsigned int handInEvent = findNpcEvent(mazim, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_HALDARS_REMAINS);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);
    openMazimHouse(game, mazim);
    ASSERT_TRUE(selectNpcEvent(game, mazim, handInEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_HALDARS_REMAINS_ACTIVE]);
    EXPECT_EQ(goldTape.delta(), 0);
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character._achievedAwardsBits[AWARD_NIGHON_RETRIEVED_HALDARS_REMAINS]);
    leaveHouse(game);

    game.teleportTo(MAP_MAZE, Vec3f(), 0);
    auto chest = std::ranges::find_if(vChests, [](const Chest &candidate) {
        return !!candidate.inventory.find(ITEM_QUEST_HALDARS_REMAINS);
    });
    ASSERT_NE(chest, vChests.end());
    const int chestId = chest - vChests.begin();
    auto chestFace = std::ranges::find_if(pIndoor->faces, [chestId](const BLVFace &face) {
        return face.Clickable() && engine->_localEventMap.hasEvent(face.eventId) &&
               std::ranges::any_of(engine->_localEventMap.function(face.eventId), [chestId](const EvtInstruction &instruction) {
                   return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == chestId;
               });
    });
    ASSERT_NE(chestFace, pIndoor->faces.end());
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; attempt++) {
        Chest::open(chestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (int i = 0; i < 40 && !pParty->hasItem(ITEM_QUEST_HALDARS_REMAINS) &&
                        pParty->pPickedItem.itemId != ITEM_QUEST_HALDARS_REMAINS; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_HALDARS_REMAINS) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_HALDARS_REMAINS);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; i++)
        experience[i] = pParty->pCharacters[i].experience;

    openMazimHouse(game, mazim);
    ASSERT_TRUE(selectNpcEvent(game, mazim, handInEvent));
    EXPECT_EQ(activeTape, tape(true, false));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_HALDARS_REMAINS));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(goldTape.delta(), 35000);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_NIGHON_RETRIEVED_HALDARS_REMAINS]);
        EXPECT_EQ(pParty->pCharacters[i].experience - experience[i], 50000);
    }
}

namespace {

void runParsonsQuillRoute(EngineController &game, TestController &test, CommonTapeRecorder &tapes,
                          bool useCheckpointBaseline = false) {
    auto letterTape = tapes.questBit(QBIT_THRUSH_LETTER_ACTIVE);
    auto quillTape = tapes.questBit(QBIT_PARSONS_QUILL_ACTIVE);
    auto goldTape = tapes.gold();
    leaveEmeraldIsland(game);
    if (useCheckpointBaseline) {
        pParty->SetGold(checkpointGoldBaseline);
        for (int i = 0; i < 4; i++)
            pParty->pCharacters[i].experience = checkpointExperienceBaseline[i];
    }
    NPCData *thrush = findHouseNpcEvent(HOUSE_ERATHIA_THRUSH_RESIDENCE, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_THRUSH_LETTER_ACTIVE);
    });
    ASSERT_NE(thrush, nullptr);
    const unsigned int offerEvent = findNpcEvent(thrush, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_THRUSH_LETTER_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);

    openThrushHouse(game, thrush);
    const int initialReputation = pParty->GetPartyReputation();
    test.startTaping();
    ASSERT_TRUE(selectNpcEvent(game, thrush, offerEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_THRUSH_LETTER_ACTIVE]);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_LETTER_FROM_NORBERT_THRUSH) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_LETTER_FROM_NORBERT_THRUSH);
    const unsigned int handInEvent = findNpcEvent(thrush, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_PARSONS_QUILL);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);

    openThrushHouse(game, thrush);
    ASSERT_TRUE(selectNpcEvent(game, thrush, handInEvent)); // The visible return topic refuses the missing quill.
    EXPECT_TRUE(pParty->_questBits[QBIT_THRUSH_LETTER_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_PARSONS_QUILL_ACTIVE]);
    EXPECT_TRUE(pParty->hasItem(ITEM_QUEST_LETTER_FROM_NORBERT_THRUSH) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_LETTER_FROM_NORBERT_THRUSH);
    EXPECT_EQ(goldTape.delta(), 0);
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character._achievedAwardsBits[AWARD_ERATHIA_RETURNED_PARSONS_QUILL]);
    leaveHouse(game);

    auto markhamIt = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array eventIds = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                               candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return std::ranges::any_of(eventIds, [](unsigned int eventId) {
            return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                   std::ranges::any_of(engine->_globalEventMap.function(eventId), [](const EvtInstruction &instruction) {
                       return instruction.opcode == EVENT_Subtract &&
                              instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
                              instruction.data.variable_descr.value ==
                                  std::to_underlying(ITEM_QUEST_LETTER_FROM_NORBERT_THRUSH);
                   });
        });
    });
    ASSERT_NE(markhamIt, pNPCStats->pNPCData.end());
    NPCData *markham = &*markhamIt;
    const unsigned int exchangeEvent = findNpcEvent(markham, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_LETTER_FROM_NORBERT_THRUSH);
    });
    ASSERT_NE(exchangeEvent, 0u);
    openMarkhamManor(game, markham);
    ASSERT_TRUE(selectNpcEvent(game, markham, exchangeEvent));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_PARSONS_QUILL);
    EXPECT_TRUE(pParty->_questBits[QBIT_PARSONS_QUILL_ACTIVE]);
    leaveHouse(game);

    std::array<uint64_t, 4> experience;
    for (int i = 0; i < 4; i++)
        experience[i] = pParty->pCharacters[i].experience;
    openThrushHouse(game, thrush);
    ASSERT_TRUE(selectNpcEvent(game, thrush, handInEvent));
    EXPECT_EQ(letterTape, tape(true, false));
    EXPECT_EQ(quillTape, tape(false, true, false));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(goldTape.delta(), 2000);
    EXPECT_EQ(pParty->GetPartyReputation() - initialReputation, -5);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_ERATHIA_RETURNED_PARSONS_QUILL]);
        EXPECT_EQ(pParty->pCharacters[i].experience - experience[i], 5000);
    }
}

} // namespace

GAME_TEST(MM7SideQuests, ParsonsQuillRequiresMarkhamExchange) {
    runParsonsQuillRoute(game, test, tapes);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteParsonsQuill) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runParsonsQuillRoute(game, test, tapes, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    leaveHouse(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_ERATHIA);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Parsons' Quill regional checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadParsonsQuill) {
    assertDisposableHeadlessProfile();
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = Blob::copy(ufs->read(checkpointSlot));
    ASSERT_GT(saved.size(), 1024u);

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, checkpointFileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, checkpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);

    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_ERATHIA);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_FALSE(pParty->_questBits[QBIT_THRUSH_LETTER_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_PARSONS_QUILL_ACTIVE]);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_LETTER_FROM_NORBERT_THRUSH));
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_PARSONS_QUILL));
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(pParty->uNumGold, checkpointGoldBaseline + 2000);
    EXPECT_EQ(pParty->GetPartyReputation(), -5);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_ERATHIA_RETURNED_PARSONS_QUILL]);
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperienceBaseline[i] + 5000);
    }
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
