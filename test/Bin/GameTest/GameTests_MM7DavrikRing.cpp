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
#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Exception.h"

namespace {

constexpr int signetRingChestId = 0;
constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";

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

int countPartyItem(ItemId itemId) {
    int result = pParty->pPickedItem.itemId == itemId;
    for (const Character &character : pParty->pCharacters)
        for (InventoryConstEntry entry : character.inventory.entries())
            result += entry->itemId == itemId;
    return result;
}

void closeChest(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
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

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteDavriksSignetRing) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);

    pParty->_questBits.set(QBIT_DAVRIKS_SIGNET_RING_ACTIVE);
    ASSERT_FALSE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_TAKEN]);
    for (const Character &character : pParty->pCharacters)
        ASSERT_FALSE(character._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_DAVRIKS_SIGNET_RING]);

    game.teleportTo(MAP_BANDIT_CAVES, Vec3f(), 0);
    ASSERT_TRUE(vChests[signetRingChestId].inventory.find(ITEM_QUEST_SIGNET_RING));
    ASSERT_TRUE(lootSignetRing(game));
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), signetRingChestId);
    ASSERT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_ACTIVE]);
    ASSERT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_TAKEN]);
    ASSERT_FALSE(vChests[signetRingChestId].inventory.find(ITEM_QUEST_SIGNET_RING));
    pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    ASSERT_EQ(countPartyItem(ITEM_QUEST_SIGNET_RING), 1);
    closeChest(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BANDIT_CAVES);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());

    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Davrik's Signet Ring regional checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadDavriksSignetRing) {
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BANDIT_CAVES);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_DAVRIKS_SIGNET_RING_TAKEN]);
    EXPECT_EQ(countPartyItem(ITEM_QUEST_SIGNET_RING), 1);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(vChests[signetRingChestId].inventory.find(ITEM_QUEST_SIGNET_RING));
    for (const Character &character : pParty->pCharacters)
        EXPECT_FALSE(character._achievedAwardsBits[AWARD_HARMONDALE_RETURNED_DAVRIKS_SIGNET_RING]);

    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
        Chest::open(signetRingChestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), signetRingChestId);
    EXPECT_FALSE(vChests[signetRingChestId].inventory.find(ITEM_QUEST_SIGNET_RING));
    EXPECT_EQ(countPartyItem(ITEM_QUEST_SIGNET_RING), 1);
    closeChest(game);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
