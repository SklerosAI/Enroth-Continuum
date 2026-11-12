#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/PartyPlacement.h"
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
constexpr std::array<uint64_t, 4> checkpointExperienceBaseline = {6000, 6100, 6200, 6300};
constexpr int checkpointGoldBaseline = 7000;

struct StatuetteSource {
    MapId map;
    ItemId item;
};

struct StatuetteShrine {
    MapId map;
    ItemId item;
    QuestBit placedBit;
};

constexpr std::array statuetteSources = {
    StatuetteSource{MAP_GRAND_TEMPLE_OF_THE_SUN, ITEM_QUEST_KNIGHT_STATUETTE},
    StatuetteSource{MAP_GRAND_TEMPLE_OF_THE_MOON, ITEM_QUEST_EAGLE_STATUETTE},
    StatuetteSource{MAP_MAZE, ITEM_QUEST_ANGEL_STATUETTE},
};

constexpr std::array statuetteShrines = {
    StatuetteShrine{MAP_AVLEE, ITEM_QUEST_KNIGHT_STATUETTE, QBIT_KNIGHT_STATUETTE_PLACED},
    StatuetteShrine{MAP_TATALIA, ITEM_QUEST_EAGLE_STATUETTE, QBIT_EAGLE_STATUETTE_PLACED},
    StatuetteShrine{MAP_BRACADA_DESERT, ITEM_QUEST_ANGEL_STATUETTE, QBIT_ANGEL_STATUETTE_PLACED},
};

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool eventSetsQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

NPCData *findThomLumbra() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array eventIds = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                               candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return std::ranges::any_of(eventIds, [](unsigned int eventId) {
            return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                   std::ranges::any_of(engine->_globalEventMap.function(eventId), [](const EvtInstruction &instruction) {
                       return eventSetsQuestBit(instruction, QBIT_STATUETTES_ACTIVE);
                   });
        });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

unsigned int findNpcEvent(const NPCData *npc, auto predicate) {
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

void openLumbraHouse(EngineController &game, NPCData *lumbra) {
    game.teleportTo(MAP_TULAREAN_FOREST, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, lumbra->house))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_TULAREAN_FOREST, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == lumbra->house)
            break;
        if (current_screen_type == SCREEN_NPC_DIALOGUE) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), lumbra->house);
    auto entry = std::ranges::find(houseNpcs, lumbra, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
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

void acquireStatuette(EngineController &game, const StatuetteSource &source) {
    loadMapAtPartyStart(game, source.map);
    auto chest = std::ranges::find_if(vChests, [&source](const Chest &candidate) {
        return !!candidate.inventory.find(source.item);
    });
    ASSERT_NE(chest, vChests.end());
    const int chestId = chest - vChests.begin();
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
        Chest::open(chestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    for (int attempt = 0; attempt < 40 && !pParty->hasItem(source.item) && pParty->pPickedItem.itemId != source.item;
         ++attempt) {
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasItem(source.item) || pParty->pPickedItem.itemId == source.item);
    if (pParty->pPickedItem.itemId == source.item)
        pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(source.item));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

bool eventPlacesStatuette(const EvtProgram &program, int eventId, const StatuetteShrine &shrine) {
    if (!program.hasEvent(eventId))
        return false;
    const auto &function = program.function(eventId);
    const bool consumesItem = std::ranges::any_of(function, [&shrine](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(shrine.item);
    });
    const bool setsBit = std::ranges::any_of(function, [&shrine](const EvtInstruction &instruction) {
        return eventSetsQuestBit(instruction, shrine.placedBit);
    });
    return consumesItem && setsBit;
}

void placeStatuette(EngineController &game, const StatuetteShrine &shrine) {
    loadMapAtPartyStart(game, shrine.map);
    auto decoration = std::ranges::find_if(pLevelDecorations, [&shrine](const LevelDecoration &candidate) {
        return candidate.uEventID != 0 &&
               (eventPlacesStatuette(engine->_localEventMap, candidate.uEventID, shrine) ||
                eventPlacesStatuette(engine->_globalEventMap, candidate.uEventID, shrine));
    });
    if (decoration != pLevelDecorations.end()) {
        const int decorationId = decoration - pLevelDecorations.begin();
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(shrine.map, decoration->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (pParty->_questBits[shrine.placedBit])
                break;
        }
    } else {
        auto face = std::ranges::find_if(pOutdoor->faces, [&shrine](const BLVFace &candidate) {
            return candidate.Clickable() &&
                   (eventPlacesStatuette(engine->_localEventMap, candidate.eventId, shrine) ||
                    eventPlacesStatuette(engine->_globalEventMap, candidate.eventId, shrine));
        });
        ASSERT_NE(face, pOutdoor->faces.end());
        const int faceId = face - pOutdoor->faces.begin();
        for (float distance : {150.0f, 300.0f, 500.0f}) {
            for (auto [direction, yaw] : {std::pair(Vec3f(1, 0, 0), 180), std::pair(Vec3f(-1, 0, 0), 0),
                                          std::pair(Vec3f(0, 1, 0), 270), std::pair(Vec3f(0, -1, 0), 90)}) {
                Vec3f position = face->boundingBox.center() + distance * direction;
                bool onWater = false;
                int floorFace = -1;
                position.z = ODM_GetFloorLevel(position, &onWater, &floorFace);
                game.teleportTo(shrine.map, position, yaw, -30);
                try {
                    game.pointMouseAtFace(faceId);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (pParty->_questBits[shrine.placedBit])
                    break;
            }
            if (pParty->_questBits[shrine.placedBit])
                break;
        }
    }
    EXPECT_TRUE(pParty->_questBits[shrine.placedBit]);
    EXPECT_FALSE(pParty->hasItem(shrine.item));
    EXPECT_NE(pParty->pPickedItem.itemId, shrine.item);
}

int totalExperience() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.experience;
    return result;
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void runThreeStatuettesRoute(EngineController &game, bool useCheckpointBaseline = false) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    if (useCheckpointBaseline) {
        pParty->SetGold(checkpointGoldBaseline);
        for (int i = 0; i < 4; ++i)
            pParty->pCharacters[i].experience = checkpointExperienceBaseline[i];
    }
    NPCData *lumbra = findThomLumbra();
    ASSERT_NE(lumbra, nullptr);
    const unsigned int offerEvent = findNpcEvent(lumbra, [](const EvtInstruction &instruction) {
        return eventSetsQuestBit(instruction, QBIT_STATUETTES_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);
    const int gold = pParty->uNumGold;
    const int experience = totalExperience();

    openLumbraHouse(game, lumbra);
    ASSERT_TRUE(selectNpcEvent(game, lumbra, offerEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_STATUETTES_ACTIVE]);
    const unsigned int handInEvent = findNpcEvent(lumbra, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_STATUETTES_ACTIVE);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);

    openLumbraHouse(game, lumbra);
    ASSERT_TRUE(selectNpcEvent(game, lumbra, handInEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_STATUETTES_ACTIVE]);
    EXPECT_EQ(pParty->uNumGold, gold);
    EXPECT_EQ(totalExperience(), experience);
    leaveHouse(game);

    for (const StatuetteSource &source : statuetteSources)
        acquireStatuette(game, source);
    for (const StatuetteShrine &shrine : statuetteShrines)
        placeStatuette(game, shrine);

    openLumbraHouse(game, lumbra);
    ASSERT_TRUE(selectNpcEvent(game, lumbra, handInEvent));
    EXPECT_FALSE(pParty->_questBits[QBIT_STATUETTES_ACTIVE]);
    EXPECT_EQ(pParty->uNumGold, gold + 50000);
    EXPECT_EQ(totalExperience(), experience + 4 * 50000);
    for (const Character &character : pParty->pCharacters)
        EXPECT_TRUE(character._achievedAwardsBits[AWARD_AVLEE_PLACED_THREE_STATUETTES]);
}

} // namespace

GAME_TEST(MM7SideQuests, ThreeStatuettesRequireChestsAndShrines) {
    runThreeStatuettesRoute(game);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteThreeStatuettes) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runThreeStatuettesRoute(game, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    leaveHouse(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TULAREAN_FOREST);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Three Statuettes checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadThreeStatuettes) {
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TULAREAN_FOREST);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_FALSE(pParty->_questBits[QBIT_STATUETTES_ACTIVE]);
    for (const StatuetteShrine &shrine : statuetteShrines) {
        EXPECT_TRUE(pParty->_questBits[shrine.placedBit]);
        EXPECT_FALSE(pParty->hasItem(shrine.item));
    }
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(pParty->uNumGold, checkpointGoldBaseline + 50000);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperienceBaseline[i] + 50000);
        EXPECT_TRUE(pParty->pCharacters[i]._achievedAwardsBits[AWARD_AVLEE_PLACED_THREE_STATUETTES]);
    }
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
